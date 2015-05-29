
#include "first.h"
#include <stdio.h>
#include <GL/glu.h>
#include <plib/sg.h>

#include "texture.h"
#include "billboard.h"
#include "globals.h"

Billboard::Billboard()
{
    size = 200.0;

    anim_frame = 0;    
    anim_fps = 16.0;
    anim_time = real_time = 0.0;

    sgSetVec3(center, 0.0, 0.0, 400.0);
    sgSetVec4(color, 1.0, 1.0, 1.0, 1.0);

    gl_src_blend = GL_SRC_COLOR;
    gl_dst_blend = GL_ONE_MINUS_SRC_COLOR;

    active = true;
    one_shot = false;
    depth_test = false;
}

bool
Billboard::add_images(const char *prefix, int digits, const char *postfix)
{
    char format[32], fname[1024];
    
    if (digits == 0) {
	strcpy(format, "%s%d%s");
    }
    else {
	sprintf(format, "%%s%%0%dd%%s", digits);
    }

    printf("format str = %s\n", format);
    int nr = 1;
    do {
	sprintf(fname, format, prefix, nr++, postfix);
	printf("loading %s\n", fname);
    } while (add_image(fname));
    
    return true;
}

void
Billboard::add_texture(Texture &t)
{
    textures.push_back(t);
}

bool
Billboard::add_image(const char *fname)
{
    Texture tex;

    if (!tex.create_from_file(fname)) return false;

    printf("texture added\n");
    textures.push_back(tex);


    return true;
}

void
Billboard::update(float ticks)
{
    if (!active) return;

    real_time += TICKS_TO_SECS(ticks);

    float anim_spf = 1.0 / anim_fps;

    while (anim_time < real_time) {
	
	anim_time += anim_spf;
	anim_frame++;
	if (anim_frame >= textures.size()) 
        {
            if (one_shot) active = false;
            anim_frame = 0;
        }
    }   
}

void
Billboard::draw()
{
    if (!active) return;

    sgVec3 bl, br, tr, tl;

    float s2 = size / 2.0;

    // vector from object center to camera
    sgVec3 e1, e2, e3, up, tmp;
    sgSubVec3(e3, globals.cam->current_pos, center);
    sgNormalizeVec3(e3);
    
    // determine y axis
    sgSetVec3(up, 0.0, 1.0, 0.0);
    float f = sgScalarProductVec3(up, e3);
    sgScaleVec3(tmp, e3, f);
    sgSubVec3(e2, up, tmp);
    sgNormalizeVec3(e2);

    // determine x axis
    sgVectorProductVec3(e1, e2, e3);

    sgScaleVec3(e1, s2);
    sgScaleVec3(e2, s2);

    sgSubVec3(bl, center, e1);
    sgSubVec3(bl, e2);

    sgAddVec3(br, center, e1);
    sgSubVec3(br, e2);

    sgAddVec3(tr, center, e1);
    sgAddVec3(tr, e2);

    sgSubVec3(tl, center, e1);
    sgAddVec3(tl, e2);

    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    if (!depth_test) glDisable(GL_DEPTH_TEST);

    glDepthMask(0);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBlendFunc(gl_src_blend, gl_dst_blend);
//    glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);


    // bind current texture
    glColor4fv(color);
//    glColor4f(1.0, 1.0, 1.0, 1.0);

    textures[anim_frame].bind();
    glBegin(GL_QUADS);

//    glColor3f(1.0, 1.0, 1.0);
    glTexCoord2f(0.0, 1.0); glVertex3fv(bl);
    glTexCoord2f(1.0, 1.0); glVertex3fv(br);
    glTexCoord2f(1.0, 0.0); glVertex3fv(tr);
    glTexCoord2f(0.0, 0.0); glVertex3fv(tl);

    glEnd();
    glDepthMask(1);

    if (!depth_test) glEnable(GL_DEPTH_TEST);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}




void
Billboard::draw_ortho(int x, int y, int width, int height)
{

    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glDisable(GL_DEPTH_TEST);

    glDepthMask(0);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(1.0, 1.0, 1.0, 1.0);

    glMatrixMode   ( GL_MODELVIEW ) ;
    glPushMatrix   () ;
    glLoadIdentity () ;

    glMatrixMode   ( GL_PROJECTION ) ;
    glPushMatrix   () ;
    glLoadIdentity () ;
    gluOrtho2D     (0, globals.win_width - 1, 0, globals.win_height -1) ;

    // bind texture
    textures[0].bind();
    glBegin(GL_QUADS);

    glTexCoord2f(0.0, 1.0); glVertex2i(x, y);
    glTexCoord2f(1.0, 1.0); glVertex2i(x + width, y);
    glTexCoord2f(1.0, 0.0); glVertex2i(x + width, y + height);
    glTexCoord2f(0.0, 0.0); glVertex2i(x, y + height);

    glEnd();
    glDepthMask(1);

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_CULL_FACE);

}

