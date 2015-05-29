
#include "first.h"
#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <plib/sg.h>

#include "globals.h"
#include "room.h"
#include "printer.h"

void
Room::init()
{
    unsigned i;
    srand(1234);

#if 0    
    // add some test objects

    for (i = 0; i < 30; i++) 
    {
        Object *obj = new Object;

        obj->set_name("test");
        obj->set_id(10 + i);

        obj->set_velocity(-0.3 + FRAND(0.6), -0.3 + FRAND(0.6), -0.3 + FRAND(0.6));
//        obj->set_velocity(0.0, -0.0, 0.0);
        obj->set_pos(globals.world->start_vp[0] + FRAND(10), 
                     globals.world->start_vp[1] + FRAND(10), 
                     globals.world->start_vp[2] + FRAND(20));
//        obj->set_pos(12280, -678, 12130);
        obj->reset_orientation();
        obj->mass = 10.0;

        objects.push_back(obj);
    }        

#if 0
    objects[0]->set_velocity(-0.08f, 0.0f, 0.05f);
    objects[0]->set_velocity(-0.00f, 0.0f, 0.00f);
    objects[0]->set_pos(globals.world->start_vp[0] - 10, 
                        globals.world->start_vp[1], 
                        globals.world->start_vp[2] - 220);
    objects[0]->mass = 10.0f;

    objects[1]->set_velocity(-0.04, 0.0, 0.0);
    objects[1]->set_pos(globals.world->start_vp[0] + 100, 
                        globals.world->start_vp[1], 
                        globals.world->start_vp[2] - 200);
    objects[1]->mass = 10.0;
#endif

#else
    
#endif
    for (i = 0; i < objects.size(); i++) 
    {

        objects[i]->set_velocity(-0.1 + FRAND(0.2), -0.1 + FRAND(0.2), 
                                 -0.1 + FRAND(0.2));

        if (id == 5) 
        {
            // room 5 - start room

            objects[0]->set_pos(globals.world->start_vp[0] + 10, 
                                globals.world->start_vp[1] + 60, 
                                globals.world->start_vp[2] - 200);

            objects[0]->set_velocity(0.0f, 0.0f, 0.1f);

            objects[3]->set_pos(globals.world->start_vp[0] + 60, 
                                globals.world->start_vp[1] + 70, 
                                globals.world->start_vp[2] + 200);

            objects[3]->set_velocity(0, 0, -0.08f);

            
        }

//        objects[i]->update_bsphere();

        objects[i]->in_room = this;     
        objects[i]->flags |= OF_LIGHTING | OF_BOUNCE;
        objects[i]->mass = 10.0;
    }

    traversed = false;
    collected_at_frame = -1;

    texture_disp_list = -1;
    light_disp_list = -1;

}

void
Room::setup_lights()
{
//    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, LightAmbient); 

    // set up light number 0.

    sgVec4 ambient, diffuse, specular, position;

    sgSetVec4(ambient, 0.0, 0.0, 0.0, 1.0);
    sgSetVec4(specular, 1.0, 1.0, 1.0, 1.0);

    assert(lights.size() <= 8);

    for (unsigned i = 0; i < lights.size(); i++) 
    {
        sgSetVec4(diffuse, lights[i].r, lights[i].g, lights[i].b, 1.0);
        
        sgSetVec4(position, lights[i].pos[0], lights[i].pos[1], 
                lights[i].pos[2], 1.0);
                
        glLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient); 
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse); 
        glLightfv(GL_LIGHT0 + i, GL_SPECULAR, specular);
        glLightfv(GL_LIGHT0 + i, GL_POSITION, position);

        glEnable(GL_LIGHT0 + i);

        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 0.005f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.000f);

    }
}

