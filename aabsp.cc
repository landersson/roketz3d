/*----------------------------------------------------------------------------

 Roketz3D - aabsp.cpp

 Axis Aligned BSP tree and collision detection/response code.

----------------------------------------------------------------------------*/


#include "first.h"
#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <plib/sg.h>

#include "globals.h"
#include "world.h"
#include "billboard.h"
#include "printer.h"


#define VERY_CLOSE 0.001f

#if 0

// "collide and slide"... not working prefectly yet
void 
Room::collide_and_slide(sgVec3 pos, sgVec3 vel, float radius, 
                        sgVec3 new_pos, int depth, int last_slided_face)
{
    int face_indx;
    sgVec3 ball_hit_pos, plane_hit_point;

    float min_hit_time = 10.0, hit_time;
                
    // find closest colliding face                
    bool ball_collided = false;
    for (unsigned f = 0; f < nr_faces; f++) {
                    
//        if (f == last_slided_face) continue;
                    
        sgVec3 tmp_hit_pos, tmp_plane_hit;
        bool tmp_collide = 
            faces[f].collide_sphere(pos, vel, radius, 
                                    tmp_hit_pos, tmp_plane_hit, 
                                    &hit_time);                
        
        if (tmp_collide) {           
//            printf("collided with face %d\n", f);
            if (hit_time < min_hit_time) {
                sgCopyVec3(ball_hit_pos, tmp_hit_pos);
                sgCopyVec3(plane_hit_point, tmp_plane_hit);
                min_hit_time = hit_time;
//                printf("face %d closest so far\n", f);
                ball_collided = true;
                face_indx = f;
            }                    
        }                 
    }
    
    hit_time = min_hit_time;
                
    if (!ball_collided) {

        // no collision, just move along...
//        printf("no collision, returning...");
        sgAddVec3(new_pos, pos, vel);
        return;
    }

    float speed = sgLengthVec3(vel);
    float hit_distance = hit_time * speed;

//    printf("hit distance =                     %.5f\n", hit_distance);
//    printf("ball_hit_pos = "); print_vector(ball_hit_pos);
    
#if 1
    if (hit_distance < VERY_CLOSE) {
        // too close, freeze!
//        printf("freeze!\n");
//        sgAddScaledVec3(new_pos, ball_hit_pos, vel, -VERY_CLOSE*1.05 / speed);
        // calculate vector from plane hit point to sphere center
#if 1
        sgVec3 tmp;
        sgSubVec3(tmp, ball_hit_pos, plane_hit_point);
        sgNormalizeVec3(tmp);

        sgAddScaledVec3(new_pos, ball_hit_pos, vel, -1.05 * VERY_CLOSE / speed);
#endif
//        sgCopyVec3(new_pos, pos);

        return;
    }
#endif    

    sgVec3 new_base_point, slide_normal, dst_pos, new_dst_pos, tmp;

    // calculate new base point, a little bit off the plane
//    sgAddScaledVec3(new_base_point, pos, vel, 
//                    hit_time - (VERY_CLOSE * 1.0) / speed);

    sgAddScaledVec3(new_base_point, ball_hit_pos, vel, 
                    -1.00 * VERY_CLOSE / speed);
//    sgCopyVec3(new_base_point, ball_hit_pos);

//    printf("pos = "); print_vector(pos);

//    printf("new_base_point = "); print_vector(new_base_point);

    // adjust plane hit point
    sgAddScaledVec3(plane_hit_point, vel, -1.0 * VERY_CLOSE / speed);

//    printf("plane_hit_point = "); print_vector(plane_hit_point);

    // calculate vector from plane hit point to sphere center
    sgSubVec3(slide_normal, new_base_point, plane_hit_point);
    sgNormalizeVec3(slide_normal);

    // original destination point
    sgAddVec3(dst_pos, pos, vel);

//    printf("vel = "); print_vector(vel);

    // calculate new destination point
//    sgSubVec3(tmp, dst_pos, plane_hit_point);
    sgSubVec3(tmp, dst_pos, new_base_point);
    float f = sgScalarProductVec3(tmp, slide_normal);
    sgAddScaledVec3(new_dst_pos, dst_pos, slide_normal, -f);

//    printf("new_dst_pos = "); print_vector(new_dst_pos);
//    printf("\n\n");

    // calculate new velocity
    sgVec3 new_vel;
    sgSubVec3(new_vel, new_dst_pos, new_base_point);

    // stop if new velocity too small
     if (sgLengthVec3(new_vel) < VERY_CLOSE || depth > 50) {
//        assert(depth <= 5);
        sgCopyVec3(new_pos, new_base_point);
        return;
    }
    
    collide_and_slide(new_base_point, new_vel, radius, new_pos, depth + 1, face_indx);
}
#endif



