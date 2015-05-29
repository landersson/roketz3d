
#include "first.h"
#include <GL/gl.h>
#include <GL/glu.h>

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include <plib/sg.h>

#include "object.h"
#include "rocket.h"
#include "input.h"
#include "billboard.h"
#include "printer.h"

#include "globals.h"

#define ROCKET_SIZE	4.0
#define GRAVITY		1.0

extern int game_state;

const char *digits[7] = {"6", "5", "4", "3", "2", "1", "0"};

char *tex_data = new char[128 * 128 * 3];

// returns random float between 0.0 and 1.0
float 
frand()
{
    return (float)(rand()) / RAND_MAX;
}


void 
Rocket::load_sound_effects()
{
            
    flame_snd = Mix_LoadWAV("data/audio/thrust.wav");
    missile_snd = Mix_LoadWAV("data/audio/laser.wav");
    countdown_snd = Mix_LoadWAV("data/audio/ignition2.wav");
    explo_snd = Mix_LoadWAV("data/audio/explosion.wav");
    liftoff_snd = Mix_LoadWAV("data/audio/liftoff.wav");
    eagle_snd = Mix_LoadWAV("data/audio/eagle.wav");

    assert(missile_snd && flame_snd && countdown_snd && explo_snd);

    missile_channel = -1;
    flame_channel = -1;
    explo_channel = -1;

    flame_channel = Mix_PlayChannel(-1, flame_snd, -1);
    Mix_Pause(flame_channel);

    music = Mix_LoadMUS("data/audio/roketz.mod");
    assert(music);
    Mix_PlayMusic(music, -1);
    int prev_volume = Mix_VolumeMusic(128);
    printf("prev volume = %d\n", prev_volume);
}


void 
Rocket::init()
{
    flags = OF_LIGHTING;
    
    countdown = 0;
    lives = 4;
    points = 0;

    use_cubemap = true;
    first_liftoff = true;


    // index into missile light array to use next
    next_missile_light = 0;

    // initialize position and orientation
//    sgSetVec3(pos, 0.0f, 30.0f, 0.0f);
    
    sgMakeIdentQuat(rot_quat);
    sgSetVec3(dir, 0.0, 0.0, 1.0);
    sgMakeIdentMat4(rot);

    // set up initial rotation matrix...
    setup_orient(0, 0);
    sgCopyMat4(prev_cam_orient, globals.cam->orient_inv);

    // load rocket jet particle system texture
    jet.set_texture("data/gfx/particle.bmp");

    // load explosion particle system texture
    explo.set_texture("data/gfx/particle.bmp");

    // load missile mesh
    missile_mesh = Mesh::create_from_OBJ_file("data/objects/shot2.obj");
    assert(missile_mesh);
    missile_mesh->scale(1.0f);
    missile_mesh->update_bsphere();

    // load rocket mesh
    mesh = Mesh::create_from_OBJ_file("data/objects/rocket2.obj");    
    assert(mesh);
    mesh->scale(5);
//    mesh->update_bsphere();

    // setup dynamic flame light
    flame_light.id = 100;
    flame_light.enabled = true;
    flame_light.intensity = 200.0f;
    sgSetVec3(flame_light.color, 1.0f, 0.3f, 0.0f);
    globals.world->add_dynamic_light(&flame_light);

    // setup blue missile lights
    for (int i = 0; i < 16; i++) 
    {
        missile_lights[i].id = 1000 + 2*i;
        missile_lights[i].enabled = false;
        missile_lights[i].intensity = 128.0f;
        sgSetVec3(missile_lights[i].color, 0.2f, 0.2f, 1.0f);
        globals.world->add_dynamic_light(&missile_lights[i]);
    }
    
    explo_light.id = 1000 + 64;
    explo_light.enabled = false;
    globals.world->add_dynamic_light(&explo_light);

    // setup jet flare billboard
    jet_flare.add_image("data/gfx/flare.tga");
    jet_flare.set_color(0.5, 0.5, 0.5, 1.0);
    jet_flare.set_size(40.0);
    jet_flare.depth_test = true;

    // setup asteroid explosion billboard
    asteroid_exp1.add_images("data/gfx/explosion3/ExplosionG", 3, ".tga");
    asteroid_exp1.set_color(1.0, 1.0, 1.0, 0.5);
//    asteroid_exp1.set_blend(GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR);
    asteroid_exp1.set_fps(24.0);
    asteroid_exp1.set_size(300.0);
    asteroid_exp1.one_shot = true;
    asteroid_exp1.active = false;

    load_sound_effects();

    Mix_PlayChannel(-1, countdown_snd, 0);

    setup_cubemaps();
    prepare_for_takeoff();
}


