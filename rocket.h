
#ifndef ROCKET_H
#define ROCKET_H

#include "particle.h"
#include "billboard.h"
#include "object.h"
#include "camera.h"

#define NR_FLAMES 320           

// game states
#define GS_LAUNCHING    0
#define GS_PLAYING      1
#define GS_DYING        2
#define GS_GAMEOVER     3


class Rocket : public Object 
{
    bool                thrust;
    bool                on_base;
    bool                aim_mode;

    float               last_launch_tick;

    sgQuat              rot_quat;

    sgMat4              rot;
    sgVec3              dir;

    float               new_flame;

    ParticleSystem      jet, explo;
    Billboard           jet_flare;
  
    int                 next_missile_light;

    DynamicLight        flame_light;
    DynamicLight        missile_lights[16];
    DynamicLight        explo_light;

    Mesh                *missile_mesh;

    vector<Object *>    missiles;

    // used for ship/asteroid explosions etc
//    vector<Billboard *> billboards;

    Billboard           asteroid_exp1;

    sgMat4              prev_cam_orient;

    Mix_Music           *music;

    Mix_Chunk           *missile_snd, *flame_snd; //, *wall_hit_snd;
    Mix_Chunk           *explo_snd, *countdown_snd, *liftoff_snd, *eagle_snd;
    

    int                 missile_channel, flame_channel; //, wall_hit_channel;
    int                 explo_channel;

    int                 countdown;
    float               launch_ticks, on_base_ticks, digit_ticks, dead_ticks;

    bool                refueling, first_liftoff;

    // cube map textures
//    int                 gl_cube_maps[6];
//    Texture             cube_tex;

 public:
    bool                use_cubemap;

    int                 lives, points;
    float               fuel, shield;

    void init();
    void load_sound_effects();
    void prepare_for_takeoff();


    void launch_missile(const sgVec3 direction);
    void asteroid_hit(Object *ast);

    float *get_pos() { return pos; }
    float *get_velocity() { return velocity; }

    void update(float ticks, Camera *camera);

    void draw();
    void draw_env();

    void setup_orient(int x, int y);
    void setup_camera();

    void setup_cubemaps();
    void update_cubemap();
};

void orient_rocket(void *_r, int x, int y);

#endif
