
#include "first.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <plib/sg.h>

#include "object.h"

#define MAX_VERTS 8192

static sgVec3 tmp_v[MAX_VERTS];
static sgVec3 tmp_n[MAX_VERTS];
static sgVec3 tmp_uv[MAX_VERTS];

static int v_cnt, vn_cnt, vt_cnt;
static char str[1024];

inline void 
get_2d(char* buf, sgVec2 v)
{
    if (2 != sscanf(buf,"%f %f", &v[0], &v[1]))
    {
        fprintf(stderr, "Error, couldn't parse 2D!\n");
    }
}

inline void 
get_3d(char* buf, sgVec3 v)
{
    if (3 != sscanf(buf,"%f %f %f", &v[0], &v[1], &v[2]))
    {
        fprintf(stderr,"Error, couldn't parse 3D!\n");
    }    
}

void
trim_str(char *str)
{
    while (*str != 0) {
	if (*str == '\n' || *str == '\r') {
	    *str = 0;
	    return;
	}
	str++;
    }
}

Face *
Mesh::load_OBJ_face(char *fstr, bool normals, bool tcoords)
{
    int p, n, t, skip;
    
    Face *face = new Face;

    face->flags = 0;
    face->lmi_handle = -1;

    bool ok;
    int i = 0;
    while (true) {
	if (normals && tcoords) {
	    ok = (3 == sscanf(fstr, " %d/%d/%d%n", &p, &t, &n, &skip));
	}
	else if (normals) {
	    ok = (2 == sscanf(fstr, " %d//%d%n", &p, &n, &skip));
	}
	else if (tcoords) {
	    ok = (2 == sscanf(fstr, " %d//%d%n", &p, &t, &skip));
	}
	else {
	    ok = (1 == sscanf(fstr, " %d%n", &p, &skip));
	}
	if (!ok) break;

//	printf("pushing index %d, str=%s\n", p - vCnt - 1, fstr);
	face->pindx.push_back(p - v_cnt - 1);
	if (normals) face->nindx.push_back(n - vn_cnt - 1);
//	if (tcoords) face->tindx.push_back(t - vt_cnt - 1);
	
	i++;
	if (i >= 32) assert(0);
	
	fstr += skip;
    }

    if (face->pindx.size() > 0) {
	return face;
    }
    else {
	delete face;
	return NULL;
    }
}

char *
find_obj_start(FILE *f)
{
    if (str[0] == 'o') return &str[2];
 
    while (fgets (str, 1024, f)) {
	switch (str[0]) {
	case 'o': return &str[2];
//	case 'v':
//	case 'f':
//	case 'p':
	    return NULL;
	}
    } 
    return NULL;	    
}

Mesh *
Mesh::create_from_OBJ_file(const char *fname)
{
    Mesh *m = new Mesh;

    m->load_OBJ_file(fname);

    return m;
}

bool 
Mesh::load_OBJ_file(const char *fname)
{
    char *str;
    FILE *f = fopen(fname, "r");
    assert(f);
//    ASSERT_ERR(f, "Error opening input file");

    v_cnt = vn_cnt = vt_cnt = 0;

    if (NULL == (str = find_obj_start(f))) {
	assert(0);
    }
    trim_str(str);
//    strncpy(name, str, 32);
//    printf("loading object %s:\n", name);
    return load_OBJ_file(f);
}

bool
Mesh::load_OBJ_file(FILE *f)
{
    unsigned i;
    bool		use_normals = false, use_tcoords = false;
    int                 cur_material;
    vector<Face *>      tmpfaces;

    nr_verts = 0; nr_norms = 0; nr_uvs = 0;
    
    while (fgets(str, 1024, f)) {
//	printf("%s\n", str);

        if (str[0] == 'v') {
	    // some type of vertex
            switch(str[1]) 
	    {
            case 't': // texture coordinate
		get_2d(&str[2], tmp_uv[nr_uvs++]);
		use_tcoords = true;
                break;
            case 'n': // normal...
                get_3d(&str[2], tmp_n[nr_norms++]);
		use_normals = true;
                break;
            case ' ': // vertex
            default: // vertex?
		assert(nr_verts < MAX_VERTS);
                get_3d(&str[2], tmp_v[nr_verts++]);
                // cerr << V << endl;
                break;
            }
        } 
	else if (str[0] == 'f') {
            
            Face *face;
	    face = load_OBJ_face(&str[1], use_normals, use_tcoords);
	    if (face == NULL) assert(0);
	
	    face->flags = 0;
	    face->material = cur_material;
	    face->vertices = vertices;
//	    printf("adding face\n");

	    tmpfaces.push_back(face);
        } 
	else if (str[0] == 'g') {
//	    cur_mat = new Material;
//            sscanf(&str[2], "%s", cur_mat->name);

//	    materials.push_back(cur_mat);
//	    printf("current material is %s\n", cur_mat->name);
	}
	else if (str[0] == 'u') 
        {
            int items = sscanf(str, "usemtl %d", &cur_material);
            if (items != 1) cur_material = 0;
            printf("using material %d\n", cur_material);
        }
	else if (str[0] == 'o') break;
    }

    // update global counters
    v_cnt += nr_verts;
    vn_cnt += nr_norms;
    vt_cnt += nr_uvs;

    // copy vertices
    vertices = new sgVec3[nr_verts];
    memcpy(vertices, tmp_v, nr_verts * sizeof(sgVec3));

    normals = new sgVec3[nr_norms];
    memcpy(normals, tmp_n, nr_norms * sizeof(sgVec3));

//    uv = new sgVec3[nr_uvs];
//    memcpy(uv, tmp_uv, nr_uvs * sizeof(sgVec3));

    // copy faces
    nr_faces = tmpfaces.size();
    faces = new Face[nr_faces];
    for (i = 0; i < nr_faces; i++) {
	memcpy(&faces[i], tmpfaces[i], sizeof(Face));
	faces[i].texture = NULL;
    }

    // initialize
    for (i = 0; i < nr_faces; i++) {
	faces[i].vertices = vertices;
	if (normals) faces[i].flags |= FF_NORMALS;
	faces[i].update_plane();
    }

//    reset_orientation();
//    printf("nr vertices: %d\nnr uvs: %d\nnr faces: %d\nloading done\n\n", 
//	   nr_verts, nr_uvs, faces.size());
    return true;
}

