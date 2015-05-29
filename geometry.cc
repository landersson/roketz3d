/*----------------------------------------------------------------------------





----------------------------------------------------------------------------*/

#include "first.h"
#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <plib/sg.h>

#include "globals.h"
#include "geometry.h"
#include "printer.h"
 

float
Plane::dist_to_point(const sgVec3 p)
{
    return sgScalarProductVec3(n, p) + d;    
}

float
Plane::project_point(sgVec3 point, sgVec3 proj)
{
    float d = dist_to_point(point);

    sgAddScaledVec3(proj, point, n, -d);
    return d;
}

void
Plane::print()
{
    printf("(%f, %f, %f, %f\n", n[0], n[1], n[2], d);
}

void
Plane::normalize()
{
    float scale = 1.0 / sgLengthVec3(n);
    sgScaleVec3(n, scale);
    d *= scale;
}

// check intersection between plane and ray. returns false if
// they are almost parallell.
bool 
Plane::intersects_ray(const sgVec3 o, const sgVec3 dir, float *save_t)
{
    float denom = sgScalarProductVec3(n, dir);
    if (fabs(denom) < 1e-16) return false;

    // distance along the ray that it hits the plane
    if (save_t != NULL) 
    {
        *save_t = (-d - sgScalarProductVec3(n, o)) / denom;
    }
    return true;
}

void
Face::update_plane()
{
    // find the two vectors that are most "perpendicular"
    // not fast, bu usually only done once...
    int indx0, indx1, indx2;
    float min = 1000000.0;

    indx0 = indx1 = indx2 = 0;

    for (unsigned i0 = 0; i0 < pindx.size(); i0++) 
    {
        unsigned i1 = i0 + 1;
        if (i1 >= pindx.size()) i1 = 0;

        unsigned i2 = i1 + 1;
        if (i2 >= pindx.size()) i2 = 0;
        
        sgVec3 v1, v2;

        sgSubVec3(v1, vertices[pindx[i0]], vertices[pindx[i1]]);
        sgSubVec3(v2, vertices[pindx[i2]], vertices[pindx[i1]]);

        float n = sgScalarProductVec3(v1, v2);

        if (fabs(n) <= min) 
        {
            min = fabs(n);
            indx0 = i0;
            indx1 = i1;
            indx2 = i2;
        }

    }
        
    // calculate plane parameters from the two vectors
    plane = Plane(vertices[pindx[indx0]], 
		  vertices[pindx[indx1]],
		  vertices[pindx[indx2]]);

}


// backface cull face wrt given viewpoint
bool
Face::backface_cull(const sgVec3 vp)
{
    sgVec3 v;
    sgSubVec3(v, vp, vertices[pindx[0]]);
#if 0
    printf("normal = (%f, %f, %f), vp = (%f, %f, %f)\n",
	   plane.n[0], plane.n[1], plane.n[2],
	   vp[0], vp[1], vp[2]);
#endif
    if (sgScalarProductVec3(v, plane.n) >= 0) return true;
    else return false;
}


bool
Face::point_inside(sgVec3 ip)
{
    int norm_max;

    // determine plane normal major direction
    if (fabs(plane.n[0]) > fabs(plane.n[1])) norm_max = 0;
    else norm_max = 1;
    if (fabs(plane.n[2]) > fabs(plane.n[norm_max])) norm_max = 2;

//    printf("p0.z = %.2f, t.z = %.2f, major = %d\n", 
//           vertices[pindx[0]][norm_max], ip[norm_max], norm_max);

    // i0 = projected x index, i1 = projected y index
    int x, y;
    if (norm_max == 0) { x = 1; y = 2; }
    else if (norm_max == 1) { x = 0; y = 2; }
    else { x = 0; y = 1; }

    bool inside = false;

    int e0 = pindx.size() - 1;
    int e1 = 0;
    
    // point above 'origo'
//    above0 = vertices[e0][y] >= ip[y];

//    printf("t.xy = (%.2f, %.2f)\n", ip[x], ip[y]);

    for (unsigned i = 0; i < pindx.size(); i++) 
    {       
        int p0 = pindx[e0];
        int p1 = pindx[e1];

//        above1 = vertices[p1][y] >= ip[y];

//        printf("p0.xy = (%.2f, %.2f)\n", vertices[p0][x],
//               vertices[p0][y]);

//        printf("p1.xy = (%.2f, %.2f)\n", vertices[p1][x],
//               vertices[p1][y]);

#if 0
        if (above0 != above1) 
        {
            // this edge crosses the x-axis, check on what side of y-axis
            
            if (above1 == ((vertices[p1][y] - ip[y]) * 
                           (vertices[p0][x] - vertices[p1][x])) >=
                ((vertices[p1][x] - ip[x]) * (vertices[p0][y] - vertices[p1][y])))
            {
                printf("flip!\n");
                inside = !inside;
            }
        }
#endif
        if (((vertices[p0][y] - ip[y]) * (vertices[p1][y] - ip[y])) < 0) 
        {
            // edge crossed y axis

            float dx = vertices[p1][x] - vertices[p0][x];
            float dy = vertices[p1][y] - vertices[p0][y];

            float delta = dx / dy;

            float cx = vertices[p0][x] + delta * (ip[y] - vertices[p0][y]);

            if (cx >= ip[x]) 
            {
                inside = !inside;
//                printf("flip!\n");
            }
        }

//        above0 = above1;
        e0 = e1;
        e1++;
    }
    return inside;
}