void 
Rocket::prepare_for_takeoff()
{
    thrust = false;
    on_base = true;
    aim_mode = false;
    refueling = false;
    new_flame = 0.0f;

    launch_ticks = 0.0f;
    digit_ticks = 0.0f;
    on_base_ticks = 0.0f;

    fuel = 100.0;
    shield = 100.0;

    sgCopyVec3(pos, globals.world->start_vp);
    pos[1] += 1.0;
    sgSetVec3(velocity, 0.000f, 0.0f, 0.00f);

    globals.cam->set_pos(10629.0, 85.0, 10670.0);

    // move to starting room
    globals.world->cur_room = globals.world->start_room;
    in_room = globals.world->start_room;
}

void
Rocket::launch_missile(const sgVec3 direction)
{
    sgVec3 start_pos, vel;
    
    Object *missile = new Object;
    
    missile->mesh = missile_mesh;

    sgCopyVec3(start_pos, pos);
    sgAddScaledVec3(start_pos, pos, dir, 20.0);
    sgAddScaledVec3(vel, velocity, dir, 1.0);

    missile->set_pos(start_pos);
    missile->set_velocity(vel);
    
    missile->set_orientation(orient);
    missile->in_room = in_room;
    missile->mass = 0.1f;
    missile->flags |= OF_ALWAYS_COLLIDE;

    // setup dynamic missile light
    missile->light_id = next_missile_light;
    missile_lights[next_missile_light].enabled = true;
    sgCopyVec3(missile_lights[next_missile_light].pos, start_pos);
    next_missile_light++;
    if (next_missile_light > 15) next_missile_light = 0;

    missiles.push_back(missile);

    missile_channel = Mix_PlayChannel(missile_channel, missile_snd, 0);
    printf("pang!\n");
}

void 
copy_asteroid_properties(Object *dst, Object *src)
{
    dst->set_pos(src->pos);
    dst->set_velocity(src->velocity);

    dst->in_room = src->in_room;
    dst->flags = src->flags;
    dst->type = src->type;
}

void 
Rocket::asteroid_hit(Object *ast)
{
    // delete original object
    if (ast->age < 500) return;
    
    ast->in_room->delete_object(ast);

    if (!(ast->flags & OF_EXPLODED))
    {
        ast->flags |= OF_EXPLODED;

        // not exploded before, split up in two pieces
        Object *new1 = new Object;
        Object *new2 = new Object;
        
        // random asteroid meshes
        int mesh1 = 4 + rand() % 3;
        int mesh2 = 4 + rand() % 3;
 
        new1->mesh = globals.world->asteroid[mesh1];
        new2->mesh = globals.world->asteroid[mesh2];

        new1->mass = 5.0f;
        new2->mass = 5.0f;

        // copy positions, velocities, flags etc.
        copy_asteroid_properties(new1, ast);
        copy_asteroid_properties(new2, ast);

        // random separation velocity vector
        sgVec3 exp_vel;
        exp_vel[0] = frand();
        exp_vel[1] = frand();
        exp_vel[2] = frand();

        sgNormalizeVec3(exp_vel);
        sgScaleVec3(exp_vel, 0.5);
        sgAddVec3(new1->velocity, exp_vel);
        sgSubVec3(new2->velocity, exp_vel);

        // add two new asteroids to the room's object list
        ast->in_room->add_object(new1);
        ast->in_room->add_object(new2);
    }

    // create explosion billboard
    asteroid_exp1.set_center(ast->pos);
    asteroid_exp1.active = true;

    // set up explosion particle system
    explo.setup_explosion(ast->pos);

    // setup dynamic explosion light
    sgCopyVec3(explo_light.pos, ast->pos);
    sgSetVec3(explo_light.color, 1.0, 0.5, 0.0);
    explo_light.intensity = 500.0f;
    explo_light.enabled = true;
    
    // play explosion sound
    explo_channel = Mix_PlayChannel(explo_channel, explo_snd, 0);
    Mix_Volume(explo_channel, MIX_MAX_VOLUME / 2);

    points += 100;
}



