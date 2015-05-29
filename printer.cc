
#include "first.h"
#include <stdio.h>


#include <GL/gl.h>
#include <plib/sg.h>

#include "texture.h"
#include "printer.h"
#include "globals.h"

Texture font_tex;

float text_r, text_g, text_b;

#define NR_LETTERS 94 

typedef struct
{
    float tx, ty, tw;
} LInfo;

LInfo letters[NR_LETTERS] = {
{77, 115, 11},               // !
{133, 59, 12},               // "
{86, 115, 24},               // #
{110, 115, 24},              // $
{134, 115, 29},              // %
{165, 115, 27},              // &
{42, 87, 8},                 // '
{56, 87, 10},                // (
{67, 87, 10},                // )
{41, 115, 18},               // *
{232, 115, 26},              // +
{188, 143, 8},               // ,
{0,  87, 15},                // -
{196, 143, 8},               // .
{192, 115, 23},              // /
{124, 87, 24},               // 0
{150, 87, 7},                // 1
{158, 87, 25},               // 2
{183, 87, 26},               // 3
{209, 87, 28},               // 4
{0,   59, 27},               // 5
{28,  59, 26},               // 6
{55,  59, 25},               // 7
{82,  59, 25},               // 8
{108, 59, 25},               // 9
{213, 143, 8},               // :
{204, 143, 8},               // ;
{237, 143, 18},              // <
{215, 115, 15},              // =
{0,  115, 18},               // >
{60, 115, 18},               // ?
{0, 0, 0},                   // @

{46, 199, 28},               // A
{75, 199, 25},               // B
{101, 199, 23},              // C
{125, 199, 25},              // D
{151, 199, 23},              // E
{175, 199, 23},              // F
{199, 199, 25},              // G
{225, 199, 25},              // H
{251, 199, 7},               // I
{0, 171, 13},                // J
{15, 171, 22},               // K
{39, 171, 23},               // L
{63, 171, 32},               // M
{96, 171, 25},               // N
{122, 171, 24},              // O
{147, 171, 25},              // P
{173, 171, 25},              // Q
{199, 171, 25},              // R
{224, 171, 24},              // S
{0,   143, 23},              // T
{23,  143, 25},              // U
{48, 143, 28},               // V
{77, 143, 36},               // W
{113, 143, 25},              // X
{139, 143, 25},              // Y
{164, 143, 23},              // Z
{78, 87, 10},                // [
{18, 115, 23},               // backslash
{88, 87, 10},                // ]
{0, 0, 0},                   // ^
{18, 87, 22},                // _
{0, 0, 0},                   // `
{1, 255, 23},                // a
{24, 255, 22},               // b
{47, 255, 22},               // c
{72, 255, 22},               // d
{94, 255, 23},               // e
{118, 255, 12},              // f
{131, 255, 22},              // g
{154, 255, 22},              // h
{176, 255, 8},               // i
{186, 255, 13},              // j
{200, 255, 19},              // k
{221, 255, 7},               // l
{229, 255, 29},              // m
{0, 227, 22},                // n
{24, 227, 22},               // o
{47, 227, 22},               // p
{70, 227, 22},               // q
{93, 227, 13},               // r
{107, 227, 21},              // s
{128, 227, 16},              // t
{145, 227, 22},              // u
{167, 227, 26},              // v
{193, 227, 131},             // w
{224, 227, 23},              // x
{1, 199, 24},                // y
{23, 199, 23},               // /_
{100, 87, 12},               // {
{0, 0, 0},                   // |
{112, 87, 12},               // }
{221, 143, 16}               // |_|

};


static void 
setOpenGLState (float w, float h)
{

    glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT | 
                 GL_TRANSFORM_BIT | GL_LIGHTING_BIT |
                 GL_COLOR_BUFFER_BIT) ;
    glDisable      ( GL_LIGHTING   ) ;
    glDisable      ( GL_FOG        ) ;
    glDisable      ( GL_TEXTURE_2D ) ;
    glDisable      ( GL_DEPTH_TEST ) ;
    glDisable      ( GL_CULL_FACE  ) ;
//  glEnable       ( GL_ALPHA_TEST ) ;
    glEnable       ( GL_BLEND ) ;
