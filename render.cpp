#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>
#include "gl_core_3_2.h"
#include "render.h"

//Overlays image over video, supposed to be spawned in a thread.
//This actually takes only four buffers, the first one serving as continous buffer for the entire video. The last three are coordinate buffers for the image.
//Also takes size of video (to figure out amount of frames), and dimensions of both video and image.
//Passes most of the parameters through to img_over_frame unchanged.
//Uses the number of overall threads and number of thread it was spawned in to evenly distribute the work by itself.
void img_over_video_thread (uint8_t* vbuffer, uint8_t* i_ycoord, uint8_t* i_ucoord, uint8_t* i_vcoord, std::streamsize vsize, unsigned int threads, unsigned int division, unsigned int vwidth, unsigned int vheight, unsigned int iwidth, unsigned int iheight)
{
    unsigned long long framelen = (vwidth*vheight)+((vwidth*vheight)/2);
    unsigned long total_frames = vsize/framelen;
    unsigned long frames = std::ceil(static_cast<float>(total_frames)/threads);
    uint8_t *yholder,*uholder,*vholder;
    for (unsigned long i=0;i<frames;i++)
    {
        //Casting streamsize to unsigned to suppress a warning. Since streamsize is very rarely negative, this should be mostly safe.
        //Oh, also this thing checks whether we passed the last element of video buffer. This will probably happen once, in last spawned thread,
        //in case work is distributed unevenly.
        if ((framelen*(division*frames+i)+(vwidth*vheight)+((vwidth*vheight)/2))>=static_cast<unsigned long long>(vsize))
            break;
        yholder=&vbuffer[framelen*(division*frames+i)];
        uholder=yholder+(vwidth*vheight);
        vholder=uholder+((vwidth*vheight)/4);
        img_over_frame(yholder,uholder,vholder,i_ycoord,i_ucoord,i_vcoord,vwidth,vheight,iwidth,iheight);
    }
}

