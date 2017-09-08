#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gl_core_3_3.h"
#include "render.h"

//Overlays image over video, supposed to be spawned in a thread.
//This actually takes only four buffers, the first one serving as continous buffer for the entire video. The last three are coordinate buffers for the image.
//Also takes size of video (to figure out amount of frames), and dimensions of both video and image.
//Passes most of the parameters through to img_over_frame unchanged.
//Compatible with threading: uses the last two arguments (number of overall threads and number of thread it was spawned in) to evenly distribute the work by itself.
void img_over_video (uint8_t* vbuffer, uint8_t* i_ycoord, uint8_t* i_ucoord, uint8_t* i_vcoord, std::streamsize vsize, unsigned int vwidth, unsigned int vheight, unsigned int iwidth, unsigned int iheight, unsigned int threads, unsigned int division)
{
    unsigned long long framelen = (vwidth*vheight)+((vwidth*vheight)/2);
    unsigned long total_frames = vsize/framelen;
    unsigned long frames;
    if (threads==0)
        frames = total_frames;
    else
        frames = std::ceil(static_cast<float>(total_frames)/threads);
    uint8_t *yholder,*uholder,*vholder;
    for (unsigned long i=0;i<frames;i++)
    {
        //Casting streamsize to unsigned to suppress a warning. Since streamsize is very rarely negative, this should be mostly safe.
        //Oh, also this thing checks whether we passed the last element of video buffer. This will probably happen once, in last spawned thread,
        //in case work is distributed unevenly.
        if ((framelen*(division*frames+i+1))>=static_cast<unsigned long long>(vsize))
            break;
        yholder=&vbuffer[framelen*(division*frames+i)];
        uholder=yholder+(vwidth*vheight);
        vholder=uholder+((vwidth*vheight)/4);
        img_over_frame(yholder,uholder,vholder,i_ycoord,i_ucoord,i_vcoord,vwidth,vheight,iwidth,iheight);
    }
}

//Overlays image over frame of video (or larger image, in case that's what you want).
//Takes six buffers in total (first three for frame, last three for image that we overlay). Also, dimensions of video and image.
//Overlays everything in the middle, but this can be changed by modifying woffset and hoffset (they could be parameters too, in principle,
//but this would pollute parameter list even more).
void img_over_frame (uint8_t* v_ybuffer, uint8_t* v_ubuffer, uint8_t* v_vbuffer, uint8_t* i_ycoord, uint8_t* i_ucoord, uint8_t* i_vcoord, unsigned int vwidth, unsigned int vheight, unsigned int iwidth, unsigned int iheight)
{
    unsigned int wdiff = vwidth-iwidth;
    unsigned int woffset = wdiff/2;
    if (woffset%2!=0)
        woffset++;
    unsigned int hoffset = (vheight-iheight)/2;
    if (hoffset%2!=0)
        hoffset++;
    unsigned long abspos = 0;
    unsigned long vidpos = 0;
    unsigned long impos = 0;
    unsigned long i_ypos = 0;
    abspos += hoffset*vwidth+woffset;
    for (unsigned int i=0;i<iheight;i+=2)
    {
        for (unsigned int j=0;j<iwidth;j+=2)
        {
            v_ybuffer[abspos]=i_ycoord[i_ypos];
            vidpos = ((hoffset+i)/2)*(vwidth/2)+((woffset+j)/2);
            impos = (i/2)*(iwidth/2)+(j/2);
            v_ubuffer[vidpos]=i_ucoord[impos];
            v_vbuffer[vidpos]=i_vcoord[impos];
            i_ypos++;
            abspos++;
            if (j>=iwidth)
                break;
            v_ybuffer[abspos]=i_ycoord[i_ypos];
            i_ypos++;
            abspos++;
        }
        abspos+=wdiff;
        if (i>=iheight)
            break;
        for (unsigned int j=0;j<iwidth;j++)
        {
            v_ybuffer[abspos]=i_ycoord[i_ypos];
            i_ypos++;
            abspos++;
        }
        abspos+=wdiff;
    }
}


