
#ifndef BILLBOARD_H
#define BILLBOARD_H

#include <vector>

#include "texture.h"

using namespace std;

class Billboard
{
    sgVec4              color;
    sgVec3		center;
    float		size;
    float		real_time;
    float               anim_fps;
    float		anim_time;
    unsigned int	anim_frame;

//    char		*data;

    vector<Texture>	textures;

    int                 gl_src_blend, gl_dst_blend;
    
 public:
    bool                one_shot, active, depth_test;

    Billboard();

    void add_texture(Texture &t);
    
    bool add_image(const char *fname);
    bool add_images(const char *prefix, int digits, const char *postfix);

    void set_center(const sgVec3 c) { sgCopyVec3(center, c); } 

    void set_center(float x, float y, float z) { 
        center[0] = x;
        center[1] = y;
        center[2] = z;
    }

    void set_color(float r, float g, float b, float a) {
        color[0] = r; color[1] = g; color[2] = b; color[3] = a;
    }
       
    void set_blend(int s, int d) { gl_src_blend = s; gl_dst_blend = d; }
    void set_center(sgVec3 new_center) { sgCopyVec3(center, new_center); }
    void set_size(float new_size) { size = new_size; }
    void set_fps(float fps) { anim_fps = fps; }
//    void set_depthtest(bool t) { depth_test

    void update(float ticks);
    void draw();
    void draw_ortho(int x, int y, int width, int height);
};

#endif
