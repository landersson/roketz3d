/*----------------------------------------------------------------------------

 Roketz3D - main.cpp

----------------------------------------------------------------------------*/

#include "first.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <vector>
#include <thread>

#include <plib/sg.h>

extern "C" {

#include <jpeglib.h>
#include <jerror.h>
#include <png.h>
#include <zlib.h>

}

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include "printer.h"
#include "camera.h"
#include "rocket.h"
#include "world.h"
#include "input.h"

#include "globals.h"
#include "WorkQueue.h"



#define WIDTH		1280
#define HEIGHT		720
//#define HEIGHT		720
//#define WIDTH		640
//#define HEIGHT        480


#define DESCENT_MODE    0
#define ROCKET_MODE     1


Camera		camera;
World		world;
Rocket		rocket1;

bool		initialized = false;
bool            paused = false;
bool            play_music = true;
int		        mode = ROCKET_MODE;
int             game_state = GS_PLAYING;

int             frame_fps_cnt = 0;
float           last_fps_ticks = 0;

Billboard       loadscreen;
Billboard       fuel_meter, shield_meter, life;
Billboard       red0, red1, orange0, orange1;


// OpenGL extension function pointers...
PFNGLMULTITEXCOORD2FVARBPROC pglMultiTexCoord2fvARB = NULL;
PFNGLACTIVETEXTUREARBPROC pglActiveTextureARB = NULL;


// global data and pointers
globals_t globals;

SDL_Surface *surf;

void 
show_info(void)
{
    char strbuf[64];

    static float fps = 0.0;

    frame_fps_cnt++;
    if (frame_fps_cnt >= 40) 
    {
        float tick = (float)SDL_GetTicks();
        fps = 40000.0 / (tick - last_fps_ticks);
        last_fps_ticks = tick;
        frame_fps_cnt = 0;
    }

#if 0
    sprintf(strbuf, "Room: %d", world.get_current_room_id());
    print_string(8, 4, 1.0f, strbuf);
#endif

#if 0
    sprintf(strbuf, "pos: %.2f, %.2f, %.2f\n", camera.current_pos[0],
            camera.current_pos[1], camera.current_pos[2]);
    print_string(8, globals.win_height - 14, 1.0f, strbuf);

#else
    sprintf(strbuf, "FPS: %.0f", fps);
    print_string(globals.win_width - 86, 4, 1.0f, strbuf);
#endif
}


// main display function
void 
display(void)
{
    if (!initialized) return;

    glPushMatrix();

    glPopMatrix();

    world.draw();
    rocket1.draw();

    show_info();

    if (game_state == GS_GAMEOVER)
    {
        center_string(globals.win_height / 2.0f + 50.0f, 2.4f,
                     "GAME OVER");

        char strbuf[32];
        sprintf(strbuf, "SCORE: %d", rocket1.points);
        center_string(globals.win_height / 2 - 50, 2.0, strbuf);
    }

#if 1
    // draw status display
    fuel_meter.draw_ortho(5, globals.win_height - 86, 200, 80);
    shield_meter.draw_ortho(globals.win_width - 205, globals.win_height - 86, 200, 80);

    // draw fuel indicator
    int i;
    for (i = 0; i < 10; i++) 
    {
        Billboard *b;
        if ((i * 10.0 + 5.0) > rocket1.fuel) b = &orange0; else b = &orange1;
        b->draw_ortho(87 + 10 * i, globals.win_height - 38, 9, 9);
    }

    // draw energy indicator
    for (i = 0; i < 10; i++) 
    {
        Billboard *b;
        if ((i * 10.0 + 5.0) > rocket1.shield) b = &red0; else b = &red1;
        b->draw_ortho(globals.win_width - 184 + 10 * i, 
                      globals.win_height - 38, 9, 9);
    }

    // draw life icons
    for (i = 0; i < rocket1.lives; i++) 
    {
        life.draw_ortho(4 + 28 * i, 4, 32, 32);
    } 
#endif
    globals.frame++;
    SDL_GL_SwapBuffers();      
}


// setup standard game viewport
void 
reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (GLfloat)width / height, 5.0, 8000);

    globals.win_width = width;
    globals.win_height = height;

    glDepthFunc(GL_LESS);
}


