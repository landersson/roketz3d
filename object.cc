
#include "first.h"
#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <plib/sg.h>

#include "globals.h"
#include "object.h"
#include "geometry.h"
#include "printer.h"

static Room *current_room_lights = NULL;
static long lights_valid_frame = 0;

// rescale object
void
Mesh::scale(float scale)
{
    for (unsigned i = 0; i < nr_verts; i++) 
    {
	vertices[i][0] *= scale;
	vertices[i][1] *= scale;
	vertices[i][2] *= scale;
    }
    for (unsigned f = 0; f < nr_faces; f++) 
    {
	faces[f].update_plane();
    }
}

// create bounding sphere using Ritters algorithm, RR p. 
// returns object radius
float
Mesh::update_bsphere()
{
    unsigned i;
    int p1, p2;
    int minx, miny, minz, maxx, maxy, maxz;
    float dx, dy, dz;
    sgVec3 v, center;

    minx = maxx = miny = maxy = minz = maxz = 0;

    // find min/max in each direction
    for (i = 1; i < nr_verts; i++) 
    {
	if (vertices[i][0] < vertices[minx][0]) minx = i;
	if (vertices[i][0] > vertices[maxx][0]) maxx = i;
	if (vertices[i][1] < vertices[miny][1]) miny = i;
	if (vertices[i][1] > vertices[maxy][1]) maxy = i;
	if (vertices[i][2] < vertices[minz][2]) minz = i;
	if (vertices[i][2] > vertices[maxz][2]) maxz = i;
    }

    dx = sgDistanceSquaredVec3(vertices[minx], vertices[maxx]);
    dy = sgDistanceSquaredVec3(vertices[miny], vertices[maxy]);
    dz = sgDistanceSquaredVec3(vertices[minz], vertices[maxz]);

    p1 = p2 = 0;
    if (dx >= dy && dx >= dz) { p1 = minx; p2 = maxx; }
    if (dy >= dx && dy >= dz) { p1 = miny; p2 = maxy; }
    if (dz >= dx && dz >= dy) { p1 = minz; p2 = maxz; }

    sgAddVec3(center, vertices[p1], vertices[p2]);
    sgScaleVec3(center, 0.5);

    float radius2 = sgDistanceSquaredVec3(center, vertices[p1]);
    radius = sqrt(radius2);

//    printf("new sphere: (%f, %f, %f)->%f\n",
//	   center[0], center[1], center[2], radius);
    
    for (i = 0; i < nr_verts; i++) 
    {
	float d2 = sgDistanceSquaredVec3(center, vertices[i]);
	if (d2 > radius2) {
	    float d = sqrt(d2);
	    sgSubVec3(v, vertices[i], center);
	    sgAddScaledVec3(center, v, (d - radius) / (2 * d));
	    radius = 0.5 * (radius + d);
	    radius2 = radius * radius;
//	    printf("new sphere: (%f, %f, %f)->%f\n",
//		   center[0], center[1], center[2], radius);
	}
    }

    // put object origin at sphere center and check that all
    // vertices are really inside the sphere
    for (i = 0; i < nr_verts; i++) 
    {
        sgSubVec3(vertices[i], center);
	float d2 = sgLengthSquaredVec3(vertices[i]);
	if (d2 > radius2) 
        {
            printf("%.4f > %.4f\n", d2, radius2);
//	    assert(0);
	}
    }

    // copy values into bounding sphere class
//    bsphere.radius = radius;
//    sgCopyVec3(bsphere.center, center);

//    printf("bounding sphere: (%f, %f, %f)->%f\n",
//	   center[0], center[1], center[2], radius);

    return radius;
}

Object::Object()
{ 
    mesh = NULL; 
    in_room = NULL;
    last_object_hit = NULL;

    mass = 10.0f;
    flags = 0;   
    age = 0;
    collided_at_frame = -1;
    moved_at_frame = -1;
    name[0] = 0;

    sgMakeIdentMat4(orient);
}


void 
Object::rotate(float angle, sgVec3 axis)
{ 
    sgMat4 r; 
    sgMakeRotMat4(r, angle, axis);
    sgPostMultMat4(orient, r);
}


bool
Object::check_wall_collisions(sgVec3 hit_point)
{
    return globals.world->aabsp.collide(
        pos, velocity, 20.0, globals.ticks_per_frame,
        hit_point);
}

