
#include "first.h"
#include <stdio.h>

#include <GL/gl.h>

#include <plib/sg.h>

#include "camera.h"

// hmmm, not much left here...

void 
Camera::setup_modelview()
{
    sgVec3 axis;
    sgMat4 tmat, mv;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    sgMakeIdentMat4(orient);

    sgSetVec3(axis, 0, 1, 0);
    sgMakeRotMat4(orient, -yaw, axis);

    sgSetVec3(axis, 1, 0, 0);
    sgMakeRotMat4(tmat, -pitch, axis);
    sgPostMultMat4(orient, tmat);

    sgTransposeNegateMat4(orient_inv, orient);

    sgMakeTransMat4(tmat, -current_pos[0], -current_pos[1], -current_pos[2]);
    sgMultMat4(mv, orient, tmat);

    glMultMatrixf((GLfloat *)mv);

    sgSetVec3(e1, mv[0][0], mv[1][0], mv[2][0]);
    sgSetVec3(e2, mv[0][1], mv[1][1], mv[2][1]);
    sgSetVec3(e3, mv[0][2], mv[1][2], mv[2][2]);

}

void 
Camera::move_forward(float d)
{
    sgAddScaledVec3(current_pos, e3, -d);
}

void 
Camera::move_right(float d)
{
    sgAddScaledVec3(current_pos, e1, d);
}

void 
Camera::move_up(float d)
{
    sgAddScaledVec3(current_pos, e2, d);
}
