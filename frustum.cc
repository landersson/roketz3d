
#include "first.h"
#include <stdio.h>

#include <plib/sg.h>

#include "globals.h"
#include "world.h"
#include "object.h"


bool
Frustum::cull_face(Face *f)
{
    for (unsigned p = 0; p < planes.size(); p++) {
	bool in = false;
//	printf("plane %d: (%.2f, %.2f, %.2f)\n", p, planes[p].n[0],
//            planes[p].n[1], planes[p].n[2]);
	for (unsigned i = 0; i < f->pindx.size(); i++) {
	    float dist = planes[p].dist_to_point(f->vertices[f->pindx[i]]);
//	    printf("  dist to p %d = %f\n", i, dist);
	    if (dist > 0) {
		in = true;
		break;
	    }
	}
	if (!in) return false;
    }
    return true;
}

bool
Frustum::cull_sphere(sgVec3 center, float radius)
{
    for (unsigned i = 0; i < planes.size(); i++) {
//	float dist = planes[i].dist_to_point(center);
//        printf("dist to plane %d = %.3f\n", i, dist);
	if (planes[i].dist_to_point(center) < -radius) return false;
    }
    return true;
}

// push clipping planes from portal
void
Frustum::push_portal(Portal *p, sgVec3 vp)
{
    Face *face = p->face; 

    sizes.push_back(planes.size());
    
    for (unsigned i1 = 0; i1 < face->pindx.size(); i1++) {
	unsigned i2 = i1 + 1;
	if (i2 == face->pindx.size()) i2 = 0;

	Plane plane = Plane(face->vertices[face->pindx[i1]], vp, 
			    face->vertices[face->pindx[i2]]);

	planes.push_back(plane);
    }
}

// push top portal planes
void
Frustum::pop_portal()
{
    assert(sizes.size() > 0);
    unsigned size = sizes.back();
    sizes.pop_back();
    while (planes.size() > size) planes.pop_back();
}

void
Frustum::push_plane(Plane &p)
{
    planes.push_back(p);
}

void
Frustum::setup_from_matrix(sgMat4 mat)
{
    Plane p;

    planes.clear();

    // left
    p = Plane(mat[0][3] + mat[0][0],
	      mat[1][3] + mat[1][0],
	      mat[2][3] + mat[2][0],
	      mat[3][3] + mat[3][0]);     	          
//    p.print();
    planes.push_back(p);

    // right
    p = Plane(mat[0][3] - mat[0][0],
	      mat[1][3] - mat[1][0],
	      mat[2][3] - mat[2][0],
	      mat[3][3] - mat[3][0]);     	          
    planes.push_back(p);
//    p.print();

    // botton
    p = Plane(mat[0][3] + mat[0][1],
	      mat[1][3] + mat[1][1],
	      mat[2][3] + mat[2][1],
	      mat[3][3] + mat[3][1]);     
    planes.push_back(p);
//    p.

    // top
    p = Plane(mat[0][3] - mat[0][1],
	      mat[1][3] - mat[1][1],
	      mat[2][3] - mat[2][1],
	      mat[3][3] - mat[3][1]);     
    planes.push_back(p);

    // near
    p = Plane(mat[0][3] + mat[0][2],
	      mat[1][3] + mat[1][2],
	      mat[2][3] + mat[2][2],
	      mat[3][3] + mat[3][2]);     
//    planes.push_back(p);
//    p.print();

    // far
    p = Plane(mat[0][3] - mat[0][2],
	      mat[1][3] - mat[1][2],
	      mat[2][3] - mat[2][2],
	      mat[3][3] - mat[3][2]);     
//    planes.push_back(p);	          
//    p.print();
}