void
Object::update()
{
    age += (int)globals.ticks_per_frame;

    sgCopyVec3(old_pos, pos);
//    sgAddScaledVec3(pos, velocity, globals.ticks_per_frame);

//    printf("object %d update...\n", id);
            
#if 0
    globals.world->aabsp.collide_and_bounce(
        pos, velocity, 20.0, globals.ticks_per_frame,
        new_pos, new_vel);

    sgCopyVec3(pos, new_pos);
    sgCopyVec3(velocity, new_vel);
#else
    // dont collide with ourselves...
    collided_at_frame = globals.frame; 

    vector<Object *> &objects = in_room->objects;

    Sphere sphere_a = Sphere(this->pos, this->mesh->radius * 0.9);

    // collide with all non collided objects in this room...
    bool object_hit = false;
    for (unsigned b = 0; b < objects.size(); b++) 
    {
        if (!(flags & OF_ALWAYS_COLLIDE))
        {
            if (objects[b]->collided_at_frame == globals.frame) continue;
        }
        sgVec3 hit_point;
        float hit_time;

        Sphere sphere_b = Sphere(objects[b]->pos, objects[b]->mesh->radius * 0.9);
            
        // calculate movement
        sgVec3 move_a, move_b;
        sgScaleVec3(move_a, this->velocity, globals.ticks_per_frame);
        sgScaleVec3(move_b, objects[b]->velocity, globals.ticks_per_frame);
        
        if (sphere_a.collide_sphere_sphere(sphere_b, move_a, move_b,
                                           hit_point, &hit_time))
        {
            
            this->bounce_objects(objects[b], move_a, move_b,
                                 hit_point, hit_time);
            object_hit = true;

            last_object_hit = objects[b];

            printf("object collision at frame %ld!\n", globals.frame);
        }
    }
    if (!object_hit) 
    {
#if 0
        
        // no object/object collision... just check room walls.
        globals.world->aabsp.collide_and_bounce(
            pos, velocity, 20.0, globals.ticks_per_frame,
            new_pos, new_vel);
        
        sgCopyVec3(pos, new_pos);
        sgCopyVec3(velocity, new_vel);
#else
        sgVec3 move_vec;
        sgScaleVec3(move_vec, velocity, globals.ticks_per_frame);
        move(move_vec, globals.ticks_per_frame);

        last_object_hit = NULL;
#endif
    }
#endif
    if (pos[0] < -100000.0 || pos[0] > 100000.0) 
    {
        printf("doh!\n");
        assert(0);
    }
    // make sure objects dont more too fast...
    if (sgLengthVec3(velocity) > 0.1) 
    {
        sgScaleVec3(velocity, 0.999f);
    }

    // add some gravity
//    velocity[1] -= 0.0005 * globals.ticks_per_frame;
    
    // check if object has passed through any portals
    Room *new_room = in_room->check_portals(old_pos, pos);
    
    if (new_room != NULL) 
    {
        // switch rooms...
        in_room = new_room;
        new_room->objects.push_back(this);
    }    
}