// just check if a moving sphere hits any wall polygons 
bool 
AABSP::collide(const sgVec3 pos, const sgVec3 velocity, 
               float radius, float ticks,  
               sgVec3 hit_point)
{
    bool collision = false;

    // calculate step
    sgVec3 move;
    sgScaleVec3(move, velocity, ticks);

    // find possible collision candidates
    sgVec3 side_lengths;
    float side_len = radius + sgLengthVec3(move);    
    sgSetVec3(side_lengths, side_len, side_len, side_len);
    vector<vector<Face *> *> faces_in_box;

    globals.world->aabsp.collect_touching_faces(
        pos, side_lengths,
        faces_in_box);

    float min_hit_time = 10.0, hit_time;
        
    // find closest colliding face                
    unsigned i, j;
    for (i = 0; i < faces_in_box.size(); i++) 
    {    
        for (j = 0; j < faces_in_box[i]->size(); j++) 
        {
            Face *f = (*faces_in_box[i])[j];

            if (f->flags & FF_PORTAL) continue;
                
            sgVec3 tmp_hit_pos, tmp_plane_hit;
            bool tmp_collide = 
                f->collide_sphere(pos, move, radius, 
                                  tmp_hit_pos, tmp_plane_hit, 
                                  &hit_time);                
            
            if (tmp_collide) 
            {           
                if (hit_time < min_hit_time) 
                {
//                    sgCopyVec3(hit_pos, tmp_hit_pos);
                    sgCopyVec3(hit_point, tmp_plane_hit);
                    min_hit_time = hit_time;
                    
                    collision = true;
                }                    
            }
        }
    }
    return collision;
}
    

// check wall collisions and use elastic bounce response
bool 
AABSP::collide_and_bounce(const sgVec3 pos, const sgVec3 vel, float radius, float ticks,  
                          sgVec3 new_pos, sgVec3 new_vel)
{
    Face *last_face_bounced = NULL;
    sgVec3 ball_hit_pos, plane_hit_point;            
    sgVec3 ball_move, old_pos, old_vel;

    sgCopyVec3(old_pos, pos);
    sgCopyVec3(old_vel, vel);
    sgCopyVec3(new_vel, vel);
    sgScaleVec3(ball_move, new_vel, ticks);

    bool collision, ball_collided = false;

    // find possible collision candidates
    sgVec3 side_lengths;
    float side_len = radius + sgLengthVec3(ball_move);    
    sgSetVec3(side_lengths, side_len, side_len, side_len);

    vector<vector<Face *> *> faces_in_box;

    globals.world->aabsp.collect_touching_faces(
        pos, side_lengths,
        faces_in_box);

    int repeats = 0;

//    printf("------ collide and bounce ------ frame = %ld\n\n", globals.frame);

    do {

//        float length = sgLengthVec3(old_vel);
//        printf("vel = %.5f\n", length);
//        if (sgLengthVec3(vel) < 0.0001) return ball_collided;
        
        if (repeats++ > 10) 
        {
            //! return true;
            break;
        }

        float min_hit_time = 10.0, hit_time;
        
        // find closest colliding face                
        collision = false;
//        for (unsigned f = 0; f < nr_faces; f++) {

        unsigned i, j;
        int total = 0;
        for (i = 0; i < faces_in_box.size(); i++) 
        {            
            total += faces_in_box.size();

            for (j = 0; j < faces_in_box[i]->size(); j++) 
            {
                Face *f = (*faces_in_box[i])[j];

                if (f->flags & FF_PORTAL) continue;
                
                f->check_at_frame = (int)globals.frame;
                
                if (f == last_face_bounced) continue;
                
//            if (!faces[f].backface_cull(new_pos)) continue;
                
                sgVec3 tmp_hit_pos, tmp_plane_hit;
                bool tmp_collide = 
                    f->collide_sphere(old_pos, ball_move, radius, 
                                      tmp_hit_pos, tmp_plane_hit, 
                                      &hit_time);                
                
                if (tmp_collide) 
                {           
//                printf("collided with face %d\n", f);
                    if (hit_time < min_hit_time) 
                    {
                        sgCopyVec3(ball_hit_pos, tmp_hit_pos);
                        sgCopyVec3(plane_hit_point, tmp_plane_hit);
                        min_hit_time = hit_time;
//                    printf("face %d closest so far (hit time = %.2f)\n", 
//                           f, hit_time);
//                    printf("hit_point = ");
//                    print_vector(plane_hit_point);
                        collision = true;
                        last_face_bounced = f;
                    }                    
                }
            }
        }                 
//        printf("colliding with %d\n", total);
        
        hit_time = min_hit_time;
        
        if (collision) 
        {        
            ball_collided = true;
    
            // calculate vector from collision point to sphere center
            sgVec3 n, old_pos;
            sgSubVec3(n, ball_hit_pos, plane_hit_point);
            sgNormalizeVec3(n);
            
//            last_face_bounced = face_indx;                    
            
//                        printf("bouncing at face %d: ball_hit_pos = ", last_face_bounced);
//                        print_vector(ball_hit_pos);                  
            
            // perfectly elastic bounce, mirror velocity along 
            // collision plane normal
            float f = sgScalarProductVec3(n, old_vel);
            sgAddScaledVec3(new_vel, old_vel, n, -2.000*f);

            // move away from plane a little bit...
            sgAddScaledVec3(ball_hit_pos, old_vel, -0.001f);

            sgCopyVec3(old_vel, new_vel);

            sgAddScaledVec3(new_pos, ball_hit_pos, 
                            new_vel, (1.000 - hit_time) * ticks); // * ticks !!!!
            
//                        printf("                  new_speed = ");
//                        print_vector(balls[0].speed);    
            
//                        printf("                    new_pos = ");
//                        print_vector(balls[0].pos);    
                    
            // update movement
            sgCopyVec3(old_pos, ball_hit_pos);
            sgSubVec3(ball_move, new_pos, ball_hit_pos);
        }        
    } while (collision);
    
    // update position
    sgAddVec3(new_pos, old_pos, ball_move);

    return ball_collided;
}


