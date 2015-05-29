#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/glu.h>

class Texture
{
    bool        valid;
    int		width, height;
//    int		bytes_per_pixel;

    GLuint	tex_id;
    char	*data;
 public:
    Texture() { valid = false; }
    void create_from_data(int w, int h, GLint components, 
                          GLenum format, GLenum type, void *data);
    bool create_from_file(const char *fname);
    void set_gl_id(GLuint id) { tex_id = id; }
    unsigned get_gl_id() { return tex_id; }
    void bind() { glBindTexture(GL_TEXTURE_2D, tex_id); }
};

#endif