// calculate the collision response between two colliding spheres given 
// the hit point and hit time of the collision.
void
Object::bounce_objects(Object *obj_b, sgVec3 move_a, sgVec3 move_b,
                       sgVec3 hit_point, float hit_time)
{
    Object *obj_a = this;
    float move_time;

    // collision normal
    sgVec3 cn;
    sgSubVec3(cn, hit_point, obj_a->pos);
    sgNormalizeVec3(cn);

    float cn2 = sgScalarProductVec3(cn, cn);
    
    
    // move objects to point of collision (almost)
#if 0
    // just move... objects may escape the room!
    sgAddScaledVec3(obj_a->pos, move_a, hit_time * 0.9999);
    sgAddScaledVec3(obj_b->pos, move_b, hit_time * 0.9999);
#else
    // check every movement against room walls
    bool wall_hit = false;
    move_time = hit_time * 0.9999;
    sgScaleVec3(move_a, move_time);
    sgScaleVec3(move_b, move_time);

    wall_hit |= obj_a->move(move_a, move_time * globals.ticks_per_frame);
    wall_hit |= obj_b->move(move_b, move_time * globals.ticks_per_frame);

    if (wall_hit) 
    {
        // one of the objects hit a wall before colliding... just return
        return;
    }
#endif

    // relative velocity
    sgVec3 vel_ab;
    sgSubVec3(vel_ab, obj_b->velocity, obj_a->velocity);

    // calculate impulse j
    float j, e = 1.0f;
    j = -1.0f * (1.0f + e) * sgScalarProductVec3(vel_ab, cn) / 
        (cn2 * ((1.0f / obj_a->mass) + (1.0f / obj_b->mass)));
    
    
    // calculate new velocities
    sgAddScaledVec3(obj_a->velocity, cn,
                    -1.0 * j / obj_a->mass);
    
    sgAddScaledVec3(obj_b->velocity, cn,
                    1.0 * j / obj_b->mass);
    
    // move with new velocities during remaining time
#if 0    
    sgAddScaledVec3(obj_a->pos, obj_a->pos,
                    obj_a->velocity, (1.0f - hit_time) * globals.ticks_per_frame);
    
    sgAddScaledVec3(obj_b->pos, obj_b->pos,
                    obj_b->velocity, (1.0f - hit_time) * globals.ticks_per_frame);
#else
    // move and check wall collisions...
    move_time = (1.0f - hit_time) * globals.ticks_per_frame;
    sgScaleVec3(move_a, obj_a->velocity, move_time);
    sgScaleVec3(move_b, obj_b->velocity, move_time);

    obj_a->move(move_a, move_time);
    obj_b->move(move_b, move_time);
#endif
    
//    printf("object %d hit object %d\n", a, b);
}


// move object while checking collision with walls.    
bool
Object::move(sgVec3 move, float time)
{
    if (time == 0.0) return false;

    if (flags & OF_BOUNCE) 
    {
        sgVec3 new_pos, new_velocity;

        sgVec3 tmp_velocity;
        sgScaleVec3(tmp_velocity, move, 1.0f / time);
        
        bool wall_hit = globals.world->aabsp.collide_and_bounce(
        pos, tmp_velocity, 20.0, time, //globals.ticks_per_frame,
        new_pos, new_velocity);
        
        sgCopyVec3(pos, new_pos);
        sgCopyVec3(velocity, new_velocity);    
        
        return wall_hit;
    }
    else 
    {
        sgAddVec3(pos, move);
        return false;
    }
}

void 
Object::draw()
{
    unsigned j;
    sgMat4 transf;

    if (mesh == NULL) return;
      
//    printf("drawing object %d... pos=%.2f, %.2f, \n", id);

    // cull object to current frustum
    if (!globals.frustum->cull_sphere(pos, mesh->radius)) return;

    // check lighting...
    if (lights_valid_frame != globals.frame) 
    {
        // new frame, static lights need to be repositioned...
        current_room_lights = NULL;
        lights_valid_frame = globals.frame;
    }

    if (in_room != current_room_lights) 
    {
        // update static lights
        current_room_lights = in_room;
        in_room->setup_lights();
    }

    // setup object transformation
    sgMakeTransMat4(transf, pos);
    sgPreMultMat4(transf, orient);

    glPushMatrix();
    glMultMatrixf((float *)transf) ;

    // call display list or create it...
    if (mesh->gl_disp_list == -1) 
    {
        // create display list
        mesh->gl_disp_list = glGenLists(1);
        glNewList(mesh->gl_disp_list, GL_COMPILE);
         
        if (flags & OF_LIGHTING) glEnable(GL_LIGHTING);
        else glDisable(GL_LIGHTING);

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);

        int cur_material = -1;
        for (j = 0; j < mesh->nr_faces; j++) 
        {           
            Face *face = &mesh->faces[j];
            
            // dont draw portals faces
            if (face->flags & FF_PORTAL) continue;
            
            // check if we need to switch materials
            if (face->material != cur_material) 
            {
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, 
                             globals.world->materials[face->material].diffuse);
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, 
                             globals.world->materials[face->material].specular);
                glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 
                            globals.world->materials[face->material].shininess);

                printf("material = %d\n", face->material);
                cur_material = face->material;
            }

            if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
            else if (face->pindx.size() == 4) glBegin(GL_QUADS);
            else glBegin(GL_POLYGON);
            