void
AABSP::create_face_tree(vector<Room*> &rooms, unsigned nr_faces)
{
    // array of pointers to all faces in the world :)
//    faces = (Face **)malloc(nr_faces * sizeof(Face *));

    face_tree_root = new FTreeNode; 

    // workspace...
    face_tree_root->faces = vector<Face *>(nr_faces, NULL);
        //(Face **)malloc(nr_faces * sizeof(Face *));

    sgVec3 min, max;
    sgSetVec3(min, 1.0 * 1e10, 1.0 * 1e10, 1.0 * 1e10);
    sgSetVec3(max, -1.0 * 1e10, -1.0 * 1e10, -1.0 * 1e10);

    // find world AABB
    int face_nr = 0;
    for (unsigned r = 0; r < rooms.size(); r++) 
    {
        Room &room = *(rooms[r]);

        // loop through all room vertices
        for (unsigned v = 0; v < room.mesh->nr_verts; v++) 
        {
            min[0] = MIN(min[0], room.mesh->vertices[v][0]);
            max[0] = MAX(max[0], room.mesh->vertices[v][0]);

            min[1] = MIN(min[1], room.mesh->vertices[v][1]);
            max[1] = MAX(max[1], room.mesh->vertices[v][1]);

            min[2] = MIN(min[2], room.mesh->vertices[v][2]);
            max[2] = MAX(max[2], room.mesh->vertices[v][2]);     
        }
        // collect all faces
        for (unsigned f = 0; f < room.mesh->nr_faces; f++) 
        {           
            face_tree_root->faces[face_nr++] = &(room.mesh->faces[f]);            
        }
    }        

    printf("World AABB: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)\n",
           min[0], min[1], min[2], max[0], max[1], max[2]);

    // start splitting boxes in two!
    int total_faces = split_box(face_tree_root, min, max, 0);

    printf("total faces collected: %d\n", total_faces);

}


