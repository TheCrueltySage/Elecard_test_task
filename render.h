#ifndef __TT_RENDER_INCLUDED__
#define __TT_RENDER_INCLUDED__
#include <cstdint>
#include <ios>

void img_over_video (uint8_t*, uint8_t*, uint8_t*, uint8_t*, std::streamsize, unsigned int, unsigned int, unsigned int, unsigned int);
void img_over_frame (uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, unsigned int, unsigned int, unsigned int, unsigned int);

#endif
