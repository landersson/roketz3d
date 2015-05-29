
#ifndef PARTICLE_H
#define PARTICLE_H

#include <vector>
using namespace std;
#include "texture.h"

typedef struct {
    float	time_to_live;
    sgVec3	pos;
    sgVec3	velocity;
    float       size;
} Particle;


class ParticleSystem
{
    Texture      texture;

    vector<Particle>   ps;

//    bool         active;
 public:
    bool set_texture(const char *fname);
    void add_particle(sgVec3 pos, sgVec3 velocity, float ttl);
    void draw();
    void draw_explo();

    void update(float ticks);
    void update_smoke(float ticks);

    void setup_explosion(sgVec3 center);
};

#endif