//Overlays image over video.
//This actually takes only four buffers, the first one serving as continous buffer for the entire video. The last three are coordinate buffers for the image.
//Also takes size of video (to figure out amount of frames), and dimensions of both video and image.
//Passes most of the parameters through to img_over_frame unchanged.
void img_over_video (uint8_t* vbuffer, uint8_t* i_ycoord, uint8_t* i_ucoord, uint8_t* i_vcoord, std::streamsize vsize, unsigned int vwidth, unsigned int vheight, unsigned int iwidth, unsigned int iheight)
{
    unsigned long framelen = (vwidth*vheight)+((vwidth*vheight)/2);
    unsigned long frames = vsize/framelen;
    uint8_t *yholder,*uholder,*vholder;
    for (unsigned long i=0;i<frames;i++)
    {
        yholder=&vbuffer[framelen*i];
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

void init_window(uint8_t* vbuffer, std::streamsize size, unsigned int framerate, unsigned int width, unsigned int height)
{
    glfwSetErrorCallback(glfw_error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,2);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE,GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(width, height, "YUV video render", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    std::atomic<bool> rfinish;
    rfinish = false;
    std::thread rthread = std::thread(render_video_thread,window,vbuffer,size,framerate,width,height,std::ref(rfinish));

    while (!glfwWindowShouldClose(window))
    {
        glfwWaitEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            return;
    }
    rfinish = true;
    rthread.join();
    glfwTerminate();
}

void glfw_error_callback(int error, const char* description)
{
    std::cerr << "Error code: " << error << std::endl << "Description: " << description << std::endl;
    //abort();
}

void render_video_thread(GLFWwindow* window, uint8_t* vbuffer, std::streamsize size, unsigned int framerate, unsigned int width, unsigned int height, std::atomic<bool>& rfinish)
{
    const GLchar* vertex_code = 
        R"glsl(
        #version 150 core
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
        #version 150 core
        in vec2 Texcoord;
        out vec4 endcolor;
        uniform sampler2D ybuffer;
        uniform sampler2D ubuffer;
        uniform sampler2D vbuffer;
        void main()
        {
            red = texture(ybuffer,TexCoord);
            green = texture(ybuffer,TexCoord);
            blue = texture(ybuffer,TexCoord);
            endcolor = vector4(1.0,1.0,1.0,1.0);
        }
        )glsl";

    unsigned long long framelen = width*height+((width*height)/2);
    unsigned long frames = size/framelen;
    unsigned long long yholder,uholder,vholder;

    GLuint vao;
    glGenVertexArrays(1,&vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1,&vbo);

    //Rectangle vertices
    GLfloat vertices[] = {
        //  Position        Texcoords
            -1.0f,  1.0f,   0.0f,   0.0f,   //Top-left corner
            1.0f,   1.0f,   1.0f,   0.0f,   //Top-right corner
            1.0f,   -1.0f,  1.0f,   1.0f,   //Bottom-right corner
            -1.0f,  -1.0f,  0.0f,   1.0f,   //Bottom-left corner
    };

    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1,&ebo);

    //Rectangle draw order
    GLuint elements[] = {
        0,  1,  2,
        2,  3,  0
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(elements),elements,GL_STATIC_DRAW);

    GLint success = 0;
    GLint logsize = 0;

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    std::cerr << "Current window: " << window << std::endl;
    std::cerr << "Current context: " << glfwGetCurrentContext() << std::endl;
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error: " << error << std::endl;
        return;
    }
    glShaderSource(vshader,1,&vertex_code,NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader,GL_COMPILE_STATUS,&success);
    if (success==GL_FALSE)
    {
        glGetShaderiv(vshader,GL_INFO_LOG_LENGTH,&logsize);
        std::vector<GLchar> errorLog(logsize);
        glGetShaderInfoLog(vshader,logsize,&logsize,&errorLog[0]);
        std::cerr << "Compilation error: ";
        for (auto i:errorLog)
            std::cerr << i;
        std::cerr << std::endl;
        glDeleteShader(vshader);
        glDeleteBuffers(1,&ebo);
        glDeleteBuffers(1,&vbo);
        glDeleteVertexArrays(1,&vao);
        glfwSetWindowShouldClose(window,GL_TRUE);
        return;
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader,1,&fragment_code,NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader,GL_COMPILE_STATUS,&success);
    if (success==GL_FALSE)
    {
        glGetShaderiv(fshader,GL_INFO_LOG_LENGTH,&logsize);
        std::vector<GLchar> errorLog(logsize);
        glGetShaderInfoLog(fshader,logsize,&logsize,&errorLog[0]);
        std::cerr << "Compilation error: ";
        for (auto i:errorLog)
            std::cerr << i;
        std::cerr << std::endl;
        glDeleteShader(fshader);
        glDeleteShader(vshader);
        glDeleteBuffers(1,&ebo);
        glDeleteBuffers(1,&vbo);
        glDeleteVertexArrays(1,&vao);
        glfwSetWindowShouldClose(window,GL_TRUE);
        return;
    }

    GLuint sprogram = glCreateProgram();
    glAttachShader(sprogram,vshader);
    glAttachShader(sprogram,fshader);
    glBindFragDataLocation(sprogram,0,"endcolor");
    glLinkProgram(sprogram);
    glUseProgram(sprogram);
    glGetProgramiv(sprogram,GL_LINK_STATUS,&success);
    if (success==GL_FALSE)
    {
        glGetProgramiv(sprogram,GL_INFO_LOG_LENGTH,&logsize);
        std::vector<GLchar> errorLog(logsize);
        glGetProgramInfoLog(sprogram,logsize,&logsize,&errorLog[0]);
        std::cerr << "Linking error: ";
        for (auto i:errorLog)
            std::cerr << i;
        std::cerr << std::endl;
        glDeleteProgram(sprogram);
        glDeleteShader(fshader);
        glDeleteShader(vshader);
        glDeleteBuffers(1,&ebo);
        glDeleteBuffers(1,&vbo);
        glDeleteVertexArrays(1,&vao);
        glfwSetWindowShouldClose(window,GL_TRUE);
        return;
    }

    GLint posattr = glGetAttribLocation(sprogram,"position");
    glEnableVertexAttribArray(posattr);
    glVertexAttribPointer(posattr,2,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),0);

    GLint texattr = glGetAttribLocation(sprogram,"texcoord");
    glEnableVertexAttribArray(texattr);
    glVertexAttribPointer(texattr,2,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),reinterpret_cast<void*>(2*sizeof(GLfloat)));

    GLuint pbo;
    glGenBuffers(1,&pbo);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER,size,vbuffer,GL_STATIC_DRAW);

    glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    GLuint textures[3];
    glGenTextures(3,textures);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,textures[0]);
    glUniform1i(glGetUniformLocation(sprogram,"ybuffer"),0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,textures[1]);
    glUniform1i(glGetUniformLocation(sprogram,"ubuffer"),1);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,textures[2]);
    glUniform1i(glGetUniformLocation(sprogram,"vbuffer"),2);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    glClearColor(0.0f,0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    double start_frame = glfwGetTime();
    yholder=0;
    uholder=yholder+(width*height);
    vholder=uholder+((width*height)/4);
    glActiveTexture(GL_TEXTURE0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width,height,0,GL_RED,GL_UNSIGNED_BYTE,reinterpret_cast<void*>(yholder));
    glActiveTexture(GL_TEXTURE1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width/2,height/2,0,GL_RED,GL_UNSIGNED_BYTE,reinterpret_cast<void*>(uholder));
    glActiveTexture(GL_TEXTURE2);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width/2,height/2,0,GL_RED,GL_UNSIGNED_BYTE,reinterpret_cast<void*>(vholder));
    glFinish();
    double finished_frame = glfwGetTime();
    double elapsed_time = finished_frame-start_frame;
    std::cout << "Time spent on first texture fetch: " << elapsed_time << std::endl;
    start_frame = glfwGetTime();
    for (unsigned long i=1;i<frames;i++)
    {
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

        yholder=framelen*i;
        uholder=yholder+(width*height);
        vholder=uholder+((width*height)/4);
        glActiveTexture(GL_TEXTURE0);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width,height,0,GL_RED,GL_UNSIGNED_BYTE,reinterpret_cast<void*>(yholder));
        glActiveTexture(GL_TEXTURE1);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width/2,height/2,0,GL_RED,GL_UNSIGNED_BYTE,reinterpret_cast<void*>(uholder));
        glActiveTexture(GL_TEXTURE2);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R8,width/2,height/2,0,GL_RED,GL_UNSIGNED_BYTE,reinterpret_cast<void*>(vholder));

        glFinish();
        finished_frame = glfwGetTime();
        elapsed_time = finished_frame-start_frame;
        std::cout << "Time spent: " << elapsed_time << std::endl;
        if (rfinish.load()==true)
            break;
        if (elapsed_time<(1.0/framerate))
            std::this_thread::sleep_for(std::chrono::duration<double>((1.0/framerate)-elapsed_time));
        if (rfinish.load()==true)
            break;

        start_frame = glfwGetTime();

        glfwSwapBuffers(window);
    }
    glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
    finished_frame = glfwGetTime();
    elapsed_time = finished_frame-start_frame;
    if (elapsed_time<(1.0/framerate))
        std::this_thread::sleep_for(std::chrono::duration<double>((1.0/framerate)-elapsed_time));

    glDeleteTextures(3,textures);
    glDeleteProgram(sprogram);
    glDeleteShader(fshader);
    glDeleteShader(vshader);
    glDeleteBuffers(1,&pbo);
    glDeleteBuffers(1,&ebo);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
    glfwSetWindowShouldClose(window,GL_TRUE);
}
