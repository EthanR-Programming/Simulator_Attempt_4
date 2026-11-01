#ifndef text_h
#define text_h

unsigned char *text_load_char(unsigned char *ttf_buffer,int *width,int *height,unsigned char character,int pixel_height,int *advance,int *yoffset, int*xoffset , unsigned char colour[3]);
void font_init(unsigned char *ttf_buffer, int height);

#endif