void
Room::update()
{
    sgVec3 yaxis, xaxis;
    sgSetVec3(yaxis, 0.0, 1.0, 0.0);
    sgSetVec3(xaxis, 1.0, 0.0, 0.0);

    float ticks = globals.ticks_per_frame;
    vector<Object *>::iterator obj_iter;

#if 1
    for (obj_iter = objects.begin(); obj_iter != objects.end(); obj_iter++) 
    {
        (*obj_iter)->rotate(0.1 * ticks, yaxis);
        (*obj_iter)->rotate(0.08 * ticks, xaxis);

        (*obj_iter)->update();

        if ((*obj_iter)->in_room != this) {
            // object has escaped! delete from object list.
            obj_iter = objects.erase(obj_iter);
            if (obj_iter == objects.end()) break;
        }
    }
#else

#if 1
    // collide objects with each other (only between objs in the same room)
    for (unsigned a = 0; a < objects.size(); a++) 
    {
        Sphere sphere_a = Sphere(objects[a]->pos, objects[a]->radius * 0.8);
        
        for (unsigned b = a + 1; b < objects.size(); b++) 
        {
            sgVec3 hit_point;
            float hit_time;

//            printf("testing object %d to object %d\n", a, b);

            Sphere sphere_b = Sphere(objects[b]->pos, objects[b]->radius * 0.8);

            // calculate movement
            sgVec3 move_a, move_b;
            sgScaleVec3(move_a, objects[a]->velocity, ticks);
            sgScaleVec3(move_b, objects[b]->velocity, ticks);

            if (sphere_a.collide_sphere_sphere(sphere_b, move_a, move_b,
                                               hit_point, &hit_time))
            {
#if 1
                objects[a]->bounce_objects(objects[b], move_a, move_b,
                                           hit_point, hit_time);

#else
                // collision normal
                sgVec3 cn;
                sgSubVec3(cn, hit_point, objects[a]->pos);
                sgNormalizeVec3(cn);

                float cn2 = sgScalarProductVec3(cn, cn);

                // move objects to point of collision (almost)
                sgAddScaledVec3(sphere_a.center, move_a, hit_time * 0.9999);
                sgAddScaledVec3(sphere_b.center, move_b, hit_time * 0.9999);

                // relative velocity
                sgVec3 vel_ab;
                sgSubVec3(vel_ab, objects[b]->velocity, objects[a]->velocity);

                // calculate impulse j
                float j, e = 0.8f;
                j = -1.0f * (1.0f + e) * sgScalarProductVec3(vel_ab, cn) / 
                    (cn2 * ((1.0f / objects[a]->mass) + (1.0f / objects[b]->mass)));

                
                // calculate new velocities
                sgAddScaledVec3(objects[a]->velocity, cn,
                                -1.0 * j / objects[a]->mass);
                                
                sgAddScaledVec3(objects[b]->velocity, cn,
                                1.0 * j / objects[b]->mass);

                // move with new velocities during remaining time

                sgAddScaledVec3(objects[a]->pos, sphere_a.center,
                                objects[a]->velocity, (1.0f - hit_time) * ticks);

                sgAddScaledVec3(objects[b]->pos, sphere_b.center,
                                objects[b]->velocity, (1.0f - hit_time) * ticks);

#endif
                printf("object %d hit object %d\n", a, b);
                // only consider one object/object collision each frame.
                // with very bad luck we may miss some collisions, but
                // that's not a catastrophe.
                break;
            }
        }
    }
#endif

    // move objects...

//    for (unsigned i = 0; i < objects.size(); i++) {
    for (obj_iter = objects.begin(); obj_iter != objects.end(); obj_iter++) 
    {
        (*obj_iter)->rotate(0.1 * ticks, axis);
        (*obj_iter)->update();

        if ((*obj_iter)->in_room != this) 
        {
            // object has escaped! delete from object list.
            obj_iter = objects.erase(obj_iter);
            if (obj_iter == objects.end()) break;
        }
    }
#endif

}
    


// recursively collect rooms and objects visible from current viewpoint.
void
Room::collect_and_recurse(sgVec3 vp, vector<Room *> &collected_rooms,
                          vector<Object *> &collected_objs,
                          int cur_depth, int max_depth)
{

    // pick up rooms
    if (collected_at_frame != globals.frame) 
    {
        collected_rooms.push_back(this);
    }

    // pick up objects
    for (unsigned i = 0; i < objects.size(); i++) 
    {
        collected_objs.push_back(objects[i]);
    }    

    if (cur_depth >= max_depth) return;

    traversed = true;

    // visit all connected rooms recursively
    for (unsigned p = 0; p < nr_portals; p++) 
    {
	assert(portals[p].face);

//        if (portals[p].croom->visited_at_frame == globals.frame) continue;
//        visited_at_frame = globals.frame;
        collected_at_frame = globals.frame;

        bool frontface = portals[p].face->backface_cull(vp);
        
	if (frontface &&
	    frustum->cull_face(portals[p].face)) 
        {
//	    printf("room %d: portal %d visible (%d faces)\n", 
//                   id, p, portals[p].face->pindx.size());

	    if (!portals[p].croom->traversed) 
            {
                frustum->push_portal(&portals[p], vp);
                portals[p].croom->collect_and_recurse(vp, 
                                                      collected_rooms,
                                                      collected_objs,
                                                      cur_depth + 1,
                                                      max_depth);
                frustum->pop_portal();
            }
	}
	else 
        {
//            printf("room %d: portal %d invisible: ", id, p);
//            if (!frontface) printf("backface\n");
//            else printf("frustum\n");
        }
    }
    traversed = false;
}



#if 0