void 
init_gl(void)
{
    reshape(globals.win_width, globals.win_height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // get OpenGL function pointers...
    pglActiveTextureARB =  
        (PFNGLACTIVETEXTUREARBPROC)SDL_GL_GetProcAddress("glActiveTextureARB");

    pglMultiTexCoord2fvARB =    
        (PFNGLMULTITEXCOORD2FVARBPROC)SDL_GL_GetProcAddress("glMultiTexCoord2fvARB");

    assert(pglActiveTextureARB && pglMultiTexCoord2fvARB);
}


void 
update_viewer(float ticks)
{

    sgCopyVec3(camera.previous_pos, camera.current_pos);
    float speed;
    if (key_down(SDLK_LSHIFT)) speed = 0.03 * ticks;
    else speed = 0.5 * ticks;

    if (key_down(SDLK_w)) camera.move_forward(speed);
    if (key_down(SDLK_s)) camera.move_forward(-speed);
    if (key_down(SDLK_a)) camera.move_right(-speed);
    if (key_down(SDLK_d)) camera.move_right(speed);

    if (key_down(SDLK_LALT)) 
    {
	if (key_down(SDLK_UP)) camera.move_up(speed);
	if (key_down(SDLK_DOWN)) camera.move_up(-speed);                  

    }
    else 
    {
	if (key_down(SDLK_LEFT)) camera.inc_yaw(0.1 * ticks);
	if (key_down(SDLK_RIGHT)) camera.inc_yaw(-0.1 * ticks);
	
	if (key_down(SDLK_UP)) camera.inc_pitch(0.1 * ticks);
	if (key_down(SDLK_DOWN)) camera.inc_pitch(-0.1 * ticks);
    }
}


void 
orient_viewer(void *_r, int x, int y)
{
    static int first = 1, last_x, last_y;

    if (first) { last_x = x; last_y = y; first=0; }

    float dx = (float)(x - last_x);
    float dy = (float)(y - last_y);
    last_x = x; last_y = y;

//    printf("dx=%f, dy=%f: yaw=%f, pitch=%f\n", dx, dy, vp.yaw, vp.pitch);

    if (ABS(dx) > 100 || ABS(dy) > 100) return;

    camera.inc_yaw(-dx * 0.3); 
    camera.inc_pitch(-dy * 0.3); 

}

void 
toggle_flight_mode()
{
    if (mode == DESCENT_MODE) 
    {
	mode = ROCKET_MODE;
	set_motion_callback(orient_rocket, &rocket1);
        SDL_WarpMouse(WIDTH / 2, HEIGHT / 4);
        update_input();
    }
    else 
    {
	mode = DESCENT_MODE;
	set_motion_callback(orient_viewer, NULL);
    }
    while (key_down(SDLK_r)) update_input();
}


int write_to_jpeg_file(int width, int height, unsigned char *data, FILE *fp, int quality)
{
    /* this struct contains the JPEG compression parameters and pointers to
     * working space (which is allocated as needed by the JPEG library).
     * It is possible to have several such structures, representing multiple
     * compression/decompression processes, in existence at once.  We refer
     * to any one struct (and its associated working data) as a "JPEG object".
     */
    struct jpeg_compress_struct cinfo;
    /* This struct represents a JPEG error handler.  It is declared separately
     * because applications often want to supply a specialized error handler
     * (see the second half of this file for an example).  But here we just
     * take the easy way out and use the standard error handler, which will
     * print a message on stderr and call exit() if compression fails.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    struct jpeg_error_mgr jerr;
    /* More stuff */
    JSAMPROW row_pointer[1];    /* pointer to JSAMPLE row[s] */
    int row_stride;             /* physical row width in image buffer */

    /* Step 1: allocate and initialize JPEG compression object */
        
    /* We have to set up the error handler first, in case the initialization
     * step fails.  (Unlikely, but it could happen if you are out of memory.)
     * This routine fills in the contents of struct jerr, and returns jerr's
     * address which we place into the link field in cinfo.
     */
    cinfo.err = jpeg_std_error(&jerr);
    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&cinfo);
        
    /* Step 2: specify data destination (eg, a file) */
    /* Note: steps 2 and 3 can be done in either order. */
        
    /* Here we use the library-supplied code to send compressed data to a
     * stdio stream.  You can also write your own code to do something else.
     * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
     * requires it in order to write binary files.
     */

    jpeg_stdio_dest(&cinfo, fp);
        
    /* Step 3: set parameters for compression */
        
    /* First we supply a description of the input image.
     * Four fields of the cinfo struct must be filled in:
     */
    cinfo.image_width = width;     /* image width and height, in pixels */
    cinfo.image_height = height;

    cinfo.input_components = 3;              /* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB;          /* colorspace of input image */

    /* Now use the library's routine to set default compression parameters.
     * (You must set at least cinfo.in_color_space before calling this,
     * since the defaults depend on the source color space.)
     */
    jpeg_set_defaults(&cinfo);
    /* Now you can set any non-default parameters you wish to.
     * Here we just illustrate the use of quality (quantization table) scaling:
     */
    jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
        
    /* Step 4: Start compressor */
        
    /* TRUE ensures that we will write a complete interchange-JPEG file.
     * Pass TRUE unless you are very sure of what you're doing.
     */
    jpeg_start_compress(&cinfo, TRUE);
        
    /* Step 5: while (scan lines remain to be written) */
    /*           jpeg_write_scanlines(...); */
        
    /* Here we use the library's state variable cinfo.next_scanline as the
     * loop counter, so that we don't have to keep track ourselves.
     * To keep things simple, we pass one scanline per call; you can pass
     * more if you wish, though.
     */
    row_stride = width * 3;   /* JSAMPLEs per row in image_buffer */
        
    unsigned char *buffer = data;

    while (cinfo.next_scanline < cinfo.image_height) {
        /* jpeg_write_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could pass
         * more than one scanline at a time if that's more convenient.
         */
        row_pointer[0] = buffer + cinfo.next_scanline * row_stride;
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
        
    /* Step 6: Finish compression */
        
    jpeg_finish_compress(&cinfo);
    /* After finish_compress, we can close the output file. */
        
    /* Step 7: release JPEG compression object */
        
    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_compress(&cinfo);
        
    /* And we're done! */
    return 1;
}

struct png_mem_buffer
{
    unsigned char *buffer;
    unsigned int  size;
    unsigned int  offset;
};

static void png_mem_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct png_mem_buffer *buf = (struct png_mem_buffer *)png_get_io_ptr(png_ptr);
    // make sure we have enough space
    while ((buf->size - buf->offset) < length)
    {
        buf->size *= 2;
//        printf("realloc!\n");
        buf->buffer = (unsigned char *)realloc(buf->buffer, buf->size);
    }
    memcpy(buf->buffer + buf->offset, data, length);
    buf->offset += length;
}


