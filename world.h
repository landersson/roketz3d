
#ifndef WORLD_H
#define WORLD_H

#include "object.h"
#include "room.h"
#include "frustum.h"
#include "binfile.h"
#include "billboard.h"
#include "aabsp.h"

//
struct LightmapInfo
{
    int   lm_tex;      // index into global lightmap texture array

    float w, h;        // this stuff is not used right now
    float x1, y1;
    float xs, ys;

    char type;
};


// main world class
class World
{
    unsigned            nr_verts, nr_faces, nr_rooms;

    vector<Room *>	rooms;
    vector<Texture *>	textures;
    Frustum             frustum;

    // dynamic lights
    vector<DynamicLight *> dynamic_lights;

    vector<Billboard *> billboards;

 public:
    // current room
    Room		*cur_room;


    Room                *start_room;

    sgVec3              cur_vp, prev_vp, start_vp;

    Texture             *lm_textures;

    LightmapInfo        *lightmaps;

    vector<Material>    materials;

    // asix aligned bsp tree containing all faces
    AABSP               aabsp;

    // asteroid meshes... 0-3 big size, 4-7 small size
    Mesh                *asteroid[8];

    // bonus objects
    Mesh                *apple;
    Mesh                *banana;

    // functions for reading data from a D3L file
    void read_texture_names(BinaryFile &f);
    void read_object_names(BinaryFile &f);
    void read_face(BinaryFile &f, Face *face);
    void read_portal(BinaryFile &f, Portal *face);
    Room *read_room(BinaryFile &f);
    void read_lightmaps(BinaryFile &f);
    void read_rooms(BinaryFile &f);
    void read_objects(BinaryFile &f);

    bool load_gam(const char *fname);

    void add_dynamic_light(DynamicLight *light) {
        dynamic_lights.push_back(light);
    }

    int get_current_room_id() { return cur_room->id; }
    Room *get_current_room() { return cur_room; }
    void init();
    void init_materials();

    void draw();
    void draw_rooms(Room *start_room, sgVec3 vp);
    void update(const sgVec3 prev_vp, const sgVec3 cur_vp);

    Room *room_by_id(unsigned id);
    bool load_d3l_level(const char *fname);

};

#endif
