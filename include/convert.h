#ifndef __TT_CONV_INCLUDED__
#define __TT_CONV_INCLUDED__
#include "bmp.h"

int clip(int, int, int);
void do_rgb_to_yuv (bmp_holder&, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, unsigned int = 0, unsigned int = 0);
void rgb_to_yuv_simple (unsigned int, unsigned int, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*);
void rgb_to_yuv_vector (unsigned int, unsigned int, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*);

#endif