int compress_png(unsigned char **buffer, int width, int height, uint8_t *data, int quality)
{
    png_structp  png_ptr = NULL;
    png_infop    info_ptr = NULL;

    png_byte *row_pointers[height];

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        return -1;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        return -1;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        return -1;
    }

    struct png_mem_buffer png_buf;


    memset(&png_buf, 0, sizeof(png_buf));
    unsigned size = width * height * 3;
    png_buf.buffer = (unsigned char *)malloc(size);
    png_buf.size = size;
    png_buf.offset = 0;

//    printf("malloc %p\n", png_buf.buffer);

    /* setup custom writer functions */
    png_set_write_fn(png_ptr, (voidp)&png_buf, png_mem_write_data, NULL);

    int zcompression = Z_NO_COMPRESSION;

    if(zcompression > Z_BEST_COMPRESSION)
        zcompression = Z_BEST_COMPRESSION;

    if(zcompression == Z_NO_COMPRESSION) // No compression
    {
        png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
        png_set_compression_level(png_ptr, Z_NO_COMPRESSION);
    }
    else if(zcompression < 0) // Default compression
        png_set_compression_level(png_ptr, Z_DEFAULT_COMPRESSION);
    else
        png_set_compression_level(png_ptr, zcompression);

    int png_type = -1;


    png_type = PNG_COLOR_TYPE_RGB;
        
    png_set_IHDR(png_ptr, info_ptr,
                 width, height,
                 8,
                 png_type,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    unsigned char *org_data = data;

    for(int i = 0; i < height; i++)
    {
        row_pointers[i] = (png_byte*)(org_data + i * width * 3);
    }

    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    *buffer = png_buf.buffer;
    
    return png_buf.offset;
}

