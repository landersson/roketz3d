
#include "first.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "binfile.h"

void
BinaryFile::read_bytes(char *buf, int nr)
{
    read(fd, buf, nr);
}

int
BinaryFile::read_byte()
{
    char tmp;
    read_bytes((char *)&tmp, 1);
    return tmp;
}

int
BinaryFile::read_short()
{
    short tmp;
    read_bytes((char *)&tmp, 2);
    return tmp;
}

int
BinaryFile::read_int()
{
    int tmp;
    read_bytes((char *)&tmp, 4);
    return tmp;
}

float
BinaryFile::read_float()
{
    float f;
    read_bytes((char *)&f, 4);
    return f;
}

void 
BinaryFile::read_vector(float *v)
{
    v[0] = read_float();
    v[1] = read_float();
    v[2] = read_float();
}

void 
BinaryFile::read_matrix(float *v)
{
    read_vector(&v[0]);
    read_vector(&v[3]);
    read_vector(&v[6]);
}


int
BinaryFile::read_string(char *dst, int max)
{
    int len = 0;
    while (len < max) 
    {
	read_bytes(&dst[len], 1);
	if (dst[len] == 0) return len + 1;
	len++;
    }
    return len;
}

void
BinaryFile::read_compressed_bytes(char *buf, int total)
{
    char compressed = read_byte();

    if (!compressed) 
    {
	read_bytes(buf, total);
	return;
    }

    int count = 0;
    while (count < total) 
    {
	unsigned char cmd = read_byte();

	if (cmd == 0) 
        {
	    // next byte is raw byte
	    buf[count++] = read_byte();
	}
	else if (cmd >= 2 && cmd <= 250) 
        {
	    // run of 'cmd' similar bytes
	    char byte = read_byte();
	    
	    for (int i = 0; i < cmd; i++) 
            {
		buf[count++] = byte;
	    }
	}
	else assert(0);
    }
}

void
BinaryFile::read_compressed_shorts(short *buf, int total)
{
    int i;
    char compressed = read_byte();

    if (!compressed) 
    {
        for (i = 0; i < total; i++) 
        {
            buf[i] = read_short();
        }
	return;
    }

    int count = 0;
    while (count < total) 
    {
        assert(count < total);
	unsigned char cmd = read_byte();

	if (cmd == 0) 
        {
	    // next byte is raw byte
	    buf[count++] = read_short();
	}
	else if (cmd >= 2 && cmd <= 250) 
        {
	    // run of 'cmd' similar bytes
	    short value = read_short();
	    
	    for (i = 0; i < cmd; i++) 
            {
		buf[count++] = value;
	    }
	}
	else assert(0); // bad compression
    }
}



void 
BinaryFile::skip_bytes(int bytes)
{
    lseek(fd, bytes, SEEK_CUR);
}

bool
BinaryFile::eof(void)
{
    int cp = lseek(fd, 0, SEEK_CUR);
    int end = lseek(fd, 0, SEEK_END);
    if (cp == end) 
    {
	return true;
    }
    else 
    {
	lseek(fd, cp, SEEK_SET);
	return false;
    }
}

void
BinaryFile::seek(long int offs)
{
    lseek(fd, offs, SEEK_SET);
}

long int
BinaryFile::tell()
{
    return lseek(fd, 0, SEEK_CUR);
}


bool
BinaryFile::open_file(const char *fname)
{
#ifdef _WIN32
    fd = open(fname, O_RDONLY | O_BINARY);
#else    
    fd = open(fname, O_RDONLY);    
#endif
    if (fd != -1) return true;
    else return false;
}

void
BinaryFile::close_file()
{
    if (fd != -1) close(fd);
}