void 
Rocket::draw()
{
    sgMat4 tmatrix, tmp;

    if (aim_mode) {
        sgMakeIdentMat4(tmatrix);
    }
    else {
        // make rocket always point forward (inverse camera rotation)
//            sgCopyMat4(tmatrix, globals.cam->orient_inv);
        sgCopyMat4(prev_cam_orient, globals.cam->orient_inv);
    }

    if (!on_base) 
    {
#if 1        

//        sgCopyMat4(tmatrix, globals.cam->orient_inv);
#endif
        // local thrust orientation
        sgCopyMat4(tmatrix, prev_cam_orient);
        sgQuatToMatrix(tmp, rot_quat) ;
        sgPreMultMat4(tmatrix, tmp);
    }
    else {
        sgVec3 axis;
        sgSetVec3(axis, 1.0, 0.0, 0.0);
        sgMakeRotMat4(tmatrix, 90.0f, axis);
//        sgMakeIdentMat4(tmatrix);
        
    }

    if (game_state != GS_DYING && game_state != GS_GAMEOVER)
    {
        set_orientation(tmatrix);
        Object::draw();

#if 0
        sgVec3 shiftx, shifty;
        sgCopyVec3(shiftx, globals.cam->e1);
        sgScaleVec3(shiftx, 2.0);
#endif
        if (use_cubemap)
        {
#if 1            
            glViewport(-1, 0, globals.win_width - 1, globals.win_height);
            draw_env();
            glViewport(0, -1, globals.win_width, globals.win_height - 1);
            draw_env();
            glViewport(+1, 0, globals.win_width + 1, globals.win_height);
            draw_env();
            glViewport(0, +1, globals.win_width, globals.win_height +1);
            draw_env();
            glViewport(0, 0, globals.win_width, globals.win_height);
#else
            draw_env();            
#endif
        }
    }

    // draw jet particle system
    explo.draw_explo();
//    explo.update(3);
//    explo.draw_explo();
    explo.update(globals.ticks_per_frame);


    jet.draw();

    if (thrust) jet_flare.draw();

    // update missiles
    vector<Object *>::iterator m;
    for (m = missiles.begin(); m != missiles.end(); m++)
    {
        sgVec3 hit_point;
        if ((*m)->check_wall_collisions(hit_point) ||
            sgDistanceVec3(pos, (*m)->pos) > 10000.0)
        {
            missile_lights[(*m)->light_id].enabled = false;
            m = missiles.erase(m);
            if (m == missiles.end()) break;
        }
        (*m)->update();

        // update dynamic light pos
        sgCopyVec3(missile_lights[(*m)->light_id].pos, (*m)->pos);

        if ((*m)->last_object_hit != NULL) 
        {
            // ok, this missile did hit an object
            //     if ((*m)->last_object_hit->is_
//            (*m)->last_object_hit->in_room->delete_object((*m)->last_object_hit);
            asteroid_hit((*m)->last_object_hit);
        }
    }

    // draw missiles
    for (m = missiles.begin(); m != missiles.end(); m++)
    {
        (*m)->draw_shot();
    }

//    printf("%d\n", missiles.size());

    asteroid_exp1.draw();

    // draw countdown
    if (game_state == GS_LAUNCHING)
    {
        launch_ticks += globals.ticks_per_frame;

        if (launch_ticks > 1500) {

            digit_ticks += globals.ticks_per_frame;

            if (digit_ticks > 1000.0f) {
                digit_ticks = 10.0;
                countdown++;
            }

            if (countdown > 6) {
                game_state = GS_PLAYING;
            }
            else
            {
                float z = 1010.0f - digit_ticks;
                
                float size = 1000.0 / z;

                float w, h;
                get_letter_size(digits[countdown][0], size, &w, &h);
                
//                float offs = size * 22.0 / 128.0f / 1.0;
//                float offs = size * 22.0 / 128.0f * globals.win_width;

                print_string(globals.win_width / 2 - w / 2,
                             globals.win_height / 2 - h / 2,
                             size, digits[countdown]);
                
            }
        }
    }
    
    // draw statistics
    char strbuf[32];
//    sprintf(strbuf, "Fuel: %d", (int)fuel);
//    print_string(8, 4, 1.5f, strbuf);    

//    sprintf(strbuf, "Shield: %d", (int)shield);
//    print_string(160, 4, 1.5f, strbuf);    

    sprintf(strbuf, "%05d", points);
    print_string(globals.win_width / 2 - 65, globals.win_height - 47, 
                 2.3f, strbuf);    


    if (refueling && (fuel < 97.0 || shield < 100.0))
    {
        print_string(globals.win_width / 2 - 85,
                     globals.win_height / 1.5, 1.5f, "REFUELING...");    

//        printf("%.2f\n", pos[1]);
    }
    
}


