#ifndef __TT_CONV_INCLUDED__
#define __TT_CONV_INCLUDED__
#include "bmp.h"

int clip(int, int, int);
void rgb_to_yuv_thread (bmp_holder&, unsigned int, unsigned int, uint8_t*, uint8_t*, uint8_t*);
void rgb_to_yuv_simple (unsigned long, unsigned int, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*);
void rgb_to_yuv_vector (unsigned long, unsigned int, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*);

#endif
