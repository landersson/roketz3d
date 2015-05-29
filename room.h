
#ifndef ROOM_H
#define ROOM_H

#include "object.h"
#include "frustum.h"
#include "binfile.h"


class Portal
{
 public:
    int               face_id;          // index of face this portal corresponds to
    class Face        *face;            // pointer to face

    int               croom_id;		// id of room this portal leads to
    class Room        *croom;           // room that this portal connects to
    
    bool check();
};

struct Light
{
    sgVec3 pos;
    float r, g, b;

};


class Room : public Object
{

    long                visited_at_frame;
    long                collected_at_frame;

    int                 texture_disp_list;
    int                 light_disp_list;

    bool                traversed;
 public:
    char                name[32];
    
    unsigned		id;
    bool                is_current; 

  
    unsigned 		nr_portals;
    Portal		*portals;
    
    vector<Object *>    objects;

    vector<Light>       lights;

    Frustum		*frustum;
    
// public:
    void init();
    void set_name(const char *str) { strncpy(name, str, 32); }

    void setup_lights();

    void update();

    void draw_and_recurse(sgVec3 vp);
    void draw();

    void draw_world(sgVec3 vp);

    void collect_and_recurse(sgVec3 vp, vector<Room *> &collected_rooms,
                             vector<Object *> &collected_objs,
                             int cur_depth, int max_depth);

    Room *check_portals(sgVec3 old_vp, sgVec3 new_vp);

    void add_object(Object *obj);
    void delete_object(Object *obj);

    void draw_lightmaps();
    void draw_textures();
};

#endif