//void orient_rocket(rocket_t *r, float d_yaw, float d_pitch)
void 
orient_rocket(void *_r, int x, int y)
{
    Rocket *r = (Rocket *)_r;
    r->setup_orient(x, y);
}


void 
Rocket::setup_orient(int x, int y)
{
//    if (on_landing_spot) return;
    
    float cx = (float)globals.win_width / 2.0;
    float cy = (float)globals.win_height / 2.0;

    float d_yaw = ((float)x - cx) / cx;
    float d_pitch = (cy - (float)y) / cy;

//    printf("ox=%f, oy=%f\n", d_yaw, d_pitch);

    if (on_base) {
        d_yaw = 0.0f;
        d_pitch = 0.5f;
    }

    sgVec3 yaxis, xaxis;
    sgMat4 rot;
    sgQuat tmpq;

#if 1
    sgMakeIdentQuat(rot_quat);    

    // rotate x axis
    sgSetVec3(yaxis, 0.0, 1.0, 0.0);
    // rotate around transformed x axis
    sgRotQuat(tmpq, d_pitch * 180, yaxis ) ;
    sgQuatToMatrix(rot, rot_quat) ;
    sgXformVec3(yaxis, rot);


    // rotate around y axis
    sgRotQuat(rot_quat, d_yaw * -180, yaxis) ;

    // rotate x axis
    sgSetVec3(xaxis, 1.0, 0.0, 0.0);
    sgQuatToMatrix(rot, rot_quat ) ;
    sgXformVec3(xaxis, rot);

    // rotate around transformed x axis
    sgRotQuat(rot_quat, d_pitch * 180, xaxis ) ;
#endif
//    sgSetVec3(yaxis, 0.0, 1.0, 0.0);
//    sgRotQuat(rot_quat, d_yaw * -180, yaxis) ;
//    sgSetVec3(yaxis, 1.0, 0.0, 0.0);
//    sgRotQuat(rot_quat, d_pitch, yaxis ) ;
}

