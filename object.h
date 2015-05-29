
#ifndef OBJECT_H
#define OBJECT_H

#include <vector>
#include "texture.h"
#include "geometry.h"

// mesh flags
#define MF_NORMALS              1

// object flags
#define OF_LIGHTING             1
#define OF_TEXTURED             2
#define OF_LIGHTMAPS            4
#define OF_BOUNCE               8
#define OF_ALWAYS_COLLIDE       16

#define OF_EXPLODED             32    // 

using namespace std;

struct Material
{
    sgVec4 ambient;
    sgVec4 diffuse;
    sgVec4 specular;
    float shininess;
};

class DynamicLight
{
 public:
    int                 id;
    bool                enabled;

    sgVec3              pos;
    sgVec3              color;
    float               intensity;

    void light_environment();
};


// vertex and face data
class Mesh
{
 public:
    int                 flags;
    int                 gl_disp_list;
    float               radius;

    unsigned int        nr_verts, nr_faces, nr_norms, nr_uvs;

    sgVec3              *vertices;
    sgVec3              *normals;
    
    Face		*faces;


    // constructor
    Mesh() { 
        vertices = NULL; normals = NULL; faces = NULL; 
        gl_disp_list = -1;
    }

    // functions for loading Wavefront OBJ data
    Face *load_OBJ_face(char *fstr, bool normals, bool tcoords);
    bool load_OBJ_file(const char *fname);
    bool load_OBJ_file(FILE *f);
    static Mesh* create_from_OBJ_file(const char *fname);

    void scale(float scale);
    float update_bsphere();

//    static vector<Object *> load_objects(const char *fname);

};


// object class
class Object 
{
 protected:
    int                 id;
    char                name[32];

 public:
    Mesh                *mesh;

    int                 flags, type, age;
    sgVec3              old_pos, pos;

    sgVec3              velocity;
    sgMat4              orient;

    long                collided_at_frame;
    long                moved_at_frame;

    // mass used in object-object collision response
    float               mass;

    class Room		*in_room;

    int                 light_id;

 public:
    Object              *last_object_hit;

    Object();

    char *get_name() { return name; }
    void set_name(const char *n) { strncpy(name, n, 32); }

    int get_id() { return id; }
    void set_id(int _id) { id = _id; }

//    void alloc_disp_list();
    
    // spatial manipulation
    void set_pos(sgVec3 _pos) { sgCopyVec3(pos, _pos); }
    void set_pos(float x, float y, float z) { sgSetVec3(pos, x, y, z); }

    void set_velocity(sgVec3 vel) { sgCopyVec3(velocity, vel); }
    void set_velocity(float x, float y, float z) { sgSetVec3(velocity, x, y, z); }
    void translate(float dx, float dy, float dz) {
        
    }
    void rotate(float angle, sgVec3 axis);

    bool move(sgVec3 move, float time);

    void reset_orientation() {sgMakeIdentMat4(orient); }
    void set_orientation(sgMat4 tmat) { sgCopyMat4(orient, tmat); }

//    void update_bsphere() { radius = mesh->update_bsphere(); }

    void bounce_objects(Object *obj_b, sgVec3 move_a, sgVec3 move_b,
                        sgVec3 hit_point, float hit_time);

    void update();

    bool check_wall_collisions(sgVec3 hit_point);

    void draw();
    void draw_shot();
    void draw_diamond();

    // apply dynamic lighting
    void light_environment();
};

#endif
