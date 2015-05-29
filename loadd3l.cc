/*
 *  Roketz3D - load_d3l.cpp
 *
 *  Functions to extract useful information from Descent3 
 *  level (D3L) files. We currently use only room data and
 *  lightmaps chunks. 
 */

#include "first.h"
#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <plib/sg.h>

#include "globals.h"
#include "object.h"
#include "world.h"
#include "binfile.h"

#define SCALE 5.0f

#define D3L_FILE_TAG		"D3LV"

#define D3L_CHUNK_ROOMS		"ROOM"
#define D3L_CHUNK_TEXTURE_NAMES	"TXNM"
#define D3L_CHUNK_GENERIC_NAMES "GNNM"

#define D3L_CHUNK_LIGHTMAPS	"LMAP"
#define D3L_CHUNK_ROOM_AABB	"AABB"
#define D3L_CHUNK_NEW_LIGHTMAPS	"NLMP"
#define D3L_CHUNK_OBJECTS       "OBJS"

#define D3L_ROOM_NAME_LEN	19
#define D3L_PAGENAME_LEN	35
#define D3L_OBJ_NAME_LEN	19

#define ISCHUNK(name)		(!strncmp(chunk_name,name,4))

#define D3L_RF_DOOR             (1<<1)  // a 3d door is here. 
#define D3L_FF_LIGHTMAP         0x0001  // face has a lightmap
#define D3L_FF_TRI              0x020  // probably a lightsource

#define D3L_OBJ_DOOR            17
#define D3L_OBJ_PLAYER          4
#define D3L_OBJ_SOUNDSOURCE     24

// texture database
int nr_textures, nr_objects;
char *texture_names, *object_names;

typedef struct 
{
    char	*name;
    Texture	*tex;
} TexInfo;

TexInfo *texture_info;


typedef struct 
{
    char	*name;
    Object	*obj;
} ObjInfo;

ObjInfo *object_info;


//
void
World::read_texture_names(BinaryFile &f)
{
    char *dst = texture_names;
    int i;
    int n = f.read_int();

    nr_textures = n;

    // allocate temporary texture database
    texture_info = new TexInfo[n];

    for (i = 0; i < n; i++) 
    {
	texture_info[i].tex = NULL; // not loaded
    }

    // read texture names
    printf("%d texture names:\n", n);
    for (i = 0; i < n; i++) 
    {
	f.read_string(dst, 1024);
//	printf("tex name %i = %s\n", i, dst);
	texture_info[i].name = dst;
	dst += strlen(dst) + 1;
    }
}

void
World::read_object_names(BinaryFile &f)
{
    char *dst = object_names;
    int i;
    int n = f.read_int();

    nr_objects = n;

    // allocate temporary texture database
    object_info = new ObjInfo[n];

    for (i = 0; i < n; i++) 
    {
	object_info[i].obj = NULL; // not loaded
    }

    // read texture names
    printf("%d object names:\n", n);
    for (i = 0; i < n; i++) 
    {
	f.read_string(dst, 1024);
	printf("obj name %i = %s\n", i, dst);
	object_info[i].name = dst;
	dst += strlen(dst) + 1;
    }
}



