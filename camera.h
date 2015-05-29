#ifndef CAMERA_H
#define CAMERA_H

//#include "rocket.h"

class Camera
{
 public:
    float yaw, pitch, roll;
    sgVec3 e1, e2, e3, e4;

    sgVec3  wanted_pos, current_pos, previous_pos;
    sgMat4  orient, orient_inv;

    sgVec4 view_mat[4];

    void inc_yaw(float d) { yaw += d; }
    void inc_pitch(float d) { pitch += d; }

    void move_forward(float d);
    void move_right(float d);
    void move_up(float d);

    void set_pos(float x, float y, float z) { 
	current_pos[0] = x;
	current_pos[1] = y;
	current_pos[2] = z;
    }

    void setup_for_rocket(const sgVec3 pos, 
                          const sgVec3 velocity,
                          bool aim_mode);
    void setup_modelview();
//    float* get_orientation() { return orient[0]; };
};

#endif