bool
Face::intersects_ray(const sgVec3 o, const sgVec3 dir, float *save_t)
{
    sgVec3 ip;
    float t;

#if 0
    float denom = sgScalarProductVec3(plane.n, d);
    if (fabs(denom) < 1e-16) return false;

    // distance along the ray that it hits the plane
    float t = (-plane.d - sgScalarProductVec3(plane.n, o)) / denom;
#else
    if (!plane.intersects_ray(o, dir, &t)) return false;
#endif

    if (save_t != NULL) *save_t = t;

//    printf("   t = %.2f, normal = (%.2f, %.2f, %.2f)\n", t,
//           plane.n[0], plane.n[1], plane.n[2]);

    // ip is intersection point ('origo' in test)
    sgAddScaledVec3(ip, o, dir, t);

    return point_inside(ip);
}

//----------------------------------------------------------------------------
// collide_shpere - checks collision between moving sphere and static face
//
// in:   c:         sphere center
//       v:         sphere velocity
//       radius:    sphere radius
//
// out:  new_pos:   center of sphere when just touching the face
//       hit_pos:   point on face where sphere first hits
//       hit_time:  time [0,1] when (if) sphere hits the polygon.
//
// return: true if sphere hits face, false otherwise.
//---------------------------------------------------------------------------- 
bool
Face::collide_sphere(const sgVec3 c, const sgVec3 v, float radius, 
                     sgVec3 new_pos, sgVec3 hit_pos, float *hit_time)
{

    float dummy;
    if (hit_time == NULL) hit_time = &dummy;

    // calculate endpoint
    sgVec3 e;

//    sgCopyVec3(c, c_org);

    // calculate distance from sphere start and end to plane
    float dc = plane.dist_to_point(c);

//    printf("dc = %.3f\n", dc);
#if 0


    if (dc < radius) 
    {
        printf("wrong side! adjusting...\n");
        sgAddScaledVec3(new_pos, c, plane.n, radius - dc + 0.01);
        sgAddScaledVec3(hit_pos, new_pos, plane.n, -radius);
        *hit_time = 1.0;
        return true;
    }
#endif
    sgAddVec3(e, c, v);
    float de = plane.dist_to_point(e);

    // calculate distance from sphere endpoint to plane

    if ((dc * de > 0) && 
        (fabs(dc) > radius) &&
        (fabs(de) > radius)) return false;

    //if (dc == de) returna false;

    // distance sphere can move before touching the plane
    float t = (dc - radius) / (dc - de);

//    assert(t > 0.0);

//    printf("t = %.2f\n", t);
//    printf("vs[2]=%.8f\n", v[2]);

    if (t >= 0 && t <= 1) {
//    if (1) {

        // nc = center of just touching sphere
        sgAddScaledVec3(new_pos, c, v, t);
        
        // calculate point on the plane where sphere touches
        sgVec3 ip;
        sgAddScaledVec3(ip, new_pos, plane.n, -radius);
//    else sgAddScaledVec3(ip, new_pos, plane.n, -radius);
        
        if (point_inside(ip)) 
        {
//        assert(dc > 0);
            sgCopyVec3(hit_pos, ip);
            *hit_time = t;
//            printf("pip!\n");
            return true;
        }
    }
    // ok, sphere doesnt hit the interior of the polygon first...
    // check the edges

    bool touches = false;
    t = 1.0;
    for (unsigned i = 0; i < pindx.size(); i++) 
    {
        if (collide_edge_sphere(c, v, radius, vertices[pindx[i]],
                                vertices[pindx[(i + 1) % pindx.size()]],
                                t, hit_pos)) 
        {
            
//            printf("touched edge %d\n", i);
            *hit_time = t;
            touches = true;
        }
    }
                                                      
    if (touches) 
    {
        // nc = center of just touching sphere
        sgAddScaledVec3(new_pos, c, v, t);
        return true;
    }
    return false;
}


