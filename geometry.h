
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>
#include "texture.h"

// face flags
#define FF_NORMALS	1     // face has vertex normals
#define FF_PORTAL       2     // face is a portal
#define FF_TEXTURED     4     // face is textured

using namespace std;

class Sphere
{
 public:
    sgVec3	center;
    float	radius;

    Sphere(sgVec3 c, float r) { sgCopyVec3(center, c); radius = r; }

    bool collide_sphere_sphere(Sphere &spb, const sgVec3 va, const sgVec3 vb, 
                               sgVec3 hit_point, float *hit_time);
};


class Plane
{
 public:
    sgVec3 n;
    float  d;

    Plane() {}

    // create plane from normal and d value
    Plane(sgVec3 an, float ad) { 
	memcpy(n, an, sizeof(sgVec3)); d = ad; 
	normalize();
    }
    // create plane from components
    Plane(float a, float b, float c, float ad) { 
	n[0] = a; n[1] = b; n[2] = c; d = ad; 
	normalize();
    }
    // calculate plane from three clockwise points
    Plane(sgVec3 p1, sgVec3 p2, sgVec3 p3) {
	sgVec3 v1, v2;
	sgSubVec3(v1, p3, p2);
	sgSubVec3(v2, p1, p2);

	sgVectorProductVec3(n, v1, v2);
#if 1
        if (sgLengthVec3(n) < 0.001) {
            printf("WARNING: small normal!\n");
        }
#endif
	d = -sgScalarProductVec3(n, p2);
	normalize();
    }
    
    void normalize();
    void print();
    void from_vectors(sgVec3 p0, sgVec3 p1, sgVec3 p2);

    float dist_to_point(const sgVec3 p);

    bool intersects_ray(const sgVec3 o, const sgVec3 d, float *save_t);

    float project_point(sgVec3 point, sgVec3 proj);
};


class Face
{
    bool collide_edge_sphere(const sgVec3 xs0, const sgVec3 vs, float rad, 
                             const sgVec3 v0, const sgVec3 v1, 
                             float &hit_time, sgVec3 hit_point);
 public:
    int                 flags;
    int                 check_at_frame;
    int                 lit_by_obj;

    Plane               plane;

    sgVec3              *vertices;   // pointer to vertex array
    sgVec2              *uv;         // uv coods
    sgVec2              *luv;        // lightmap uv's

    Texture             *texture;

    // index into global matrial table
    short                 material;
    // index into global lightmap table
    short                 lmi_handle;

    vector<int>		pindx;
    vector<int>		nindx;       

// public:
    void update_plane();
    bool backface_cull(const sgVec3 vp);
    bool point_inside(sgVec3 ip);
    bool intersects_ray(const sgVec3 o, const sgVec3 d, float *save_t);
    bool collide_sphere(const sgVec3 o, const sgVec3 dir, float radius, 
                        sgVec3 new_pos, sgVec3 hit_pos, float *save_t);
};

#endif