void 
Rocket::update(float ticks, Camera *camera)
{
    sgVec3 sp;
    sgMat4 rot;

    float dist_to_base = sgDistanceVec3(pos, globals.world->start_vp);
//    printf("dtb = %.2f\n", dist_to_base);

    asteroid_exp1.update(globals.ticks_per_frame);
    jet.update_smoke(ticks);
//    explo.update(ticks);
//    explo.update_explosion();

    if (game_state == GS_GAMEOVER)
    {
        return;
    }

    if (game_state == GS_DYING) 
    {
        dead_ticks += ticks;
        if (dead_ticks > 2000.0f) 
        {
            game_state = GS_PLAYING;
            prepare_for_takeoff();
        }
        return;        
    }

    if (dist_to_base < 25.0f) {
        on_base = true;
        if (pos[1] < -459.9f && sgLengthVec3(velocity) < 0.01f)
        {
            on_base_ticks += ticks;
                         
            if (on_base_ticks > 1000) {
             
                if (refueling == false) 
                {
                    refueling = true;                    
                    if (!first_liftoff) Mix_PlayChannel(-1, eagle_snd, 0);
                }

                fuel += 0.01 * ticks;
                if (fuel > 100.0) fuel = 100.0;
                
                if (shield < 100.0) shield += 0.01 * ticks;
                else shield = 100.0;
            }
        }
    }
    else {
        if (on_base) 
        {
            // leaving base, warp pointer to position representing "up"
            SDL_WarpMouse(globals.win_width / 2, globals.win_height / 4);            
            on_base = false;
            if (first_liftoff)
            {
//                Mix_SetMusicPosition(4);
                Mix_SetMusicPosition(25);
                first_liftoff = false;
            }
            if (refueling) {
                Mix_PlayChannel(-1, liftoff_snd, 0);
                refueling = false;
            }
            on_base_ticks = 0.0f;
        }
    }

    if (dist_to_base < 150.0f) {
        // probably trying to land... use fixed camera position.
        sgCopyVec3(globals.cam->wanted_pos, globals.world->start_vp);
        globals.cam->wanted_pos[0] += 200.0;
        globals.cam->wanted_pos[1] += 60.0;
    }


    //
    if (thrust) {
//    if (false)    {
        flame_light.enabled = true;
        flame_light.intensity = 300 + 30 * frand();
    }
    else {
        if (flame_light.intensity < 10.0) {
            flame_light.enabled = false;
        } 
        else {
            flame_light.intensity *= pow(0.997, ticks);
        }
    }


//    static int last_ticks;

    if (key_down(SDLK_SPACE) && game_state == GS_PLAYING) thrust = 1; 
    else thrust = 0; 

//    printf("ticks = %f\n", ticks);

//    ticks *= 0.001f;
   
    float speed = sgLengthVec3(velocity);
//    if (sgLengthVec3(velocity) > 0.1) {

    // save previous camera position position
    sgCopyVec3(camera->previous_pos, camera->current_pos);

    // sp vector from current pos to wanted pos (how to move camera
    sgSubVec3(sp, camera->current_pos, camera->wanted_pos);
    // 0.05: how fast camera follows wanted pos after rocket
    float cam_follow_speed = 0.05f;
//    float cam_follow_speed = 0.55;
    if (aim_mode) cam_follow_speed += 0.5;
    sgScaleVec3(sp, 0.008 * ticks * (cam_follow_speed + speed)); 
    
    // move towards wanted camera position
    sgSubVec3(camera->current_pos, sp);

//    printf("%d, %.1f\n", last_ticks, ticks);

    // untransformed direction (on bsase, straight up)
    sgSetVec3(dir, 0.0, 0.0, -1.0);
    sgQuatToMatrix(rot, rot_quat) ;

//    if (!aim_mode) {
//        sgPostMultMat4(rot, camera->orient_inv) ;
        sgPostMultMat4(rot, prev_cam_orient) ;
//    }
//    sgScaleVec3(sp, velocity, ticks);
//    sgAddVec3(pos, sp);

#if 1
    // perform collision detection...
    sgVec3 new_pos, new_vel;
    bool bounce = globals.world->aabsp.collide_and_bounce(
        pos, velocity, 20.0, ticks, //globals.ticks_per_frame,
        new_pos, new_vel);

    Room *new_room = in_room->check_portals(pos, new_pos);
    
    if (new_room != NULL) {
        // switch rooms...
        in_room = new_room;
    }

    sgCopyVec3(pos, new_pos);
    sgCopyVec3(velocity, new_vel);

    if (bounce) 
    {
        // rocket bounced into a wall, 50% elastic bounce
        sgScaleVec3(velocity, 0.5);
        
        if (!on_base) {
            float damage = sgLengthSquaredVec3(velocity);

            // make sure rocket explodes if it's out of fuel...
            if (fuel <= 0.0f) damage += 0.0026f;
            shield -= 768.0f * damage;
        }
        if (shield < 0.0 && game_state != GS_DYING)
        {
            // doh! we're toasted...
            shield = 0.0;
            dead_ticks = 0.0;

            lives--;

            if (lives == 0) 
            {
                game_state = GS_GAMEOVER;
                Mix_HaltChannel(-1);   
            }
            else 
            {
                game_state = GS_DYING;
            }
                
            // create explosion billboard
            asteroid_exp1.set_center(pos);
            asteroid_exp1.active = true;
            
            explo.setup_explosion(pos);
            
            // play explosion sound
            explo_channel = Mix_PlayChannel(explo_channel, explo_snd, 0);
            Mix_Volume(explo_channel, MIX_MAX_VOLUME / 2);
            
            sgZeroVec3(velocity);
        }
        
    }

    if (pos[1] < -1800) 
    {
	pos[1] = -1800.0;
	velocity[1] = 0.0;
    }
    sgCopyVec3(flame_light.pos, pos);
#endif

    // dir = direction that rocket points in
    sgXformVec3(dir, rot);

//    print_vector(dir);

    // update jet flare position
    sgVec3 flare_pos;
    sgAddScaledVec3(flare_pos, pos, dir, -ROCKET_SIZE * 3.2f);
    jet_flare.set_center(flare_pos);

    if (thrust && fuel > 0.0) 
    {
        fuel -= 0.0014 * ticks;        
        if (fuel < 0.0) fuel = 0.0;

#if 1
        // thrust sound
        if (flame_channel == -1) {
            flame_channel = Mix_PlayChannel(-1, flame_snd, -1);
        }
        else Mix_Resume(flame_channel);
#endif        

        // accelerate in the forward direction
        velocity[0] += 0.00020 * dir[0] * ticks;
        velocity[1] += 0.00020 * dir[1] * ticks;
        velocity[2] += 0.00020 * dir[2] * ticks;
        
        new_flame += ticks * 2;
        while (new_flame > 4.0) {
            
            sgVec3 ppos, pvel;
            
            sgAddScaledVec3(ppos, pos, dir, -ROCKET_SIZE * 3.0f);
            sgAddScaledVec3(pvel, velocity, dir, -0.4f);
            
            pvel[0] += -0.04 + 0.08 * frand();
            pvel[1] += -0.04 + 0.08 * frand();
            pvel[2] += -0.04 + 0.08 * frand();
            
            sgAddScaledVec3(ppos, pvel, new_flame - 4.0);
            
            jet.add_particle(ppos, pvel, 2600 + 400*frand());
            
            new_flame -= 2.0;
        }
        
    }
    else 
    {
        Mix_Pause(flame_channel);
    }

#if 1
    // gravity
    velocity[1] -= 0.00008 * ticks * GRAVITY;

    // fake air resistance
    double air_resistance = 1.0;
    air_resistance = pow(0.9999, ticks / 2);
    velocity[0] *= air_resistance;
    velocity[1] *= air_resistance;
    velocity[2] *= air_resistance;
#endif

    // launch missiles
    if (mouse_button_down(LEFT_MOUSE_BUTTON)) 
    {
        if ((globals.ticks - last_launch_tick) > 100) 
        {
            last_launch_tick = globals.ticks;
            launch_missile(dir);
        }
    }

    if (key_down(SDLK_LSHIFT)) {
        aim_mode = true;
    }
    else {
        aim_mode = false;
    }

    if (explo_light.enabled)
    {
        if (explo_light.color[1] > 0.0) 
        {
//            explo_light.color[1] -= 0.002 * ticks;
            if (explo_light.color[1] < 0.0)
                explo_light.color[1] = 0.0f;
        }
        if (explo_light.color[1] < 0.5) 
        {
//            explo_light.color[0] -= 0.001 * ticks;
            if (explo_light.color[0] < 0.0)
                explo_light.color[0] = 0.0;
        }
        explo_light.intensity *= pow(0.996, ticks);
        if (explo_light.intensity < 10.0)
            explo_light.enabled =  false;
    }
}

    

