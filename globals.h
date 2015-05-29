
#ifndef M_PI
#define M_PI 3.14159
#endif

#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : (-(x)))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define DEG_2_RAD(x)    (M_PI * (x) / 180.0)

#define TICKS_TO_SECS(x) (((float) (x)) * 0.001)

#include <GL/glu.h>
#include <GL/glext.h>
#include "object.h"
#include "camera.h"
#include "frustum.h"
#include "world.h"

typedef struct {
    long frame;
    int lm_mode;
    int draw_textures, draw_lightmaps;

    float ticks, ticks_per_frame;
    int win_width, win_height;
    GLUquadricObj *q;
    Camera *cam;
    Frustum *frustum;
    World *world;

    Texture lightbulb_tex;
    Texture cube_tex;
}
globals_t;

extern globals_t globals;

#define FRAND(x) ((x) * (float)(rand()) / RAND_MAX)

extern PFNGLMULTITEXCOORD2FVARBPROC pglMultiTexCoord2fvARB;
extern PFNGLACTIVETEXTUREARBPROC pglActiveTextureARB;