//  glAlphaFunc    ( GL_GREATER, 0.1f ) ;
//  glBlendFunc    ( GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR ) ;
    glBlendFunc    ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
    
    glViewport     ( 0, 0, (int)w, (int)h ) ;
    glMatrixMode   ( GL_PROJECTION ) ;
    glPushMatrix   () ;
    glLoadIdentity () ;
    gluOrtho2D     ( 0, w, 0, h ) ;
    glMatrixMode   ( GL_MODELVIEW ) ;
    glPushMatrix   () ;
    glLoadIdentity () ;
}

static void 
restoreOpenGLState ( void )
{
    glMatrixMode   ( GL_PROJECTION ) ;
    glPopMatrix    () ;
    glMatrixMode   ( GL_MODELVIEW ) ;
    glPopMatrix    () ;
    glPopAttrib    () ;
}


void
center_string(float y, float size, const char *str)
{
    float w = get_string_size(str, size, NULL, NULL);
    print_string((globals.win_width - w) / 2, y, size, str);
}

void
print_string(float x, float y, float size, const char *str)
{
    GLfloat   vp[4];

    glGetFloatv(GL_VIEWPORT, vp);

    setOpenGLState(vp[2], vp[3]) ;

//    float tex_env_color[4] = {1.0, 0.0, 0.0, 1.0};

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, tex_env_color); 
    font_tex.bind();

    glColor4f(0.0, 0.0, 0.0, 1.0);
    draw_string(x + 2, y - 2, size * 128.0f, str);
    glColor4f(text_r, text_g, text_b, 1.0);
    draw_string(x, y, size * 128.0f, str);

    restoreOpenGLState () ;

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); 
}

void 
draw_string(float x, float y, float size, const char *str)
{

    float th = 22.0 / 256.0;

    glBegin(GL_QUADS);    
    while (*str != 0) 
    {
        if (*str <= 125) 
        {
            if (*str < 33) 
            {
                x += 0.08f * size;
                str++;
                continue;
            }

            LInfo &li = letters[*str - 33];

            // lower left
            glTexCoord2f(li.tx, li.ty);
            glVertex2f(x, y);

            // lower right
            glTexCoord2f(li.tx + li.tw, li.ty);
            glVertex2f(x + li.tw * size, y);

            // upper right
            glTexCoord2f(li.tx + li.tw, li.ty - th);
            glVertex2f(x + li.tw * size, y + th * size);

            // upper left
            glTexCoord2f(li.tx, li.ty - th);
            glVertex2f(x, y + th * size);

            //printf("letter at %.2f, %2f, %.2f\n", 
            //256*li.tx, 256*li.ty, 256*li.tw);

            x += li.tw * size;
        }
        str++;
    }
    glEnd();

}



void 
print_matrix(const sgMat4 m)
{
    SGfloat *f = (SGfloat *)m;

    for (int i = 0; i < 4; i++) 
    {
	printf("%5.1f, %5.1f, %5.1f, %5.1f\n", f[0], f[4], f[8], f[12]);
	f++;
    }
   
}

void
get_letter_size(char ch, float size, float *w, float *h)
{
    if (ch == 32) 
    {
        *w = 10.24 * size;
        *h = 0.0;
        return;
    }
    LInfo &li = letters[ch - 33];
    *h = 22.0 / 256.0 * (size * 128.0f);
    *w = li.tw * size * 128.0f;
}

float
get_string_size(const char *str, float size, float *wp, float *hp)
{
    float sh = 0.0f;
    float sw = 0.0f;

    int i = 0;
    while (str[i] != 0)
    {
        float w, h;
        get_letter_size(str[i], size, &w, &h);
        sw += w;
        if (h > sh) sh = h;
        i++;
    }
    if (wp != NULL) *wp = sw;
    if (hp != NULL) *hp = sh;

    return sw;
}

void
set_text_color(float r, float g, float b)
{
    text_r = r;
    text_g = g;
    text_b = b;
}

void 
print_vector(const sgVec3 v)
{
    printf("[%.8f, %.8f, %.8f]\n", v[0], v[1], v[2]);
}

void 
init_printer()
{
    if (!font_tex.create_from_file("data/gfx/font-alpha.png")) 
    {
	assert(0);
    }

    text_r = text_g = text_b = 1.0;

    for (int i = 0; i < NR_LETTERS; i++) 
    {
        letters[i].tx /= 256.0;
        letters[i].ty /= 256.0; //(256.0 - letters[i].ty) / 256.0;
        letters[i].tw /= 256.0;

    }
    
//    times_roman = new fntTexFont("Times-Bold.txf");
//    printf("0x%x\n", times_roman);
}
