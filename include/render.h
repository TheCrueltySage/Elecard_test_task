#ifndef __TT_RENDER_INCLUDED__
#define __TT_RENDER_INCLUDED__
#include <cstdint>
#include <ios>
#include <atomic>
#include <string>
#include <GLFW/glfw3.h>

void img_over_video (uint8_t*, uint8_t*, uint8_t*, uint8_t*, std::streamsize, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int = 0, unsigned int = 0);
void img_over_frame (uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, unsigned int, unsigned int, unsigned int, unsigned int);

void render_video(uint8_t*, std::streamsize, unsigned int, unsigned int, unsigned int);

//Reusable render_video subfunctions
void glfw_error_callback(int, const char*);
GLFWwindow* create_empty_opengl_window (unsigned int, unsigned int, const char*);
void make_rectangle_data (GLuint*, GLuint*, GLuint*);
void build_shader_program (const GLchar*, const GLchar*, GLuint&, GLuint&, GLuint&, const GLchar*);
void gen_yuv_to_rgb_matrix_uniform (GLuint&, const GLchar*);
//Non-reusable render_video subfunctions
inline void gen_yuv_textures_uniform_with_pbo(GLuint*, GLuint*, uint8_t*, std::streamsize, GLuint&, const GLchar*, const GLchar*, const GLchar*);
inline void download_pbo_yuv_textures (unsigned long long, unsigned long long, unsigned long long, unsigned int, unsigned int);

#endif
