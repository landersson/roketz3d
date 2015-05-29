
#include "first.h"
#include <GL/gl.h>
#include <GL/glu.h>

#include <plib/sg.h>
#include "particle.h"
#include "globals.h"

#define GRAVITY 1.0

float 
frand2()
{
    return (float)(rand()) / RAND_MAX;
}

void 
ParticleSystem::add_particle(sgVec3 pos, sgVec3 velocity, float ttl)
{
    Particle *newp = NULL;

    for (unsigned i = 0; i < ps.size(); i++) {
	if (ps[i].time_to_live < 0.0) {
	    newp = &ps[i];
	}
    }
    if (newp == NULL) {
	Particle tmp;
	ps.push_back(tmp);
//        printf("size = %d\n", ps.size());
	newp = &(ps.back());
    }
    sgCopyVec3(newp->pos, pos);
    sgCopyVec3(newp->velocity, velocity);

    newp->time_to_live = ttl;
    newp->size = 4.0;
}

void 
ParticleSystem::update_smoke(float ticks)
{
    sgVec3 move;

    for (unsigned i = 0; i < ps.size(); i++) 
    {
	if (ps[i].time_to_live > 0.0) 
        {
	    sgScaleVec3(move, ps[i].velocity, ticks);
	    sgAddVec3(ps[i].pos, move);
	    ps[i].velocity[1] -= 0.0005 * ticks * GRAVITY;
	    ps[i].time_to_live -= ticks;
	    float ticks2 = ticks;
#if 1
	    while (ticks2 > 0) {
		ps[i].velocity[0] *= 0.95f;
		ps[i].velocity[1] *= 0.95f;
		ps[i].velocity[2] *= 0.95f;
		ticks2 -= 3.0f;
	    }
#endif
	}
    }
}


void 
ParticleSystem::update(float ticks)
{
    sgVec3 move;

    for (unsigned i = 0; i < ps.size(); i++) 
    {
	if (ps[i].time_to_live > 0.0) 
        {
	    sgScaleVec3(move, ps[i].velocity, ticks);
	    sgAddVec3(ps[i].pos, move);
	    ps[i].velocity[1] -= 0.0002 * ticks * GRAVITY;
	    ps[i].time_to_live -= ticks;
	    float ticks2 = ticks;
	    while (ticks2 > 0) {
		ps[i].velocity[0] *= 0.999f;
		ps[i].velocity[1] *= 0.999f;
		ps[i].velocity[2] *= 0.999f;
		ticks2 -= 3.0f;
	    }
	}
    }
}



void 
ParticleSystem::setup_explosion(sgVec3 center)
{
    if (ps.size() < 128) 
    {
        Particle dummy;
        ps.resize(128, dummy);
    }
        
    for (unsigned i = 0; i < ps.size(); i++)
    {
        sgCopyVec3(ps[i].pos, center);
        sgSetVec3(ps[i].velocity, -0.5f + 1.0f * frand2(),
                  -0.5f + 1.0f * frand2(),
                  -0.5f + 1.0f * frand2());

        ps[i].time_to_live = 2000 + 1000.0 * frand2();
        ps[i].size = 3.0 + 3 * frand2();

        printf("speed = %.2f, %.2f, %.2f\n", ps[i].velocity[0],
               ps[i].velocity[1], ps[i].velocity[2]);
    }

}

bool
ParticleSystem::set_texture(const char *fname)
{
    return texture.create_from_file(fname);
}