//	printf("drawing face %d\n   ", j);
            
            for (unsigned c = 0; c < face->pindx.size(); c++) 
            {                
                if (face->flags & FF_NORMALS) 
                {
                    assert(mesh->normals);
                    glNormal3fv(mesh->normals[face->nindx[c]]);
                }
                
//                glColor3f(1.0, 1.0, 1.0);
                glVertex3fv(mesh->vertices[face->pindx[c]]);	    
            }
//	printf("\n");
            glEnd();
        }       
        glEndList();  
    }
    glCallList(mesh->gl_disp_list);

    glPopMatrix();
}


void 
Object::draw_shot()
{
    unsigned j;
    sgMat4 transf;

    if (mesh == NULL) return;
      
//    printf("drawing object %d... pos=%.2f, %.2f, \n", id);

    // cull object to current frustum
    if (!globals.frustum->cull_sphere(pos, mesh->radius)) return;

    // check lighting...
    if (lights_valid_frame != globals.frame) 
    {
        // new frame, static lights need to be repositioned...
        current_room_lights = NULL;
        lights_valid_frame = globals.frame;
    }

    if (in_room != current_room_lights) 
    {
        // update static lights
        current_room_lights = in_room;
        in_room->setup_lights();
    }

    // setup object transformation
    sgMakeTransMat4(transf, pos);
    sgPreMultMat4(transf, orient);

    glPushMatrix();
    glMultMatrixf((float *)transf) ;

    // call display list or create it...
//    mesh->gl_disp_list = -1;
    if (mesh->gl_disp_list == -1) 
    {
        // create display list
        mesh->gl_disp_list = glGenLists(1);
        glNewList(mesh->gl_disp_list, GL_COMPILE);
         
        glDisable(GL_LIGHTING);       
        glDisable(GL_TEXTURE_2D);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDepthMask(0);

        float scale[5] = {5.0f, 3.0f, 2.0f, 1.5f, 1.0f};
        float alpha[5] = {0.2f, 0.4f, 0.55f, 0.7f, 1.0f};

        for (unsigned i = 0; i < 5; i++) 
        {
#if 1
            for (j = 0; j < mesh->nr_faces; j++) 
            {                
                Face *face = &mesh->faces[j];
                                
                float r = i * 0.25;
                float g = i * 0.33;
//                float b = 1.0;

                if (g > 1.0) g = 1.0;

                glColor4f(r, g, 1.0, alpha[i]);

                // basic texture
                
                if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
                else if (face->pindx.size() == 4) glBegin(GL_QUADS);
                else glBegin(GL_POLYGON);
                
                for (unsigned c = 0; c < face->pindx.size(); c++) 
                {
                    sgVec3 tmp;
                    if (face->flags & FF_NORMALS) {
                        assert(mesh->normals);
                        glNormal3fv(mesh->normals[face->nindx[c]]);
                    }
                    sgScaleVec3(tmp, mesh->vertices[face->pindx[c]], scale[i]);	     
                    glVertex3fv(tmp);
                }
                glEnd();
            }
        }
#else

        glColor4f(0.0, 0.8, 1.0, 0.6);

        for (j = 0; j < mesh->nr_faces; j++) 
        {            
            Face *face = &mesh->faces[j];
            
            // dont draw portals faces
            if (face->flags & FF_PORTAL) continue;
            
            // basic texture
            
            if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
            else if (face->pindx.size() == 4) glBegin(GL_QUADS);
            else glBegin(GL_POLYGON);
            
            for (unsigned c = 0; c < face->pindx.size(); c++) 
            {
                sgVec3 tmp;
                if (face->flags & FF_NORMALS) {
                    assert(mesh->normals);
                    glNormal3fv(mesh->normals[face->nindx[c]]);
                }
                sgScaleVec3(tmp, mesh->vertices[face->pindx[c]], 1.5);	     
                glVertex3fv(tmp);
            }
            glEnd();
        }




        glColor4f(1.0, 1.0, 1.0, 0.9);

        for (j = 0; j < mesh->nr_faces; j++) 
        {            
            Face *face = &mesh->faces[j];
            
            // dont draw portals faces
            if (face->flags & FF_PORTAL) continue;
            
            // basic texture
            
            if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
            else if (face->pindx.size() == 4) glBegin(GL_QUADS);
            else glBegin(GL_POLYGON);
            
            for (unsigned c = 0; c < face->pindx.size(); c++) 
            {
                sgVec3 tmp;
                if (face->flags & FF_NORMALS) {
                    assert(mesh->normals);
                    glNormal3fv(mesh->normals[face->nindx[c]]);
                }
                sgScaleVec3(tmp, mesh->vertices[face->pindx[c]], 1.0);	     
                glVertex3fv(tmp);
            }
            glEnd();
        }
#endif
        glDepthMask(1);
        glEndList();  
    }
    glCallList(mesh->gl_disp_list);

    glPopMatrix();
}