//  given an edge of a polygon and a moving sphere, find the first contact the sphere 
//  makes with the edge, if any.  note that hit_time must be primed with a  value of 1
//  before calling this function the first time.  it will then maintain the closest 
//  collision in subsequent calls.
//
// xs0:			start point (center) of sphere
// vs: 			path of sphere during frame
// rad:			radius of sphere
// v0:			vertex #1 of the edge
// v1:			vertex #2 of the edge
// hit_time:	(OUT) time at which sphere collides with polygon edge
// hit_point:	(OUT) point on edge that is hit
//
// returns - whether the edge (or it's vertex) was hit
//
// Note: this function has been adapted from code found at 
//       http://www.gdmag.com/src/aug01.zip

bool 
Face::collide_edge_sphere(const sgVec3 xs0, const sgVec3 vs, float rad, 
                          const sgVec3 v0, const sgVec3 v1, 
                          float &hit_time, sgVec3 hit_point)
{
    static sgVec3 temp_sphere_hit;
//    bool try_vertex = false; // Assume we don't have to try the vertices.
    
    sgVec3 ve, delta;
    float delta_dot_ve, delta_dot_vs, delta_sqr;
    float ve_dot_vs, ve_sqr, vs_sqr;
    
    sgSubVec3(ve, v1, v0);
    sgSubVec3(delta, xs0, v0);
    delta_dot_ve = sgScalarProductVec3(delta, ve); 
    delta_dot_vs = sgScalarProductVec3(delta, vs); 
    delta_sqr = sgLengthSquaredVec3(delta);
    
    ve_dot_vs = sgScalarProductVec3(ve, vs);
    ve_sqr = sgLengthSquaredVec3(ve); 
    vs_sqr = sgLengthSquaredVec3(vs); 
    
    float temp;
    sgVec3 tmp1;

    // position of the collision along the edge is given by: xe = v0 + ve*s, where s is
    //  in the range [0,1].  position of sphere along its path is given by: 
    //  xs = xs + vs*t, where t is in the range [0,1].  t is time, but s is arbitrary.
    //
    // solve simultaneous equations
    // (1) distance between edge and sphere center must be sphere radius
    // (2) line between sphere center and edge must be perpendicular to edge
    //
    // (1) (xe - xs)*(xe - xs) = rad*rad
    // (2) (xe - xs) * ve = 0
    //
    // then apply mathematica
    
    float A, B, C, root, discriminant;
    float root1 = 0.0f;
    float root2 = 0.0f;
    A = ve_dot_vs * ve_dot_vs - ve_sqr * vs_sqr;
    B = 2 * (delta_dot_ve * ve_dot_vs - delta_dot_vs * ve_sqr);
    C = delta_dot_ve * delta_dot_ve + rad * rad * ve_sqr - delta_sqr * ve_sqr;
    
    if( A > -0.0001f && A < 0.0001f ) 
    {
        // degenerate case, sphere is traveling parallel to edge
//        printf("degenerate!\n");
//        try_vertex = true;
    } 
    else 
    {
        discriminant = B*B - 4*A*C;
        if( discriminant > 0 ) {
            root = (float)sqrt(discriminant);
            root1 = (-B + root) / (2 * A);
            root2 = (-B - root) / (2 * A);
            
            // sort root1 and root2, use the earliest intersection.  the larger root 
            //  corresponds to the final contact of the sphere with the edge on its 
            //  way out.
            if( root2 < root1 ) 
            {
                temp = root1;
                root1 = root2;
                root2 = temp;
            }
            
            // root1 is a time, check that it's in our currently valid range
            if( (root1 < -0.1) || (root1 >= hit_time) ) 
            {
//            if( (root1 < -0.000) || (root1 >= hit_time) ) {
                goto check_vertex;
                return false;
            }
            
            // find sphere and edge positions

            //! temp_sphere_hit = xs0 + vs * root1;
            sgAddScaledVec3(temp_sphere_hit, xs0, vs, root1);
            
            // check if hit is between v0 and v1

            //! float s_edge = ((temp_sphere_hit - v0) * ve) / ve_sqr;
            sgSubVec3(tmp1, temp_sphere_hit, v0); 
            float s_edge = sgScalarProductVec3(tmp1, ve) / ve_sqr;
            if( (s_edge >= 0) && (s_edge <= 1) ) 
            {
		// bingo
                hit_time = root1;
                //!hit_point = v0 + ve * s_edge;
                sgAddScaledVec3(hit_point, v0, ve, s_edge);
                return true;
            }
        } 
        else 
        {
            // discriminant negative, sphere passed edge too far away
            goto check_vertex;
            return false;
        }
    }
    
 check_vertex:
    // sphere missed the edge, check for a collision with the first vertex.  note
    //  that we only need to check one vertex per call to check all vertices.

//    print_vector((float *)vs);
    A = vs_sqr;
    B = 2 * delta_dot_vs;
    C = delta_sqr - rad * rad;
    
    discriminant = B*B - 4*A*C;
//    printf("A=%.8f,  B=%.8f, C=%.8f, disc=%.9f\n", A, B, C, discriminant);
    if (discriminant > 0) 
    {
        root = (float)sqrt(discriminant);
        root1 = (-B + root) / (2 * A);
        root2 = (-B - root) / (2 * A);
        
        // sort the solutions
        if (root1 > root2) 
        {
            temp = root1;
            root1 = root2;
            root2 = temp;
        }
        
        // check hit vertex is valid and earlier than what we already have
        if( (root1 < 0) || (root1 >= hit_time) ) 
        {
//            printf("out here 1: root1=%.9f, hit_time=%.9f\n", root1, hit_time);
            return false;
        }
    } 
    else 
    {
        // discriminant negative, sphere misses vertex too
//        printf("out here 2\n");
        return false;
    }
    
    // bullseye
    hit_time = root1;
//    printf("hit_time = %.2f\n", hit_time);
    //!hit_point = v0;
    sgCopyVec3(hit_point, v0);
    return true;
}