void
World::read_face(BinaryFile &f, Face *face)
{
    int i;
    int nr_corners = f.read_byte();
    float vec[3];

    face->flags = 0;

//    printf(" %d corners\n", nr_corners);

    // read vertex indices

//    printf("    indices: ");    
    for (i = 0; i < nr_corners; i++) 
    {
	int indx = f.read_short();
	face->pindx.push_back(indx);
//	printf("%d ", indx);

    }
//    printf("\n");    

    // read texture coordinates
    face->uv = new sgVec2[nr_corners];

//    printf("    uvs: ");    
    face->flags |= FF_TEXTURED;
    for (i = 0; i < nr_corners; i++) 
    {
	face->uv[i][0] = f.read_float();
	face->uv[i][1] = f.read_float();

    f.read_byte();
//	float alpha = f.read_byte();

//	printf("(%.2f, %.2f, %.2f) ", face->uv[i][0], 
//	       face->uv[i][1], alpha);
    }
//    printf("\n");    

    // read flags
    int flags = f.read_short();
//    printf("    flags = 0x%x\n", flags);

//    if (flags & 0x100) {
//        printf("lightface!\n");
//    }

    // read portal number
    f.read_byte(); // portal_num
//    printf("    portal num = %d\n", portal_num);

    // read texture number
    int tnum = f.read_short();
//    printf("    texture num = %d\n", tnum);    

    assert(tnum < nr_textures);

    if (texture_info[tnum].tex == NULL)
    {
        char fname[128], *punkt;        
        printf("loading new texture: %s\n", texture_info[tnum].name);
        texture_info[tnum].tex = new Texture;

        if (NULL != (punkt = strchr(texture_info[tnum].name, '.'))) 
        {
            *punkt = 0;
        }
        //sprintf(fname, "data\\gfx\\d3images\\%s.tga", texture_info[tnum].name);
        sprintf(fname, "data/gfx/d3images/%s.tga", texture_info[tnum].name); 
        if (!texture_info[tnum].tex->create_from_file(fname)) 
        {
            // if not found, use first texture loaded...
            delete texture_info[tnum].tex;
            assert(!textures.empty());
            texture_info[tnum].tex = textures.front();
        }
        textures.push_back(texture_info[tnum].tex);
    }
    face->texture = texture_info[tnum].tex;

    // check if there is a lightmap
    if (flags & D3L_FF_LIGHTMAP) 
    {
	face->lmi_handle = f.read_short();

//        printf("lmi_handle = %d : \n", face->lmi_handle);

        face->luv = new sgVec2[nr_corners];

	// read lightmap uv's
	for (i = 0; i < nr_corners; i++) 
        {
	    face->luv[i][0] = f.read_float();
	    face->luv[i][1] = f.read_float();
	    
//            printf("(%.2f, %.2f)", u, v);

	    // these may need snapping to [0,1]
	
	}
//        printf("\n");
    }
    else face->lmi_handle = -1;

    // what's this??
    f.read_byte(); // light_multiple
    int special = f.read_byte();

    if (special) 
    {
        int num_smooth_verts = 0;
        
        f.read_byte(); // type
        int num = f.read_byte();
        int smooth = f.read_byte();
        
        if (smooth) 
        {
            num_smooth_verts = f.read_byte();
        }
        
        for (i = 0; i < num; i++) 
        {
            f.read_vector(vec);
            f.read_short();
        }
        
        if (smooth) 
        {
            for (i = 0; i < num_smooth_verts; i++) 
            {
                f.read_vector(vec);
            }
        }
    }
}

void
World::read_portal(BinaryFile &f, Portal *p)
{
    f.read_int(); // flags

    p->face_id = f.read_short();
    p->croom_id = f.read_int();
    f.read_int(); // cportal

    f.read_short(); // bnode_index

    // ??
    float path_pnt[3];
    f.read_vector(path_pnt);
    f.read_int(); // combine_master

//    printf(" flags=0x%x, face=%d, croom=%d, cportal=%d\n",
//	   flags, p->face_id, p->croom_id, cportal);
}

