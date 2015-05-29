
#ifndef BINFILE_H
#define BINFILE_H

class BinaryFile
{
    int fd;
 public:
    void read_bytes(char *buf, int nr);
    int read_byte();
    int read_short();
    int read_int();
    float read_float();

    void read_vector(float *v);
    void read_matrix(float *v);

    int	read_string(char *dst, int max);

    void read_compressed_bytes(char *buf, int total);
    void read_compressed_shorts(short *buf, int total);

    void skip_bytes(int bytes);
    bool eof(void);

    void seek(long int offs);
    long int tell();

    bool open_file(const char *fname);
    void close_file();
};

#endif