void 
Rocket::setup_camera()
{
    Camera &cam = *globals.cam;

    SGfloat f;
//    sgVec3 e1, e2, e3, e4;
//    sgVec4 view_mat[4];

    sgVec3 up, vdir, tmp, s;

    // try to move camera 50 units behind rocket (in direction of movement)

    if (!aim_mode) 
    {
        sgNormalizeVec3(s, velocity);
        sgScaleVec3(s, 50.0);
    }
    else
    {
//        printf("dir = ");
//        print_vector(dir);
        sgScaleVec3(s, dir, 50.0f);
    }

    sgSubVec3(cam.wanted_pos, pos, s);

    // veiw direction is camera->rocket (z axis)
    sgSubVec3(vdir, cam.current_pos, pos);
    sgNormalizeVec3(cam.e3, vdir);

    // determine y axis
    sgSetVec3(up, 0.0, 1.0, 0.0);
    f = sgScalarProductVec3(up, cam.e3);
     sgScaleVec3(tmp, cam.e3, f);
    sgSubVec3(cam.e2, up, tmp);
    sgNormalizeVec3(cam.e2);

    // determine x axis
    sgVectorProductVec3(cam.e1, cam.e2, cam.e3);
     
    sgMakeIdentMat4(cam.view_mat);
//    sgMakeTransMat4(view_mat, up);

    // create view matrix
    cam.view_mat[0][0] = cam.e1[0];
    cam.view_mat[1][0] = cam.e1[1];
    cam.view_mat[2][0] = cam.e1[2];

    cam.view_mat[0][1] = cam.e2[0];
    cam.view_mat[1][1] = cam.e2[1];
    cam.view_mat[2][1] = cam.e2[2];

    cam.view_mat[0][2] = cam.e3[0];
    cam.view_mat[1][2] = cam.e3[1];
    cam.view_mat[2][2] = cam.e3[2];

//    view_mat[0][3] = 0.0;
//   view_mat[1][3] = 0.0;
//    view_mat[2][3] = 0.0;
    cam.view_mat[3][3] = 1.0;

    sgCopyMat4(globals.cam->orient, cam.view_mat);

    sgTransposeNegateMat4(globals.cam->orient_inv, globals.cam->orient);

    sgXformVec3(cam.e4, globals.cam->current_pos, cam.view_mat);

    cam.view_mat[3][0] = -cam.e4[0];
    cam.view_mat[3][1] = -cam.e4[1];
    cam.view_mat[3][2] = -cam.e4[2];

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();    
    glMultMatrixf((GLfloat *)cam.view_mat);
    
//    print_matrix(view_mat);
}