Room *
World::read_room(BinaryFile &f)
{
    unsigned i;
//    char buf[1024];
    char name[D3L_ROOM_NAME_LEN + 1];
    Room *r = new Room;

//    printf("new room at offset = %ld\n", f.tell());

    r->id = f.read_short();
    
//    int offs = lseek(fd, 0, SEEK_CUR);

    r->mesh = new Mesh();

    Mesh *m = r->mesh;

    m->nr_verts = f.read_int();
    m->nr_faces = f.read_int();

    r->nr_portals = f.read_int();

    f.read_string(name, sizeof(name));
    r->set_name(name);

#if 1    
    printf("room '%s' (%d), offset=%d\n", name, r->id, (int)(f.tell()));

    printf("  nr_verts=%d\n", m->nr_verts);
    printf("  nr_faces=%d\n", m->nr_faces);
    printf("  nr_portals=%d\n", r->nr_portals);
#endif
    assert(m->nr_verts < 10000);

    float vec[3];

    // read path point (what's this?)
    f.read_vector(vec);
//    printf("path point = (%f, %f, %f)\n", vec[0], vec[1], vec[2]);

    // read vertices
    m->vertices = new sgVec3[m->nr_verts];
    for (i = 0; i < m->nr_verts; i++) 
    {
	f.read_vector(m->vertices[i]);

        sgScaleVec3(m->vertices[i], SCALE);

//	printf("  vertex %i: (%.2f, %.2f, %.2f)\n", i, r->vertices[i][0], 
//	       r->vertices[i][1], r->vertices[i][2]);

    }

    // read faces
    m->faces = new Face[m->nr_faces];
    for (i = 0; i < m->nr_faces; i++) 
    {
//	printf("  face %i:", i);
	read_face(f, &m->faces[i]);
	m->faces[i].vertices = m->vertices;
	m->faces[i].update_plane();
    }

    // read portals
    r->portals = new Portal[r->nr_portals];    
    for (i = 0; i < r->nr_portals; i++) 
    {
	printf("  portal %i: ", i);
	read_portal(f, &r->portals[i]);
    }

    int flags = f.read_int();

    printf("  flags = 0x%x\n", flags);

    f.read_byte(); // pulse_time
    f.read_byte(); // pulse_offset
    f.read_short(); // mirror_face

    // door stuff... ignore
    if (flags & D3L_RF_DOOR) 
    {
	assert(0);

	f.read_byte(); // dflags
	f.read_byte(); // doornum

	f.read_float(); // position
    }

    // volume lights...
    if (1 == f.read_byte()) 
    {
	int w = f.read_int();
	int h = f.read_int();
	int d = f.read_int();

	int size = w * h * d;
//	printf("volume lights %d, %d, %d\n", w, h, d);

//	char buf[1024]; // FIX!!
	char *vlights = new char[size];
	f.read_compressed_bytes(vlights, size);
	delete vlights;
    }

    // fog parameters
    f.read_float(); // fog_depth
    f.read_float();     // fog_r
    f.read_float();     // fog_g
    f.read_float();     // fog_b

//    printf("  fog rgb = (%.4f, %.4f, %.4f), depth = %.4f\n", 
//	   fog_r, fog_g, fog_b, fog_depth);

    // ambient sound pattern name

    char tbuf[D3L_PAGENAME_LEN];
    f.read_string(tbuf, sizeof(tbuf));

//    printf("ambient sound: %s\n", tbuf);

    f.read_byte();  // env_reverb
    f.read_float(); // damage
    f.read_byte();  // damage_type 

//    printf("damage = %f\n", damage);

    r->flags = 0;
    r->reset_orientation();

    printf("room read\n");
    return r;
}


void
World::read_rooms(BinaryFile &f)
{
    // assume texture names loaded...
    assert(texture_info);

    nr_rooms = f.read_int();
    nr_verts = f.read_int();
    nr_faces = f.read_int();

    int nr_faceverts = f.read_int();
    int nr_portals = f.read_int();  

    printf("nr_rooms=%d\n", nr_rooms);
    printf("nr_verts=%d\n", nr_verts);
    printf("nr_faces=%d\n", nr_faces);
    printf("nr_faceverts=%d\n", nr_faceverts);
    printf("nr_portals=%d\n", nr_portals);

    // read rooms
    for (unsigned i = 0; i < nr_rooms; i++) 
    {	
	Room *r = read_room(f);	

	rooms.push_back(r);
    }
}

float get_color(char *hex)
{
    int hi, lo;
    if (hex[0] >= '0' && hex[0] <= '9') hi = hex[0] - '0';
    else hi = 10 + hex[0] - 'A';
    if (hex[1] >= '0' && hex[0] <= '9') lo = hex[0] - '0';
    else lo = 10 + hex[0] - 'A';

    return (float)(16*hi + lo) / 256.0f;
}