class ImageData
{
public:
    ImageData() = default;
    ImageData(uint8_t *data, int width, int height, int id) : 
        _data(data), 
        _width(width),
        _height(height),
        _id(id) 
    { }
    
    uint8_t* data() const { return _data; }
    int width() const { return _width; }
    int height() const { return _height; }
    int id() const { return _id; }

private:
    uint8_t* _data;
    int      _width;
    int      _height;
    int      _id;
};

static bool all_done = false;
static WorkQueue<ImageData> img_queue;

int jpeg_writer_thread(int id)
{
    while (!all_done)
    {
        ImageData img_data;

        if (img_queue.getNextJob(img_data))
        {

#if 1
            char filename[32];
#if 0
            sprintf(filename, "roketz%05d.jpg", img_data.id());
            
            FILE *fp = fopen(filename, "wb");
            
            write_to_jpeg_file(img_data.width(), img_data.height(),
                               img_data.data(), fp, 100);
            
            fclose(fp);

#else
            sprintf(filename, "roketz%05d.png", img_data.id());
            
            FILE *fp = fopen(filename, "wb");
            
            uint8_t *buffer;
            unsigned bytes = compress_png(&buffer, img_data.width(), img_data.height(),
                                          img_data.data(), 100);
            fwrite(buffer, 1, bytes, fp);

//            printf("free %p\n", buffer);

            free(buffer);
            fclose(fp);

#endif
#endif            
            free(img_data.data());         

            printf("jpeg thread %d: got job id %d\n", id, img_data.id());
        }
    }
    return 0;
}



// save screenshots as BMP files...
int 
save_screenshot()
{
//    return 1;

    static int shot_nr = 0;

    size_t data_size = globals.win_width * globals.win_height * 3;
    unsigned char *data = (unsigned char *)malloc(data_size);


    glReadPixels(0, 0, globals.win_width, globals.win_height, GL_RGB, 
                 GL_UNSIGNED_BYTE, data);
       
    // flip image vertically
    int rowsize = globals.win_width * 3;
    unsigned char *tmp = new unsigned char[rowsize];
    for (int i = 0; i < globals.win_height / 2; i++) 
    {
        memcpy(tmp, &data[rowsize * (globals.win_height - 1 - i)], rowsize);
        memcpy(&data[rowsize * (globals.win_height - 1 - i)], 
               &data[rowsize * i], 
               rowsize);
        memcpy(&data[rowsize * i], tmp, rowsize);
    }


#if 0
    char filename[32];
    sprintf(filename, "roketz%03d.bmp", shot_nr++);

    // create SDL surface from image data
    SDL_Surface *shot_surf = SDL_CreateRGBSurfaceFrom(data, 
                                                      globals.win_width, 
                                                      globals.win_height,
                                                      24,
                                                      globals.win_width * 3,
                                                      0x0000FF,
                                                      0x00FF00,
                                                      0xFF0000,
                                                      0x000000);
                                         
    SDL_SaveBMP(shot_surf, filename);
    
    SDL_FreeSurface(shot_surf);
    delete data;

#else
    ImageData jpeg_data(data, globals.win_width, globals.win_height, shot_nr++);
    img_queue.postJob(jpeg_data);
#endif

    delete tmp;
    return shot_nr;
}


void
key_released(void *data, int key)
{
    if (key == SDLK_p && game_state == GS_PLAYING)
    {
        paused = !paused;
        if (paused) Mix_Pause(-1);
        else Mix_Resume(-1);
    }
    if (key == SDLK_m)
    {
        play_music = !play_music;
        if (play_music) Mix_ResumeMusic();
        else Mix_PauseMusic();
    } 
    if (key == SDLK_c) rocket1.use_cubemap = !rocket1.use_cubemap;        
    if (key == SDLK_F12) save_screenshot();
}