static int cubefaces[6][3] = {{ 1, 90,  GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB },
                              { 1, -90,  GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB },
                              { 0, 90,  GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB },
                              { 0, -90,  GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB },
                              { 1, 180,  GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB },
                              { 1, 0,  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB }};


void
Rocket::setup_cubemaps()
{
    int i;
    for (i = 0; i < 6; i++)
    {
        if (i != 0) continue;
        glTexImage2D(cubefaces[i][2], 0, GL_RGB, 128, 128, 0, 
                     GL_RGB, GL_UNSIGNED_BYTE, NULL);

    } 
}


// update one face of the cube
void
Rocket::update_cubemap()
{
    if (!use_cubemap) return;

    sgMat4 view, turn;
    sgVec3 axis;

    static int face = 0;
    
    if (cubefaces[face][0] == 1) sgSetVec3(axis, 0.0, 1.0, 0.0);
    else sgSetVec3(axis, 1.0, 0.0, 0.0);

    // translate to rocket position
    sgMakeTransMat4(view, -pos[0], -pos[1], -pos[2]);
    
    // look in the wanted cube face direction
    sgMakeRotMat4(turn, cubefaces[face][1], axis);

    // orient camera first

    sgPostMultMat4(view, globals.cam->orient);

#if 1
    if (face == 2 || face == 3) 
    {
        sgMat4 turn2;
        sgSetVec3(axis, 0.0, 0.0, 1.0);
        sgMakeRotMat4(turn2, 180.0, axis);
        sgPostMultMat4(view, turn2);
    }
#endif
    sgPostMultMat4(view, turn);

    if (face != 2 || face != 3) 
    {
        sgSetVec3(axis, 0.0, 0.0, 1.0);
        sgMakeRotMat4(turn, 180.0, axis);
        sgPostMultMat4(view, turn);
    }

    // push modelview
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixf((GLfloat *)view);

    // push current viewport
    glPushAttrib(GL_VIEWPORT_BIT);

    // setup 90 degree, 128x128 pixel viewport
    glViewport(0, 0, 128, 128);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(90, 1.0f, 5.0, 8000);

    // draw environment from current rocket position
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);

    globals.frame -= 2;
    globals.world->draw_rooms(in_room, pos); 
    globals.frame += 2;
  