void
World::read_objects(BinaryFile &f)
{
    int nr_objs = f.read_int();

    char objname[D3L_OBJ_NAME_LEN + 1];

    Object *obj;

    for (int i = 0; i < nr_objs; i++) 
    {
        f.tell(); // offs
        int handle = f.read_int();

        char type = f.read_byte();

        int id = f.read_short();
      
        f.read_string(objname, sizeof(objname));

        f.read_int(); // flags

        if (type == D3L_OBJ_DOOR) f.read_short(); // skip door shields

        int room_id = f.read_int();

//        printf("object %i '%s': type = %d, id = %d, flags=0x%x, handle=%d, room=%d\n",
//               i, objname, type, id, flags, handle, room_id);

        sgVec3 pos;

        f.read_vector(pos);

        sgScaleVec3(pos, SCALE);

//	pos[0] -= 2100.0;
//	pos[1] += 100;
//	pos[2] -= 2100.0;


        sgMat3 orient;
        f.read_matrix(orient[0]);
        
        printf("   position: %.2f,%.2f,%.2f\n", pos[0], pos[1], pos[2]);

        f.read_byte(); // contains_type
        f.read_byte(); // contains_id
        f.read_byte(); // contains_count  

        f.read_float(); // life left
//        printf("   life left: %.2f\n", f.read_float()); // lifeleft

        if (type == D3L_OBJ_SOUNDSOURCE) 
        {
//            assert(0);
            char sndname[D3L_PAGENAME_LEN + 1];
            f.read_string(sndname, sizeof(sndname));
            f.read_float(); // volume
        }
        
        int b;
        b = f.read_byte();
        if (b > 0) 
        {
            // default script name
            f.skip_bytes(i);
        }

        b = f.read_byte();
        if (b > 0) 
        {
            // default module name
            f.skip_bytes(i);
        }

#if 1
        int lm_data = f.read_byte();

        if (lm_data) 
        {
//            printf("   lightmap:\n");
            int nr_models = f.read_byte();

            for (int i = 0; i < nr_models; i++) 
            {
                int nr_faces = f.read_short();

                for (int t = 0; t < nr_faces; t++) 
                {
                    f.read_short(); // lmi_handle

                    sgVec3 tmp;
                    f.read_vector(tmp); // rvec
                    f.read_vector(tmp); // uvec
                    
                    int nr_verts = f.read_byte();

                    for (int k = 0; k < nr_verts; k++) 
                    {
                        f.read_float(); // u
                        f.read_float(); // v                 
                    }
                }
            }
        }
#endif

        Room *r = room_by_id(room_id);
        assert(r);

        bool asteroid = (strstr(objname, "asteroid") != NULL);
        bool apple = (strstr(objname, "apple") != NULL);
        bool banana = (strstr(objname, "banana") != NULL);

        if (asteroid || apple || banana) 
        {
            obj = new Object;
            obj->set_name(objname);
            obj->set_id(2*handle);
            printf("object id = %d\n", id);
//            sgScaleVec3(pos, 6.0);
            obj->set_pos(pos);
            obj->set_velocity(0, 0, 0);
            obj->reset_orientation();

            if (asteroid) obj->mesh = globals.world->asteroid[rand() % 4];
            else if (apple) obj->mesh = globals.world->apple;
            else if (banana) obj->mesh = globals.world->banana;
            
            r->objects.push_back(obj);
        }
        else if (strstr(objname, "light")) 
        {
            Light light;

            sgCopyVec3(light.pos, pos);
            char *rgb = strchr(objname, ':');
            assert(rgb);
            rgb++;
            assert(strlen(rgb) >= 6);
            light.r = get_color(rgb);
            light.g = get_color(rgb + 2);
            light.b = get_color(rgb + 4);
            
            printf("light: room %d, color = %.2f, %.2f, %.2f\n",
                   r->id, light.r, light.g, light.b);
            
            r->lights.push_back(light);
        }
        else if (type == D3L_OBJ_PLAYER && id == 0) 
        {
            printf("start pos '%s', id=%d\n", objname, id);
            sgCopyVec3(start_vp, pos);
            globals.cam->set_pos(pos[0], pos[1], pos[2]);

            start_room = cur_room = r;
            cur_room->is_current = true;            
        }
    }
}

void 
World::read_lightmaps(BinaryFile &f)
{
    int nr_maps = f.read_int();
    short *data;


    // read actual lightmap textures

    printf("nr lightmaps = %d\n", nr_maps);
    lm_textures = new Texture[nr_maps];
    int i;
    for (i = 0; i < nr_maps; i++) 
    {
        int w = f.read_short();
        int h = f.read_short();

//        printf("lightmap %d: %dx%d\n", i, w, h);

        data = new short[w * h];

        f.read_compressed_shorts(data, w * h);

        // create lighmap texture from the data
#if 1
        lm_textures[i].create_from_data(w, h, 3, GL_BGRA,
                                        GL_UNSIGNED_SHORT_1_5_5_5_REV,
                                        data);
#else
        lm_textures[i].create_from_data(w, h, 4, GL_RGBA,
//                                        GL_UNSIGNED_SHORT_1_5_5_5_REV,
                                        GL_UNSIGNED_SHORT_5_5_5_1_EXT, 
                                        data);
#endif

    }

    nr_maps = f.read_int();
    printf("nr lightmap infos = %d\n", nr_maps);

    lightmaps = new LightmapInfo[nr_maps];

    for (i = 0; i < nr_maps; i++) 
    {
        lightmaps[i].lm_tex = f.read_short();

        lightmaps[i].w = f.read_short();
        lightmaps[i].h = f.read_short();
        lightmaps[i].type = f.read_byte();

//        printf("lightmap info %d: w=%f, h=%f, type = %f, remap = %f\n",
//               i, lightmaps[i].w, lightmaps[i].h, 
        //              lightmaps[i].type, lightmaps[i].lm_tex);

        lightmaps[i].x1 = f.read_short();
        lightmaps[i].y1 = f.read_short();
        lightmaps[i].xs = f.read_byte();
        lightmaps[i].ys = f.read_byte();

//        printf("                  x1=%f, y1=%f, xs=%f, ys=%f\n",
//               lightmaps[i].x1, lightmaps[i].y1, 
//               lightmaps[i].xs, lightmaps[i].ys);

        sgVec3 ul, norm;

        f.read_vector(ul);
        f.read_vector(norm);

    }
}