void gameplay_mainloop(void)
{
    static long last_msec = -1;

    float current_msec = (float)SDL_GetTicks();

    float ticks, last_ticks;
    bool first_pause_frame = true;

    float turn_angle = 0.0;

    last_ticks = SDL_GetTicks();

    reshape(WIDTH, HEIGHT);

//    Mix_SetMusicPosition(120);

    while(!key_down(SDLK_q)) 
    {
        // update key states
        update_input();
        if (paused) {
            if (first_pause_frame)
            {
                print_string(globals.win_width / 2 - 85, 
                             globals.win_height / 2, 2.5,
                             "PAUSED");
                SDL_GL_SwapBuffers();
                first_pause_frame = false;
            }
            last_ticks = (float)SDL_GetTicks();
            continue;
        }
        else first_pause_frame = true;
        
        // update timing

        current_msec += 33.33333333333;
        while ((ticks = (float)SDL_GetTicks()) < current_msec) { }
            
        ticks = (float)SDL_GetTicks() - last_ticks;
        last_ticks = (float)SDL_GetTicks();
        globals.ticks = last_ticks;
//        globals.ticks_per_frame = 40;
        globals.ticks_per_frame = ticks;  
//        printf("%.2f\n", ticks);
        
//        SDL_Delay(40);
        
        // toggle flight modes
        if (key_down(SDLK_r)) toggle_flight_mode();
        
        if (key_down(SDLK_PAGEUP)) turn_angle += 0.05 * ticks;
        if (key_down(SDLK_PAGEDOWN)) turn_angle -= 0.05 * ticks;
        
        // toggle fullscreen
        if (key_down(SDLK_f)) 
        {
            SDL_WM_ToggleFullScreen(surf);
            while(key_down(SDLK_f)) update_input();
        }
        
        if (mode == ROCKET_MODE) 
        {
            rocket1.update(ticks, &camera);
            rocket1.setup_camera();
            
        } 
        else 
        {
            update_viewer(ticks);
            
#if 0
            // camera collision detection, not functional
            sgVec3 new_pos, cam_move;
            sgSubVec3(cam_move, camera.current_pos, camera.previous_pos);
            
//            printf("cam move = "); print_vector(cam_move);
            world.get_current_room()->collide_and_slide(camera.previous_pos, 
                                                        cam_move, 20.0, new_pos, 0, -1);
//            printf("new pos = "); print_vector(new_pos);
            
            sgCopyVec3(camera.current_pos, new_pos);
#endif
            
            camera.setup_modelview();
        }
        rocket1.update_cubemap();
        
        // update viewer world position
        world.update(camera.previous_pos, camera.current_pos);
        
        display();
    
//        save_screenshot();
    
        struct timeval  now;
        struct timezone ttz;
        gettimeofday(&now, &ttz);
        
        long msec = now.tv_sec*1000 + now.tv_usec / 1000;
        
        printf("ms = %d\n", (int)(msec - last_msec));

        last_msec = msec;
    }
}

static FILE* audio_fp = 0;
static bool  audio_dump = false;

static void postmix(void *udata, Uint8 *stream, int len)
{
    if (!audio_dump) return;

//    printf("postmix %d bytes, stream=%d\n", len, *stream);

    fwrite(stream, 1, len, audio_fp);
}


bool
init_audio()
{

    int audio_rate = 44100;
    Uint16 audio_format = AUDIO_S16; /* 16-bit stereo */
    int audio_channels = 2;
    int audio_buffers = 4096;
//    int audio_buffers = 64;

    // open audio device
    if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) 
    {
        printf("Unable to open audio!\n");
        exit(1);
    }

    audio_fp = fopen("audio.raw", "wb");
    if (audio_fp == 0)
    {
        fprintf(stderr, "Unable to open audio out file\n");
        exit(1);
    }

    Mix_SetPostMix(postmix, 0);

    return true;
}



// *****  MAIN  **************************************************************
int 
main(int argc, char** argv)
{
    globals.win_width = WIDTH;
    globals.win_height = HEIGHT;

    globals.q = gluNewQuadric();
    globals.cam = &camera;
    
    // initialize SDL's video subsystem.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) 
    {
        fprintf(stderr, "SDL initialization failed: %s\n",
                SDL_GetError());
	exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    
    if (0 == (surf = SDL_SetVideoMode(globals.win_width, 
                                      globals.win_height, 
#ifdef _WIN32
                                      32, SDL_OPENGL | SDL_FULLSCREEN))) 
#else
                                      32, SDL_OPENGL | SDL_FULLSCREEN))) 