void 
DynamicLight::light_environment()
{
    sgVec3 side_lengths;

    if (!enabled) return;

    float light_intensity = intensity;

    float light_intensity2 = light_intensity * light_intensity; 

    sgSetVec3(side_lengths, light_intensity, 
              light_intensity, light_intensity);

    vector<vector<Face *> *> faces_in_box;
    faces_in_box.clear();

    globals.world->aabsp.collect_touching_faces(
        pos, side_lengths,
        faces_in_box);
    
    glDepthMask(0);
    glDisable(GL_LIGHTING);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
//    glDisable(GL_TEXTURE_2D);
    globals.lightbulb_tex.bind();

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); 

    float border_color[4] = {0.0, 0.0, 0.0, 0.0};

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glBlendEquation(GL_FUNC_ADD_EXT);
    glPolygonOffset(0, -2.0);

    
    unsigned i, j, polys_lit = 0;
    for (i = 0; i < faces_in_box.size(); i++) 
    {
        for (j = 0; j < faces_in_box[i]->size(); j++) 
        {
            unsigned c;
            sgVec3 vs, vt, tmp;

            Face *face = (*faces_in_box[i])[j];
    
            // dont draw portals faces
            if (face->flags & FF_PORTAL ||
                face->lit_by_obj == (id + globals.frame)) continue;

            // KLUDGE: the face list may contain dublicates... that's why
            // this check is here: FIX face list!
            face->lit_by_obj = id + globals.frame;

            // check if this face needs to be considered at all
            bool face_affected = false;
            for (c = 0; c < face->pindx.size(); c++) 
            {
                if (sgDistanceSquaredVec3(pos, face->vertices[face->pindx[c]]) <=
                    light_intensity2)
                {
                    face_affected = true;
                    break;
                }
            } 
            
            // calculate closest point to light source on face
            sgVec3 proj;
            float dist = face->plane.project_point(pos, proj);        

            if (dist < light_intensity) face_affected = true;
            if (!face_affected) continue;

//            if (dist <= 0) continue;
            float dist2 = dist * dist;

            sgSubVec3(tmp, pos, face->vertices[face->pindx[0]]);
            sgNormalizeVec3(tmp);
            //float scale2 = sgScalarProductVec3(tmp, face->plane.n);


            sgSubVec3(vs, face->vertices[face->pindx[0]], proj);
            sgNormalizeVec3(vs);

            sgVectorProductVec3(vt, face->plane.n, vs);
            sgNegateVec3(vt);
//            sgNormalizeVec3(vt);                

            float smu = light_intensity2 - dist2;
            
            if (smu < 1000.0) continue;

            float scale = 0.5 / sqrt(smu);

            if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
            else if (face->pindx.size() == 4) glBegin(GL_QUADS);
            else glBegin(GL_POLYGON);

            float inten = 1.0f - dist2 / light_intensity2;

            if (inten <= 0.0) continue;

//            inten = 1.0;

//            glColor4f(color[0], color[1], color[2], inten*0.4);
            glColor4f(color[0], color[1], color[2], inten*0.6);

            
//            printf("inten = %.4f\n", inten);

            polys_lit++;

            for (c = 0; c < face->pindx.size(); c++) 
            {
                sgSubVec3(tmp, face->vertices[face->pindx[c]], proj);
                float s = sgScalarProductVec3(tmp, vs) * scale + 0.5f;
                float t = sgScalarProductVec3(tmp, vt) * scale + 0.5f;

                glTexCoord2f(s, t);	    

//                glVertex3fv(proj);
                glVertex3fv(face->vertices[face->pindx[c]]);	    
            }
            glEnd();            
        }
    }
//    printf("%d polys lit\n", polys_lit);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_BLEND);
    glDepthMask(1);
    glEnable(GL_DEPTH_TEST);
}