void render_video(uint8_t* vbuffer, std::streamsize size, unsigned int framerate, unsigned int width, unsigned int height)
{
    //Shader code
    //Keeping it in main rendering function for easy reference
    const GLchar* vertex_code = 
        R"glsl(
        #version 330 core
        in vec2 position;
        in vec2 texcoord;
        out vec2 Texcoord;
        void main()
        {
            Texcoord = texcoord;
            gl_Position = vec4(position,0.0,1.0);
        }
        )glsl";
    const GLchar* fragment_code = 
        R"glsl(
        #version 330 core
        in vec2 Texcoord;
        out vec4 endcolor;
        uniform sampler2D ybuffer;
        uniform sampler2D ubuffer;
        uniform sampler2D vbuffer;
        uniform mat4 rgb_matrix;
        void main()
        {
            float y = texture(ybuffer,Texcoord).x;
            float u = texture(ubuffer,Texcoord).x;
            float v = texture(vbuffer,Texcoord).x;
            endcolor = clamp(rgb_matrix*vec4(y,u,v,1.0f),0.0f,1.0f);
        }
        )glsl";

    //Create an empty opengl window with glfw
    glfwSetErrorCallback(glfw_error_callback);
    GLFWwindow* window = create_empty_opengl_window(width, height, "YUV video render");

    //Create vertex array, vertices and elements that draw a rectangle continously
    GLuint vao, vbo, ebo;
    make_rectangle_data(&vao,&vbo,&ebo);

    //Building a shader program from earlier code
    GLuint vshader, fshader, sprogram;
    try
    {
        build_shader_program(vertex_code,fragment_code,vshader,fshader,sprogram,"endcolor");
    }
    catch (char error)
    {
        //There was an error, show the log and gracefully exit
        GLint logsize = 1;
        std::vector<GLchar> errorLog(logsize);
        if (error=='v')
        {
            glGetShaderiv(vshader,GL_INFO_LOG_LENGTH,&logsize);
            errorLog.resize(logsize);
            glGetShaderInfoLog(vshader,logsize,&logsize,&errorLog[0]);
            std::cerr << "Compilation error: ";
        }
        else if (error=='f')
        {
            glGetShaderiv(fshader,GL_INFO_LOG_LENGTH,&logsize);
            errorLog.resize(logsize);
            glGetShaderInfoLog(fshader,logsize,&logsize,&errorLog[0]);
            std::cerr << "Compilation error: ";
        }
        else if (error=='l')
        {
            glGetProgramiv(sprogram,GL_INFO_LOG_LENGTH,&logsize);
            errorLog.resize(logsize);
            glGetProgramInfoLog(sprogram,logsize,&logsize,&errorLog[0]);
            std::cerr << "Linking error: ";
        }
        else
            return;
        for (auto i:errorLog)
            std::cerr << i;
        std::cerr << std::endl;
        glDeleteProgram(sprogram);
        glDeleteShader(fshader);
        glDeleteShader(vshader);
        glDeleteBuffers(1,&ebo);
        glDeleteBuffers(1,&vbo);
        glDeleteVertexArrays(1,&vao);
        glfwTerminate();
        return;
    }

    //Binding attributes to shader program
    GLint posattr = glGetAttribLocation(sprogram,"position");
    glEnableVertexAttribArray(posattr);
    glVertexAttribPointer(posattr,2,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),0);
    GLint texattr = glGetAttribLocation(sprogram,"texcoord");
    glEnableVertexAttribArray(texattr);
    glVertexAttribPointer(texattr,2,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),reinterpret_cast<void*>(2*sizeof(GLfloat)));

    //Generating pbo and binding 3 textures to uniforms
    GLuint pbo;
    GLuint textures[3];
    gen_yuv_textures_uniform_with_pbo(&pbo,textures,vbuffer,size,sprogram,"ybuffer","ubuffer","vbuffer");

    gen_yuv_to_rgb_matrix_uniform(sprogram,"rgb_matrix"); //Generating a YUV to RGB conversion matrix to use in fragment shader

    //Clearing the screen
    glClearColor(0.0f,0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //Variable initialisation
    unsigned long long framelen = width*height+((width*height)/2); //Frame length and number of frames
    unsigned long frames = size/framelen;
    unsigned long long yholder,uholder,vholder; //Offsets to current textures in pbo
    double start_frame; //Timekeepers, to watch over the framerate
    double finished_frame;
    double elapsed_time;
    double eps = 0.001;
    //double real_error = 0;

    //Beginning first loop
    yholder=0;
    uholder=yholder+(width*height);
    vholder=uholder+((width*height)/4);
    download_pbo_yuv_textures (yholder, uholder, vholder, width, height); //This function uploads 3 textures from given pbo offsets
    start_frame = glfwGetTime();
    for (unsigned long i=1;i<frames;i++)
    {
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0); //Draw a rectangle

        glfwPollEvents(); //Look for user input events
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) //Escape exits the program
            return;

        finished_frame = glfwGetTime();
        elapsed_time = finished_frame-start_frame; //How much time we spent on this frame

        //If we spent more than required time with eps precision, then change the frame to the next one by uploading new textures
        if (elapsed_time>((1.0/framerate)-eps))
        {
            //real_error += elapsed_time-(1.0/framerate);
            //std::cout << "Frame: " << i << " Time spent: " << elapsed_time << " Error: " << real_error << std::endl;
            start_frame = glfwGetTime();
            yholder=framelen*i;
            uholder=yholder+(width*height);
            vholder=uholder+((width*height)/4);
            download_pbo_yuv_textures (yholder, uholder, vholder, width, height);
        }
        else
            i--; //Frame didn't change, so keeping frame number the same
        
        glfwSwapBuffers(window); //Swap front and back buffers; this action doesn't return until new frame is rendered on the screen
                                 //Subject to vertical synchronisation
                                 //So it's the only reason we're not busy waiting in the loop

        //Log rendering errors
        GLenum render_error = glGetError();
        if (render_error != GL_NO_ERROR)
        {
            std::cerr << "OpenGL error: " << render_error << std::endl;
        }
    }
    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0); //Drawing the last frame
    glfwSwapBuffers(window);

    glDeleteTextures(3,textures); //Cleaning up the resources
    glDeleteProgram(sprogram);
    glDeleteShader(fshader);
    glDeleteShader(vshader);
    glDeleteBuffers(1,&pbo);
    glDeleteBuffers(1,&ebo);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
    glfwTerminate();
}