//                                      32, SDL_OPENGL))) 
#endif
    {
        fprintf(stderr, "Video mode set failed: %s\n", SDL_GetError());
        exit(1);
    }

    printf("Roketz3D\n");
    printf("Screen BPP: %d\n\n", SDL_GetVideoSurface()->format->BitsPerPixel);
    printf("Vendor     : %s\n", glGetString(GL_VENDOR));
    printf("Renderer    : %s\n", glGetString(GL_RENDERER));
    printf("GL Version  : %s\n", glGetString(GL_VERSION));
    printf("GLU Version : %s\n", gluGetString(GLU_VERSION));
    printf("Extensions  : %s\n", glGetString(GL_EXTENSIONS));
    printf("\n");

    // check if cube mapping extension is available
    char *ext_str = (char *)glGetString(GL_EXTENSIONS);
    if (NULL == strstr(ext_str, "_ARB_texture_cube_map"))
        rocket1.use_cubemap = false;
    else
        rocket1.use_cubemap = true;


    init_audio();
  
    init_gl();
    init_input();
    init_printer();

    globals.frame = 0;
    globals.lm_mode = 4;
    globals.draw_textures = ~0;
    globals.draw_lightmaps = 0;

    camera.inc_yaw(00.0);

//    set_motion_callback(orient_viewer, &rocket1);
    set_motion_callback(orient_rocket, &rocket1);

    // setup HUD billboards
    fuel_meter.add_image("data/gfx/left3.png");
    shield_meter.add_image("data/gfx/right3.png");

    red0.add_image("data/gfx/red0.png");
    red1.add_image("data/gfx/red1.png");
    orange0.add_image("data/gfx/orange0.png");
    orange1.add_image("data/gfx/orange1.png");
    life.add_image("data/gfx/life.png");

    loadscreen.add_image("data/gfx/loading2.png");

//    SDL_ShowCursor(SDL_DISABLE);

    initialized = true;
 
    // cube texture
//    GLuint id;
    char *data = new char[128 * 128 * 3];
    globals.cube_tex.create_from_data(128, 128, 3, GL_RGB, 
        GL_UNSIGNED_BYTE, data);

//    globals.cube_tex.create_from_file("data/gfx/red0.png");

    set_keyup_callback(key_released, NULL);

    // initialize jpeg writer threads
    const int num_threads = 3;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++)
    {
        threads.push_back(std::thread(jpeg_writer_thread, i));
    }



    int action = 1;
    while (true) 
    {

        if (action == 0) exit(0);
        if (action == 1) 
        {
            glClear(GL_COLOR_BUFFER_BIT);

#if 0
            loadscreen.draw_ortho(0, 0, globals.win_width, globals.win_height);

            
            center_string(10, 2.5, "FUELING UP...");
            SDL_GL_SwapBuffers();  

            SDL_Delay(2);
#endif
            world.init();
            rocket1.in_room = world.get_current_room();
            world.start_vp[1] -= 40.0;

#if 1
            bool start = false;
            int blink = 0;
            int blink_ticks;
            
            bool blink_on = false;

            while (!start) 
            {            
                loadscreen.draw_ortho(0, 0, globals.win_width, globals.win_height);
                
                center_string(650.0f, 1.5f, "CODING AND DESIGN");
                center_string(610.0f, 2.2f, "LARS ANDERSSON");

                center_string(530.0f, 1.5f, "ARTWORK AND DESIGN");
                center_string(490.0f, 2.2f, "CHRISTOFER SANDIN");



                if (blink > 10) 
                {
                    blink_on = !blink_on;
                    blink -= 10;
                }
                blink++;

                if (blink_on)
                    center_string(10.0f, 1.6f, "PRESS SPACE TO TAKE OFF");
                SDL_GL_SwapBuffers();  

//                save_screenshot();

                blink_ticks = SDL_GetTicks();
                while ((SDL_GetTicks() - blink_ticks) < 33) 
                {
                    if (key_down(SDLK_SPACE)) start = true; 
                    if (key_down(SDLK_q)) goto quit;
                    update_input();
                }
            }
#endif
            rocket1.init();
            rocket1.prepare_for_takeoff();

            audio_dump = true;
            gameplay_mainloop();

//            world.free(); not implemented
            reset_keys();
            action = 0;
        }
    }
quit:
    Mix_CloseAudio();
    SDL_Quit();
        
    img_queue.sync();
    all_done = true;

    img_queue.wakeAllConsumers();

    for (auto &t : threads)
    {
        t.join();
    }

    exit(0);
}


    