bool
World::load_d3l_level(const char *fname)
{
    BinaryFile f;
    if (!f.open_file(fname)) return false;

    char tag[4];
    f.read_bytes(tag, 4);
    if (strncmp(tag, D3L_FILE_TAG, 4)) 
    {
	fprintf(stderr, "not a level file!\n");
	exit(1);
    }

    int version = f.read_int();
    printf("version: %d\n", version);

    // temporary data for texture names
    texture_names = NULL;
    cur_room = NULL;

    // read and parse chunks
    while (!f.eof()) 
    {
	char chunk_name[4];

	f.read_bytes(chunk_name, 4);
	int chunk_start = f.tell();
	int chunk_size = f.read_int();

	printf("Chunk: %c%c%c%c, size=%d\n",chunk_name[0],
	       chunk_name[1], chunk_name[2],
	       chunk_name[3], chunk_size);

        if (ISCHUNK(D3L_CHUNK_NEW_LIGHTMAPS)) 
        {
            read_lightmaps(f);
        }

	if (ISCHUNK(D3L_CHUNK_ROOMS)) 
        {
	    read_rooms(f);
	}

	if (ISCHUNK(D3L_CHUNK_OBJECTS)) 
        {
	    read_objects(f);
	}


	if (ISCHUNK(D3L_CHUNK_TEXTURE_NAMES)) 
        {
	    texture_names = new char[chunk_size];
	    read_texture_names(f);
	}

	if (ISCHUNK(D3L_CHUNK_GENERIC_NAMES)) 
        {
	    object_names = new char[chunk_size];
	    read_object_names(f);
	}

	
	// jump to next chunk
	f.seek(chunk_start + chunk_size);
    }
    f.close_file();
    if (texture_info) delete texture_info;
    if (texture_names) delete texture_names;

    // initialize

    // look for and set up portals
    for (unsigned i = 0; i < rooms.size(); i++) 
    {
	Room *r = rooms[i];
	printf("room %d:\n", r->id);

        r->is_current = false;

	for (unsigned p = 0; p < r->nr_portals; p++) 
        {
	    r->frustum = &frustum;

            assert((unsigned)r->portals[p].face_id < r->mesh->nr_faces);
	    r->portals[p].face = &r->mesh->faces[r->portals[p].face_id];
	    r->portals[p].face->flags |= FF_PORTAL;
	    r->portals[p].croom = room_by_id(r->portals[p].croom_id);

            assert(r->portals[p].croom);

//	    for (unsigned f = 0; f < r->nr_faces; f++) {
//		r->faces[f].vertices = r->vertices;
//	    }

	}
    }
    f.close_file();
    return true;
}


#define PAGETYPE_UNKNOWN	0
#define PAGETYPE_TEXTURE	1
#define PAGETYPE_WEAPON		2
#define PAGETYPE_ROBOT		3
#define PAGETYPE_POWERUP	4
#define PAGETYPE_DOOR		5
#define PAGETYPE_SHIP		6
#define PAGETYPE_SOUND		7
#define PAGETYPE_MEGACELL	8
#define PAGETYPE_GAMEFILE	9
#define PAGETYPE_GENERIC	10

#define PAGENAME_LEN                35




void
read_generic_page(BinaryFile &f)
{
    f.read_short(); // version
    f.read_byte(); // type

    char name[PAGENAME_LEN+1];
    
    f.read_string(name, sizeof(name));
}



bool
World::load_gam(const char *fname)
{
    BinaryFile f;
    if (!f.open_file(fname)) return false;

    int handle = 0;
    while (!f.eof()) {

	int page_type = f.read_byte();

        int page_start = f.tell();
        int page_size = f.read_int();

        assert(page_type <= PAGETYPE_GENERIC);

        printf("%4d: pagetype %d, size=%d\n", handle, page_type, page_size);

        switch(page_type)
        {
        case PAGETYPE_GENERIC:
            read_generic_page(f);
            break;
        }

        f.seek(page_start + page_size);
        handle++;
    }
    f.close_file();
    return true;
}
