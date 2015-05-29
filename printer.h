
#ifndef PRINTER_H
#define PRINTER_H

void init_printer();
void center_string(float y, float size, const char *str);
void print_string(float x, float y, float size, const char *str);
void draw_string(float x, float y, float size, const char *str);
void print_matrix(const sgMat4 m);
void print_vector(const sgVec3 v);
void set_text_color(float r, float g, float b);
void get_letter_size(char ch, float size, float *w, float *h);
float get_string_size(const char *str, float size, float *sw, float *sh);


#endif