// straigt implementation of algorithm on page 622 in Möller/Haines...
bool
Sphere::collide_sphere_sphere(Sphere &spb, const sgVec3 va, const sgVec3 vb, 
                              sgVec3 hit_point, float *hit_time)
{
    sgVec3 vab, l;

    sgSubVec3(vab, va, vb);
    sgSubVec3(l, center, spb.center); 

//    printf("testing (%.2f, %.2f, %.2f)->%.2f to (%.2f, %.2f, %.2f)->%.2f\n",
//           center[0], center[1], center[2], radius,
//           spb.center[0], spb.center[1], spb.center[2], spb.radius);

    float hit_dist = radius + spb.radius;

    float a = sgScalarProductVec3(vab, vab);
    float b = 2 * sgScalarProductVec3(l, vab);    
    float c = sgScalarProductVec3(l, l) - hit_dist * hit_dist;

    float disc = b*b - 4*a*c;
    if (disc < 0) return false;

    float sign_b = (b >= 0 ? 1.0f : -1.0f);

    float q = -0.5 * (b + sign_b * sqrt(disc));

    float t0 = q / a;
    float t1 = c / q;

    // find smallest
    if (t0 > t1) 
    {
        float temp = t1;
        t1 = t0;
        t0 = temp;
    }

//    printf("t0 = %.2f, t1 = %.2f\n", t0, t1);

    if (t0 >= 0.0 && t0 <= 1.0) 
    {
        // collision! calcluate point where spheres touch...
        sgAddScaledVec3(center, va, t0);
        sgAddScaledVec3(spb.center, vb, t0);

        sgVec3 c;
        sgSubVec3(c, spb.center, center);

        sgAddScaledVec3(hit_point, center, c, radius / hit_dist);

        *hit_time = t0;
        return true;
    }
    return false;
}