void 
ParticleSystem::draw()
{
    glPushMatrix();

    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND);
    glDepthMask(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    texture.bind();

    sgVec3 bl, br, tr, tl;
    float s2 = 2.0 / 2.0;    

    sgScaleVec3(bl, globals.cam->e1, -s2);
    sgAddScaledVec3(bl, globals.cam->e2, -s2);

    sgScaleVec3(br, globals.cam->e1, s2);
    sgAddScaledVec3(br, globals.cam->e2, -s2);

    sgScaleVec3(tr, globals.cam->e1, s2);
    sgAddScaledVec3(tr, globals.cam->e2, s2);

    sgScaleVec3(tl, globals.cam->e1, -s2);
    sgAddScaledVec3(tl, globals.cam->e2, s2);
    
//    glScalef(2.0, 2.0, 2.0);
    glBegin(GL_QUADS);
#if 1
    for (unsigned i = 0; i < ps.size(); i++) {
	if (ps[i].time_to_live > 0.0) {

            float r = 1.0, g = 1.0, b = 1.0;
            float s = 1.0 - ps[i].time_to_live / 3000.0;

            s *= 2;
//            s /= 2;    

            b -= s * 4.0f;
            if (s > 0.25) g -= (s - 0.25f) * 4.0f;
            if (s > 0.5) r -= (s - 0.5)  * 1.0f;
            
            if (r < 0.0f) r = 0.0f;
            if (g < 0.0f) g = 0.0f;
            if (b < 0.0f) b = 0.0f;

//	    printf("p(%f, %f, %f)\n", p->pos[0], p->pos[1], p->pos[2]);
//	    float s2 = p->size / 2;
	    sgVec3 p;

	    glColor3f(r, g, b);

	    sgAddScaledVec3(p, ps[i].pos, bl, ps[i].size);
            glTexCoord2f(0.0, 0.0);
	    glVertex3fv(p);

	    sgAddScaledVec3(p, ps[i].pos, br, ps[i].size);
            glTexCoord2f(1.0, 0.0);
	    glVertex3fv(p);

	    sgAddScaledVec3(p, ps[i].pos, tr, ps[i].size);
            glTexCoord2f(1.0, 1.0);
	    glVertex3fv(p);

	    sgAddScaledVec3(p, ps[i].pos, tl, ps[i].size);
            glTexCoord2f(0.0, 1.0);
	    glVertex3fv(p);

	}
    }
#endif
    glEnd();
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);

    glDepthMask(1);
    glPopMatrix();
}
    

void
ParticleSystem::draw_explo()
{
    glPushMatrix();

    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND);
    glDepthMask(0);
//    glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
    glBlendFunc(GL_ONE, GL_ONE);
    texture.bind();

    sgVec3 bl, br, tr, tl;
    float s2 = 2.0 / 2.0;    

    sgScaleVec3(bl, globals.cam->e1, -s2);
    sgAddScaledVec3(bl, globals.cam->e2, -s2);

    sgScaleVec3(br, globals.cam->e1, s2);
    sgAddScaledVec3(br, globals.cam->e2, -s2);

    sgScaleVec3(tr, globals.cam->e1, s2);
    sgAddScaledVec3(tr, globals.cam->e2, s2);

    sgScaleVec3(tl, globals.cam->e1, -s2);
    sgAddScaledVec3(tl, globals.cam->e2, s2);
    
//    glScalef(2.0, 2.0, 2.0);
    glBegin(GL_QUADS);
#if 1
    for (unsigned i = 0; i < ps.size(); i++) {
	if (ps[i].time_to_live > 0.0) {

            float r = 1.0, g = 1.0, b = 1.0;

#if 1
            float s = 1.0 - ps[i].time_to_live / 3000.0;

//            s /= 2;
//            s /= 2;    

            b -= s * 1.0f;
            if (s > 0.25) g -= (s - 0.25f) * 1.0f;
            if (s > 0.5) r -= (s - 0.5)  * 0.25f;
            
            if (r < 0.0f) r = 0.0f;
            if (g < 0.0f) g = 0.0f;
            if (b < 0.0f) b = 0.0f;
#endif
//	    printf("p(%f, %f, %f)\n", p->pos[0], p->pos[1], p->pos[2]);
//	    float s2 = p->size / 2;
	    sgVec3 p;

	    glColor4f(r, g, b, 0.8f);

	    sgAddScaledVec3(p, ps[i].pos, bl, ps[i].size);
            glTexCoord2f(0.0, 0.0);
	    glVertex3fv(p);

	    sgAddScaledVec3(p, ps[i].pos, br, ps[i].size);
            glTexCoord2f(1.0, 0.0);
	    glVertex3fv(p);

	    sgAddScaledVec3(p, ps[i].pos, tr, ps[i].size);
            glTexCoord2f(1.0, 1.0);
	    glVertex3fv(p);

	    sgAddScaledVec3(p, ps[i].pos, tl, ps[i].size);
            glTexCoord2f(0.0, 1.0);
	    glVertex3fv(p);

	}
    }
#endif
    glEnd();
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);

    glDepthMask(1);
    glPopMatrix();
}
    