//Error callback for glfw
void glfw_error_callback(int error, const char* description)
{
    std::cerr << "Error code: " << error << std::endl << "Description: " << description << std::endl;
    abort();
}

//Creates an empty opengl window
GLFWwindow* create_empty_opengl_window (unsigned int width, unsigned int height, const char* name)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); //Context version is 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE); //Profile is core
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE); //Forward-compatible
    glfwWindowHint(GLFW_RESIZABLE,GL_FALSE); //Non-resizable window

    GLFWwindow* window = glfwCreateWindow(width, height, name, nullptr, nullptr);
    glfwMakeContextCurrent(window);
    return window;
}

//Draws a rectangle taking up the whole screen, every frame
void make_rectangle_data (GLuint* vao, GLuint* vbo, GLuint* ebo)
{
    glGenVertexArrays(1,vao);
    glBindVertexArray(*vao);

    glGenBuffers(1,vbo);

    //Rectangle vertices
    GLfloat vertices[] = {
        //  Position        Texcoords
            -1.0f,  1.0f,   0.0f,   0.0f,   //Top-left corner
            1.0f,   1.0f,   1.0f,   0.0f,   //Top-right corner
            1.0f,   -1.0f,  1.0f,   1.0f,   //Bottom-right corner
            -1.0f,  -1.0f,  0.0f,   1.0f,   //Bottom-left corner
    };

    glBindBuffer(GL_ARRAY_BUFFER,*vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1,ebo);

    //Rectangle draw order
    GLuint elements[] = {
        0,  1,  2,
        2,  3,  0
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,*ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(elements),elements,GL_STATIC_DRAW);
}

