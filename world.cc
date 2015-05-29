
#include "first.h"
#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <plib/sg.h>

#include "globals.h"
#include "world.h"
#include "billboard.h"
#include "printer.h"


Billboard light_tex;

int 
create_lightbulb_texture(int size)
{
    int radius = size / 2;
    
    float *data = new float[size * size * 4];

    for (int i = 0; i < size * size * 4; i++) 
    {
        data[i]= 1.0;
    }

//    memset(data, 255, size * size * 4 * sizeof(float));
        
    float radius2 = radius * radius;
    for (int x = 0; x < size; x++) 
    {
        for (int y = 0; y < size; y++) 
        {
            float pix;
            float xf = x - radius;
            float yf = y - radius;

            float dist2 = (xf * xf + yf * yf) * 1.1;

            if (dist2 < radius2) 
            {
                
                pix = 1.0 * sqrt(dist2 / radius2);
                if (pix > 1.0) 
                    pix = 1.0;
                else if (pix < 0) 
                    pix = 0.0;
                pix = 1.0 - pix;
            }
            else 
            {
                pix = 0.0f;
            }
            data[(x * size * 4) + (y * 4) + 3] = pix;
        }
    }
    
    globals.lightbulb_tex.create_from_data(size, size, 4,
//                                           GL_LUMINANCE, 
                                           GL_RGBA, 
                                           GL_FLOAT, data);
    
    return 1;	
}