#if 0  
    if (face == 2) 
    {
        globals.cube_tex.bind();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
//    memset(tex_data, 0, 128 * 128 * 3);
        
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0,
//                     GL_RGB, GL_UNSIGNED_BYTE, tex_data);
        
//    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, 128, 128,
//                      GL_RGB, GL_UNSIGNED_BYTE, tex_data);


    // copy to 2d texture (for testing)

//    glClear(GL_DEPTH_BUFFER_BIT);
//        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 128, 128, 0);
    }
#endif

    glCopyTexImage2D(cubefaces[face][2], 0,
                     GL_RGB, 0, 0, 128, 128, 0);
//    glClear(GL_COLOR_BUFFER_BIT);

    glPopAttrib();

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    face++;
    if (face > 5) face = 0;
}


void 
Rocket::draw_env()
{
    unsigned j;
    sgMat4 transf;

    if (mesh == NULL) return;
      
    // setup object transformation
    sgMakeTransMat4(transf, pos);
    sgPreMultMat4(transf, orient);

    glPushMatrix();
    glMultMatrixf((float *)transf) ;

    // call display list or create it...
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0, -2.0);


    glDisable(GL_LIGHTING); 
//    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);

#if 1
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
#else
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_ARB);
#endif

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glEnable(GL_TEXTURE_CUBE_MAP_ARB);

#if 1
//    int cur_material = -1;
    for (j = 0; j < mesh->nr_faces; j++) {
            
        Face *face = &mesh->faces[j];
                        
        if (face->pindx.size() == 3) glBegin(GL_TRIANGLES);
        else if (face->pindx.size() == 4) glBegin(GL_QUADS);
        else glBegin(GL_POLYGON);
        
        for (unsigned c = 0; c < face->pindx.size(); c++) {
            
            if (face->flags & FF_NORMALS) {
                assert(mesh->normals);
                glNormal3fv(mesh->normals[face->nindx[c]]);
            }
            
//                glColor3f(1.0, 1.0, 1.0);
            glVertex3fv(mesh->vertices[face->pindx[c]]);	    
        }
//	printf("\n");
        glEnd();
    }
#else       
    gluSphere(globals.q, 50, 20, 20);

#endif
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);

    glDisable(GL_TEXTURE_CUBE_MAP_ARB);

    glDisable(GL_POLYGON_OFFSET_FILL);

    glPopMatrix();
}
