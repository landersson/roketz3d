
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

extern "C" {

#include <png.h>

#include <zlib.h>

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
    png_buf.buffer = (unsigned char *)malloc(8192);
    png_buf.size = 8192;
    png_buf.offset = 0;


    /* setup custom writer functions */
    png_set_write_fn(png_ptr, (voidp)&png_buf, png_mem_write_data, NULL);

    int zcompression = Z_DEFAULT_COMPRESSION;

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

int main()
{
    FILE *fp = fopen("kalle.png", "wb");

    const int width = 1280;
    const int height = 960;

    uint8_t* data = (uint8_t *)malloc(width * height * 3);

    memset(data, 0, width * height * 3);
    
    uint8_t *buffer;
    unsigned bytes = compress_png(&buffer, width, height,
                                  data, 100);
    fwrite(buffer, 1, bytes, fp);
    
    free(data);
    free(buffer);
    fclose(fp);

}