// multitexturing... not used.
void
Room::draw()
{
    unsigned j;
    sgVec3 tmp;

//    return;
     
    float env_color[4] = {0.0, 0.0, 0.0, 0.0};

    glPushMatrix();

//    printf("drawing room %d\n", id);

    glDisable(GL_LIGHTING);

//    glMultMatrixf((float *)transf) ;

    glColor3f(1.0, 1.0, 1.0);
    int lm_unit;
#if 1
    if (globals.draw_textures) {
//        pglActiveTextureARB(GL_TEXTURE0_ARB);
        pglActiveTextureARB(GL_TEXTURE0_ARB);
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        lm_unit = GL_TEXTURE1_ARB;
    }
    else {
        lm_unit = GL_TEXTURE0_ARB;
    }    
#endif
    for (j = 0; j < nr_faces; j++) {
	
	Face *face = &faces[j];

	// dont draw portals faces
	if (face->flags & FF_PORTAL) continue;

//        if (face->check_at_frame != globals.frame) continue;

        // basic texture
#if 1
//        pglActiveTextureARB(GL_TEXTURE0_ARB);
        glActiveTextureARB(GL_TEXTURE0_ARB);
        if (face->texture != NULL) face->texture->bind();

        if (face->lmi_handle != -1 && globals.draw_lightmaps) {

            // lightmap texture
            glActiveTextureARB(lm_unit);
            switch(globals.lm_mode) {
            case 1: 
                glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE); break;
            case 2: 
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB); 

                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
/*
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

                // Arg1 = texture
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

*/
                glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
                break;


            case 3:
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB); 

                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);
                glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env_color);	 

                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_CONSTANT_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

                // Arg1 = texture
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

                // Arg2 = -lightmap
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_CONSTANT_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_COLOR);
//                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
//                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_ONE_MINUS_SRC_COLOR);

                glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1);
                break;

            case 4: 
                glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB); 
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);

                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);

                // Arg1 = texture
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

                glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2);
                break;

            }
            globals.world->lm_textures[
                globals.world->lightmaps[face->lmi_handle].lm_tex].bind();
            glEnable(GL_TEXTURE_2D);

        }


	if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
	else if (face->pindx.size() == 4) glBegin(GL_QUADS);
	else glBegin(GL_POLYGON);
	
//	printf("drawing face %d\n   ", j);

        unsigned c;
	for (c = 0; c < face->pindx.size(); c++) {
            
	    if (face->flags & FF_NORMALS) {
            glNormal3fv(normals[face->nindx[c]]);
	    }

	    if (face->flags & FF_TEXTURED && globals.draw_textures) {
//                pglMultiTexCoord2fvARB(GL_TEXTURE0_ARB, face->uv[c]);
                glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, face->uv[c]);
	    }

            if (face->lmi_handle != -1 && globals.draw_lightmaps) {
//                pglMultiTexCoord2fvARB(lm_unit, face->luv[c]);
                glMultiTexCoord2fvARB(lm_unit, face->luv[c]);
            }

//	    glColor3f(1.0, 1.0, 1.0);
	    sgScaleVec3(tmp, vertices[face->pindx[c]], 1.0);
//	    printf("(%.2f, %.2f", vertices[face->pindx[c]][0],
//		   vertices[face->pindx[c]][1], vertices[face->pindx[c]][2]);
	    glVertex3fv(tmp);	    
	}
//	printf("\n");
	glEnd();
#if 0
        glDisable(GL_DEPTH_TEST);
//        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_LINE_STRIP);
	for (c = 0; c < face->pindx.size(); c++) {
	
	    glColor3f(1.0, 1.0, 1.0);
	    glVertex3fv(vertices[face->pindx[c]]);
        }
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
#endif
#endif
    }
    glPopMatrix();

    glActiveTextureARB(lm_unit);
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glDisable(GL_TEXTURE_2D);
}
#endif


void
Room::draw_lightmaps()
{
    unsigned j;

//    float env_color[4] = {0.0, 0.0, 0.0, 0.0};

    glPushMatrix();

    if (light_disp_list == -1) {

        light_disp_list = glGenLists(1);

        glNewList(light_disp_list, GL_COMPILE);

        glDisable(GL_LIGHTING);
        glDisable(GL_BLEND);
        
        glColor3f(1.0, 1.0, 1.0);
        
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glEnable(GL_TEXTURE_2D);
        
        for (j = 0; j < mesh->nr_faces; j++) {
            
            Face *face = &mesh->faces[j];
            
            // dont draw portals faces
            if (face->flags & FF_PORTAL) continue;
            
            if (face->lmi_handle == -1) continue;

            globals.world->lm_textures[
                globals.world->lightmaps[face->lmi_handle].lm_tex].bind();

//        printf("drawing lightmap %d\n", j);        
            if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
            else if (face->pindx.size() == 4) glBegin(GL_QUADS);
            else glBegin(GL_POLYGON);
            
            unsigned c;
            for (c = 0; c < face->pindx.size(); c++) {
                
                if (face->flags & FF_TEXTURED && globals.draw_textures) {
//            glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, face->luv[c]);
                    glTexCoord2fv(face->luv[c]);
                }
                
                glVertex3fv(mesh->vertices[face->pindx[c]]);	    
            }
            glEnd();
            
        }
        glEndList();
    }

    glCallList(light_disp_list);

    glPopMatrix();
        
//    glDisable(GL_TEXTURE_2D);
}