int
AABSP::split_box(FTreeNode *node, const sgVec3 min, 
                 const sgVec3 max, int depth)
{

    float dx = max[0] - min[0];
    float dy = max[1] - min[1];
    float dz = max[2] - min[2];

    // stop splitting if AABB volume or number of triangles too small
    if ((dx*dy*dz < 10000.0) || (node->faces.size() < 16) || depth > 100) 
    {
//        printf("new leaf: volume=%.2f, faces=%d\n", 
//               dx*dy*dz, node->faces.size());

        node->is_leaf = true;
        return node->faces.size();
    }
    else 
    {
        node->is_leaf = false;
    }

    // find major axis of box...
    int major;

    if (dx > dy) 
    {
        if (dz > dx) major = 2;
        else major = 0;
    }
    else 
    {
        if (dz > dy) major = 2;
        else major = 1;
    }

    vector<Face *> &faces = node->faces;


    unsigned f, v;
#if 0
    // calculate average vertex pos on major axis
    int nr_vertices = 0;
    double sum = 0;

    for (f = 0; f < faces.size(); f++) 
    {    
        for (v = 0; v < faces[f]->pindx.size(); v++) 
        {     
            sum += faces[f]->vertices[faces[f]->pindx[v]][major];
        }
        nr_vertices += faces[f]->pindx.size();
    }
    node->split_pos = sum / nr_vertices;
#endif
    node->major = major;

    // just split at the middle...
    node->split_pos = (min[major] + max[major]) / 2.0f;

    // find out on what side the triangles are and add them to the subtrees

    // add triangles to positive and negative subtrees...
    node->neg_branch = new FTreeNode();
    node->pos_branch = new FTreeNode();

    for (f = 0; f < faces.size(); f++) 
    {    
        bool neg = false;
        bool pos = false;
        for (v = 0; v < faces[f]->pindx.size(); v++) {
     
            if (faces[f]->vertices[faces[f]->pindx[v]][major] >= node->split_pos) 
                pos = true;
            else 
                neg = true;
        }

        // a triangle may be part of both trees
        if (pos) node->pos_branch->faces.push_back(faces[f]);
        if (neg) node->neg_branch->faces.push_back(faces[f]);
    }
#if 0
    printf("---------------------------------\n");
    printf("   box volume: %.3f\n", dx*dy*dz);
    printf("   split pos : %.3f, major=%d\n", node->split_pos, major);
    printf("   nr neg faces: %d\n", node->neg_branch->faces.size());
    printf("   nr pos faces: %d\n", node->pos_branch->faces.size());

    printf("   AABB: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)\n",
           min[0], min[1], min[2], max[0], max[1], max[2]);
#endif
    // dont need these anymore... all stored in child nodes.
    node->faces.clear();

    // create new AABB's
    sgVec3 neg_max, pos_min;

    sgCopyVec3(neg_max, max);
    sgCopyVec3(pos_min, min);

    neg_max[major] = node->split_pos;
    pos_min[major] = node->split_pos;

    // and split them recursively...
    return (split_box(node->neg_branch, min, neg_max, depth + 1) +
            split_box(node->pos_branch, pos_min, max, depth + 1));
}



#if 1
void
AABSP::collect_touching_faces(const sgVec3 center, 
                       const sgVec3 side_lengths, 
                       vector<vector<Face *> *> &intervals)
{
    collect_touching_faces_rek(face_tree_root, center, 
                               side_lengths, intervals);
}
#endif

// find all faces touching given AABB
// 
// TODO: fix problem with duplicates!
void
AABSP::collect_touching_faces_rek(FTreeNode *node, const sgVec3 center, 
                                  const sgVec3 side_lengths, 
                                  vector<vector<Face *> *> &intervals)
{
    if (node->is_leaf) 
    {        
        // we found a leaf... push face array intervals
//        intervals.push_back(node->first);
//        intervals.push_back(node->last);
        intervals.push_back(&node->faces);
    }    
    else 
    {
#if 1
        FTreeNode *other_side;

        // first traverse the half in which box center is located
        if (center[node->major] >= node->split_pos)
        {
            collect_touching_faces_rek(node->pos_branch, center, 
                                       side_lengths, intervals);
            other_side = node->neg_branch;
        }
        else 
        {
            collect_touching_faces_rek(node->neg_branch, center, 
                                       side_lengths, intervals);
            other_side = node->pos_branch;
        }
        // if box crosses over to the other side then traverse it too.
        if (fabs(center[node->major] - node->split_pos) <= 
            side_lengths[node->major])
        {
            collect_touching_faces_rek(other_side, center, 
                                       side_lengths, intervals);
        }
#else
        // collect all faces, just for testing...
        collect_touching_faces_rek(node->neg_branch, center, 
                               side_lengths, intervals);
        collect_touching_faces_rek(node->pos_branch, center, 
                               side_lengths, intervals);
#endif
    }
}