//Compiles and links a shader program from given sources, output is outname
void build_shader_program (const GLchar* vertex_code, const GLchar* fragment_code, GLuint& vshader, GLuint& fshader, GLuint& sprogram, const GLchar* outname)
{
    GLint success = 0;
    vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader,1,&vertex_code,NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader,GL_COMPILE_STATUS,&success);
    if (success==GL_FALSE)
        throw 'v';

    fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader,1,&fragment_code,NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader,GL_COMPILE_STATUS,&success);
    if (success==GL_FALSE)
        throw 'f';

    sprogram = glCreateProgram();
    glAttachShader(sprogram,vshader);
    glAttachShader(sprogram,fshader);
    glBindFragDataLocation(sprogram,0,outname);
    glLinkProgram(sprogram);
    glUseProgram(sprogram);
    glGetProgramiv(sprogram,GL_LINK_STATUS,&success);
    if (success==GL_FALSE)
        throw 'l';
}

//Program-specific function that creates a Pixel Buffer Object and three textures and binds them to uniforms in shaders
inline void gen_yuv_textures_uniform_with_pbo(GLuint* pbo, GLuint* textures, uint8_t* vbuffer, std::streamsize size, GLuint& sprogram, const GLchar* yname, const GLchar* uname, const GLchar* vname)
{
    glGenBuffers(1,pbo);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,*pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER,size,vbuffer,GL_STATIC_DRAW);

    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    glGenTextures(3,textures);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,textures[0]);
    glUniform1i(glGetUniformLocation(sprogram,yname),0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,textures[1]);
    glUniform1i(glGetUniformLocation(sprogram,uname),1);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,textures[2]);
    glUniform1i(glGetUniformLocation(sprogram,vname),2);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
}

//Function that creates YUV to RGB transformation matrix and builds it to given uniform
void gen_yuv_to_rgb_matrix_uniform (GLuint& sprogram, const GLchar* matname)
{
    glm::mat4 trans, scale, colorshift;
    trans = glm::translate(glm::mat4(1.0),glm::vec3(-0.0625f,-0.5f,-0.5f));
    float scarr[16] = {
        1.164,  0,      1.596, 0,
        1.164,  -0.391, -0.813, 0,
        1.164,  2.018,  0,      0,
        0,      0,      0,      0
    };
    scale = glm::transpose(glm::make_mat4(scarr));
    colorshift = scale*trans;
    GLint uniMatrix = glGetUniformLocation(sprogram, matname);
    glUniformMatrix4fv(uniMatrix,1,GL_FALSE,glm::value_ptr(colorshift));
}

//Program-specific function that changes three active textures to positions given by offsets in pbo
inline void download_pbo_yuv_textures (unsigned long long yholder, unsigned long long uholder, unsigned long long vholder, unsigned int width, unsigned int height)
{
    glActiveTexture(GL_TEXTURE0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width,height,0,GL_RED,GL_UNSIGNED_BYTE,reinterpret_cast<void*>(yholder));
    glActiveTexture(GL_TEXTURE1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width/2,height/2,0,GL_RED,GL_UNSIGNED_BYTE,reinterpret_cast<void*>(uholder));
    glActiveTexture(GL_TEXTURE2);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width/2,height/2,0,GL_RED,GL_UNSIGNED_BYTE,reinterpret_cast<void*>(vholder));
}