void
Room::draw_textures()
{
    int j;

//    float env_color[4] = {0.0, 0.0, 0.0, 0.0};

    glPushMatrix();

    if (texture_disp_list == -1) 
    {

        texture_disp_list = glGenLists(1);

        glNewList(texture_disp_list, GL_COMPILE);

        glDisable(GL_LIGHTING);
        
        glColor4f(1.0, 1.0, 1.0, 0.5);
        
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
//        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendFunc(GL_DST_COLOR, GL_SRC_ALPHA);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0, -4.0);

        // draw unlit faces...
        for (j = mesh->nr_faces - 1; j >= 0; j--) {         
            Face *face = &mesh->faces[j];            
            if (face->lmi_handle >= 0) continue; 

            // dont draw portals faces
            if (face->flags & FF_PORTAL) continue;         
            if (face->texture != NULL) face->texture->bind();
            
            if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
            else if (face->pindx.size() == 4) glBegin(GL_QUADS);
            else glBegin(GL_POLYGON);
            
            unsigned c; 
            for (c = 0; c < face->pindx.size(); c++) {
                
                if (face->flags & FF_TEXTURED && globals.draw_textures) {
                    pglMultiTexCoord2fvARB(GL_TEXTURE0_ARB, face->uv[c]);
                }
                
                glVertex3fv(mesh->vertices[face->pindx[c]]);	    
            }
            glEnd();            
        }
     
        glEnable(GL_BLEND);

        glColor4f(1.0, 1.0, 1.0, 0.5);        
//    glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
//    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_DST_COLOR);
//    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR);        
        glBlendFunc(GL_DST_COLOR, GL_SRC_ALPHA);
        
        for (j = 0; j < (int)mesh->nr_faces; j++) {
            
            Face *face = &mesh->faces[j];
            
            if (face->lmi_handle == -1) continue; 

            // dont draw portals faces
            if (face->flags & FF_PORTAL) continue;
            
            if (face->texture != NULL) face->texture->bind();
            
            if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
            else if (face->pindx.size() == 4) glBegin(GL_QUADS);
            else glBegin(GL_POLYGON);
            
            unsigned c; 
            for (c = 0; c < face->pindx.size(); c++) {
                
                if (face->flags & FF_TEXTURED && globals.draw_textures) {
                    pglMultiTexCoord2fvARB(GL_TEXTURE0_ARB, face->uv[c]);
                }
                
                glVertex3fv(mesh->vertices[face->pindx[c]]);	    
            }
            glEnd();
            
        }
        
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_BLEND);

        glEndList();
    }
    
    glCallList(texture_disp_list);

    glPopMatrix();    
}


// check if we have crossed a portal and return it's 
// destination room if we have...
Room *
Room::check_portals(sgVec3 old_vp, sgVec3 new_vp)
{
    sgVec3 dir;
    sgSubVec3(dir, new_vp, old_vp);
    sgNormalizeVec3(dir);

    for (unsigned p = 0; p < nr_portals; p++) {
//    for (unsigned p = 0; p < 1; p++) {
//	croom->portals[p]->plane.print();
//	printf("vp = (%f, %f, %f)\n", vp[0], vp[1], vp[2]);
        Face &face = *portals[p].face;

        float dist = face.plane.dist_to_point(new_vp);

//        printf("nr_verts = %d\n", cur_room->nr_verts);

//	if (!cur_room->portals[p].face->backface_cull(vp)) continue;
//	printf("portal %d: dist = %f\n", p, dist);

	if (dist < 0) {

//            printf("dir = (%.2f, %.2f, %.2f)\n", dir[0], dir[1], dir[2]);

            if (face.intersects_ray(old_vp, dir, NULL)) {
//                printf("ray intersects!\n");

                return portals[p].croom;
            }
//            printf("ray doesn't intersect...\n");
        }
    }
    return NULL;
}


void
Room::delete_object(Object *obj)
{
    vector<Object *>::iterator obj_iter;

    for (obj_iter = objects.begin(); obj_iter != objects.end(); obj_iter++) 
    {
        if (*obj_iter == obj)
        {
            objects.erase(obj_iter);
            return;
        }
    }
}

void
Room::add_object(Object *obj)
{
    objects.push_back(obj);
}