void 
World::init()
{

    globals.world = this;

    glClearColor(0.6f, 0.0f, 0.9f, 0.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    globals.frustum = &frustum;

    // load asteroids
    char name[] = "data/objects/asteroidX.obj";
    for (int i = 0; i < 8; i++)
    {
        name[21] = '0' + i;
        asteroid[i] = Mesh::create_from_OBJ_file(name);
        assert(asteroid[i]);
        if (i <= 3) asteroid[i]->scale(40.0);
        else asteroid[i]->scale(20.0);
        asteroid[i]->update_bsphere();
    }

    // load bonus objects...
    apple = Mesh::create_from_OBJ_file("data/objects/apple.obj");
    banana = Mesh::create_from_OBJ_file("data/objects/banana.obj");
    assert(apple && banana);

    apple->scale(5.0);
    apple->update_bsphere();

    banana->scale(5.0);
    banana->update_bsphere();


    // load level data
    bool success = load_d3l_level("data/levels/roketz10.d3l");    
//   load_d3l_level("data/levels/cres.d3l");    
//    load_d3l_level("data/levels/reactor.d3l");    
//   load_d3l_level("data/levels/collision.d3l");    
    assert(success);

    // initialize rooms
    for (unsigned r = 0; r < rooms.size(); r++) 
    {    
        rooms[r]->init();
    }

//    exit(1);

//    if (!test_tex.create_from_file("data/gfx/textures/tegel1.png")) {
//	assert(0);
//    }

    create_lightbulb_texture(256);

//    light_tex.add_texture(*rooms[0]->faces[0].texture);
//    light_tex.add_images("data/gfx/smoke_exp/black_smoke", 3, ".tga");
//    light_tex.add_images("data/gfx/explosion/ExplosionCC", 3, ".tga");
//    light_tex.add_image("data/gfx/flare.tga");
//    light_tex.set_color(1.0, 1.0, 1.0, 0.5);
//    light_tex.add_texture(lm_textures[0]);
    light_tex.add_texture(globals.cube_tex);
//    light_tex.set_center(2100, 100, 2100.0);
//    light_tex.set_blend(GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR);
    light_tex.set_blend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    light_tex.set_fps(32.0);
  
    // create axis aligned bsp tree with all polygons in the world
    aabsp.create_face_tree(rooms, nr_faces);

    init_materials();
}

void
World::update(const sgVec3 pvp, const sgVec3 cvp)
{
    glPushMatrix();

    sgCopyVec3(cur_vp, cvp);
    sgCopyVec3(prev_vp, pvp);

    // don't need to check portals if we haven't moved...
    if (cur_vp[0] == prev_vp[0] &&
        cur_vp[1] == prev_vp[1] &&
        cur_vp[2] == prev_vp[2]) return;
    
    Room *new_room = cur_room->check_portals(prev_vp, cur_vp);
    
    if (new_room != NULL) {
        // switch rooms...

        cur_room->is_current = false;
        cur_room = new_room;
        cur_room->is_current = true;
    }

    glPopMatrix();
}

void 
World::draw()
{    
    sgMat4 c, v, p;
    unsigned i;
    vector<Room *> collected_rooms;
    vector<Object *> collected_objs;

    // setup view frustum
    glGetFloatv(GL_MODELVIEW_MATRIX, (float*)v);
    glGetFloatv(GL_PROJECTION_MATRIX, (float *)p);
    sgMultMat4(c, p, v);
    frustum.setup_from_matrix(c);

//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    // draw rooms

    cur_room->collect_and_recurse(cur_vp, collected_rooms, collected_objs, 0, 8);

    // update rooms and draw lightmaps
    for (i = 0; i < collected_rooms.size(); i++) 
    {
//        printf("frame %ld: updating room %d\n",
//               globals.frame, collected_rooms[i]->id); 
        collected_rooms[i]->update();
        collected_rooms[i]->draw_lightmaps();
    }

    // draw dynamic lights
    for (i = 0; i < dynamic_lights.size(); i++) 
    {
        dynamic_lights[i]->light_environment();

    }
    // draw objects and dynamic lights
//    printf("%d objects collected\n", collected_objs.size());

    for (i = 0; i < collected_objs.size(); i++) 
    {     
        collected_objs[i]->draw();
//        printf("frame %ld: drawing obj %d\n",
//               globals.frame, collected_objs[i]->get_id()); 
    }

    // finally, render textures on top of all...
    for (i = 0; i < collected_rooms.size(); i++) 
    {     
        collected_rooms[i]->draw_textures();
    }

    if (globals.frame < 2) {
//        light_tex.set_center(cur_vp[0] - 200, cur_vp[1] - 100, cur_vp[2]);
        light_tex.set_center(start_vp[0] - 100, start_vp[1] + 50, start_vp[2]);
    }
    light_tex.update(globals.ticks_per_frame);// / 1000.0);
//    light_tex.draw();
}


// just draw rooms... used to render cubemap faces.
void
World::draw_rooms(Room *start_room, sgVec3 vp)
{
    sgMat4 c, v, p;
    unsigned i;
    vector<Room *> collected_rooms;
    vector<Object *> collected_objs;

    // setup view frustum
    glGetFloatv(GL_MODELVIEW_MATRIX, (float*)v);
    glGetFloatv(GL_PROJECTION_MATRIX, (float *)p);
    sgMultMat4(c, p, v);
    frustum.setup_from_matrix(c);

    glClear(GL_DEPTH_BUFFER_BIT);

    glDisable(GL_LIGHTING);

    // draw rooms,
    start_room->collect_and_recurse(vp, collected_rooms, 
                                    collected_objs, 0, 1);

    // update rooms and draw lightmaps
    for (i = 0; i < collected_rooms.size(); i++) 
    {
        collected_rooms[i]->draw_lightmaps();
    }

    // no dynamic lights for cubemaps now
#if 0
    for (i = 0; i < dynamic_lights.size(); i++) 
    {
        dynamic_lights[i]->light_environment();
    }
#endif

    // finally, render textures on top of all...
    for (i = 0; i < collected_rooms.size(); i++) 
    {     
        collected_rooms[i]->draw_textures();
    }
}

Room *
World::room_by_id(unsigned id)
{
    for (unsigned i = 0; i < rooms.size(); i++) {
	if (rooms[i]->id == id) return rooms[i];
    }
    return NULL;
}


// hardcoded materials. this is all a BIG kludge... FIX!
void
World::init_materials()
{

    Material m;

    // 0: rocket material
    sgSetVec4(m.ambient, 0, 0, 0, 0);
    sgSetVec4(m.diffuse, 0.5f, 0.5f, 0.5f, 1.0f);
    sgSetVec4(m.specular, 1.0f, 1.0f, 1.0f, 1.0f);
    m.shininess = 30.0f;
    materials.push_back(m);

    // 1: asteroid material
    sgSetVec4(m.diffuse, 0.5f, 0.5f, 0.5f, 1.0f);
    sgSetVec4(m.specular, 0.8f, 0.8f, 0.8f, 1.0f);
    m.shininess = 10.0f;
    materials.push_back(m);

    // 2: apple green
    sgSetVec4(m.diffuse, 0.1f, 1.0f, 0.0f, 1.0f);
    sgSetVec4(m.specular, 1.0f, 1.0f, 1.0f, 1.0f);
    m.shininess = 10.0f;
    materials.push_back(m);

    // 3: apple brown
    sgSetVec4(m.diffuse, 0.5f, 0.3f, 0.0f, 1.0f);
    sgSetVec4(m.specular, 1.0f, 0.6f, 0.0f, 1.0f);
    m.shininess = 5.0f;
    materials.push_back(m);

    // 4: apple red
    sgSetVec4(m.diffuse, 1.0f, 0.1f, 0.0f, 1.0f);
    sgSetVec4(m.specular, 1.0f, 1.0f, 1.0f, 1.0f);
    m.shininess = 30.0f;
    materials.push_back(m);

    // 5: banana black 
    sgSetVec4(m.diffuse, 0.1f, 0.3f, 0.1f, 1.0f);
    sgSetVec4(m.specular, 0.2f, 0.6f, 0.2f, 1.0f);
    m.shininess = 5.0f;
    materials.push_back(m);

    // 6: banana yellow 
    sgSetVec4(m.diffuse, 0.86f, 0.79f, 0.1f, 1.0f);
    sgSetVec4(m.specular, 1.0f, 0.9f, 0.2f, 1.0f);
    m.shininess = 8.0f;
    materials.push_back(m);
}
