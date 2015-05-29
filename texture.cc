
#include "first.h"
#include <stdio.h>
#include <string.h>
//#include <malloc.h>
#include <assert.h>

#include <GL/glu.h>
#include <GL/glext.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "texture.h"

void
Texture::create_from_data(int w, int h, GLint components, 
                          GLenum format, GLenum type, 
                          void *tex_data)
{
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    gluBuild2DMipmaps(GL_TEXTURE_2D, components, w, h,
                      format, type, tex_data);

    glTexImage2D(GL_TEXTURE_2D, 0, components, w, h, 0,
           format, type, tex_data);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                    GL_NEAREST_MIPMAP_NEAREST);
    

    valid = true;
    width = w;
    height = h;
    data = (char *)tex_data;
}

bool
Texture::create_from_file(const char *fname)
{
    SDL_Surface *surf;

    surf = IMG_Load(fname);

    if(!surf) {
	fprintf(stderr, "texture::create_from_file: %s\n", IMG_GetError());
	return false;
    }
#if 0
    printf("image loaded:\nsize:   %d x %d\n", surf->w, surf->h);
    printf("bytes_per_pixel: %d\n", surf->format->BytesPerPixel);
    printf("bits_per_pixel: %d\n", surf->format->BitsPerPixel);
    printf("rmask, bmask, gmask, amask: %x, %x, %x, %x\n", 
	   surf->format->Rmask,
	   surf->format->Gmask,
	   surf->format->Bmask,
	   surf->format->Amask);
#endif

    int bytes_per_pixel = surf->format->BytesPerPixel;

    width = surf->w;
    height = surf->h;

    assert(bytes_per_pixel == 3 || bytes_per_pixel == 4);

    data = new char[width * height * bytes_per_pixel];
    assert(data);

    SDL_LockSurface(surf);
    memcpy(data, surf->pixels, width * height * bytes_per_pixel);
    SDL_UnlockSurface(surf);

#if 1
    // create OpenGL texture
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int gl_format;
    if (surf->format->Rmask > 0xFF) 
    {
        gl_format = (bytes_per_pixel == 3 ? GL_BGR : GL_BGRA);
    }
    else 
    {
        gl_format = (bytes_per_pixel == 3 ? GL_RGB : GL_RGBA);
    }

    gluBuild2DMipmaps(GL_TEXTURE_2D, bytes_per_pixel, width, height,
		      gl_format, GL_UNSIGNED_BYTE, data);
    
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
#endif

    SDL_FreeSurface(surf);
    return true;
}
