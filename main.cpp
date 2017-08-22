#include <iostream>
#include <string>
#include <getopt.h>
#include <ios>
#include <cstdlib>
#include <cmath>
#include <thread>
#include "fs.h"
#include "bmp.h"
#include "convert.h"
#include "render.h"

void img_overlay(uint8_t*, std::string, unsigned int, unsigned int, std::streamsize, unsigned int);
void help();

int main(int argc, char *argv[])
{
    //Using CLI. See help() down below for details.
    int c;
    std::string input_filename, image_filename, output_filename = "";
    unsigned int width = 0, height = 0, frames = 0, threadnum = 0;
    static int render_flag = 0;
    bool threadset = false;

    while (1)
    {
	static struct option long_options[] =
	{
	    {"render",		no_argument,		    &render_flag, 1},
	    {"input-video",	required_argument,	    0, 'i'},
	    {"appended-image",	required_argument,	    0, 'a'},
	    {"output",		required_argument,	    0, 'o'},
	    {"width",		required_argument,	    0, 'W'},
	    {"height",		required_argument,	    0, 'H'},
	    {"framerate",	required_argument,	    0, 'F'},
	    {"threads",		required_argument,	    0, 't'},
	    {"help",		no_argument,		    0, 'h'}
	};

	int index = 0;
	c = getopt_long (argc, argv, "i:a:o:W:H:F:t:h", long_options, &index);

	if (c==-1)
	{
	    if (optind == 1)
	    {
		help();
	    	return 0;
	    }
	    else
		break;
	}
	
	switch (c)
	{
	    case 0:
		break;
	    case 'i':
		input_filename = optarg;
		break;
	    case 'a':
		image_filename = optarg;
		break;
	    case 'o':
		output_filename = optarg;
		break;
	    case 'W':
		width = atoi(optarg);
		break;
	    case 'H':
		height = atoi(optarg);
		break;
	    case 't':
		threadnum = atoi(optarg);
		threadset = true;
		break;
	    case 'F':
		frames = atoi(optarg);
		break;
	    case 'h':
	    case '?':
		help();
		return 0;
	    default:
		help();
		abort();
	}
    }

    if (input_filename != "" && output_filename != "")
    {
    	if (threadset!=true)
	    threadnum = std::thread::hardware_concurrency();
	std::streamsize size = get_size(input_filename);
	if (size == -1)
	{
	    std::cerr << "Error opening the original video. Aborting." << std::endl;
	    return 0;
	}
	uint8_t *vbuffer = new uint8_t[size];
	bool check;
	check = file_to_memory(input_filename, reinterpret_cast<char*>(vbuffer), size);
	if (!check)
	{
	    std::cerr << "Error opening the original video. Aborting." << std::endl;
	    return 0;
	}
	if (image_filename != "")
	{
	    img_overlay(vbuffer, image_filename, width, height, size, threadnum);
	}
	if (render_flag)
	{
	    if (frames==0)
	    {
		std::cerr << "Error: must define video framerate to render" << std::endl;
		return 0;
	    }
	    render_video(vbuffer,size,frames,width,height);
	}
	check = memory_to_file(output_filename, reinterpret_cast<char*>(vbuffer), size);
	if (!check)
	{
	    std::cerr << "Error writing the output file." << std::endl;
	}
	delete [] vbuffer;
    }
    else
    {
	std::cerr << "Need at least two arguments: input video and output file" << std::endl;
	help();
	return 0;
    }
}

//Pretty much part of main, but put into function for readability.
//Allocates dynamic memory since it needs it to be dynamic size. Make sure that it gets rid of it.
void img_overlay(uint8_t* vbuffer, std::string image_filename, unsigned int width, unsigned int height, std::streamsize size, unsigned int threadnum)
{
    //bmp_holder opens and parses a bmp file on its own in constructor.
    bmp_holder bmpdata (image_filename);
    if ((width==0)||(height==0))
    {
	std::cerr << "Error: must define video dimensions to merge with image" << std::endl;
        return;
    }
    if ((height<bmpdata.height)||(width<bmpdata.width))
    {
	std::cerr << "Error: image must be smaller in dimensions than video" << std::endl;
        return;
    }

    //Allocating (requires dynamic size)
    uint8_t* red = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* green = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* blue = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* i_ybuffer = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* i_ubuffer = new uint8_t[(bmpdata.width*bmpdata.height)/4];
    uint8_t* i_vbuffer = new uint8_t[(bmpdata.width*bmpdata.height)/4];

    //Setting up the threads
    if (threadnum>bmpdata.height)
        threadnum=bmpdata.height;
    if ((threadnum==0)||(threadnum==1))
    {
	do_rgb_to_yuv(bmpdata,red,green,blue,i_ybuffer,i_ubuffer,i_vbuffer);
    	img_over_video(vbuffer,i_ybuffer,i_ubuffer,i_vbuffer,size,width,height,bmpdata.width,bmpdata.height);
    }
    else
    {
	//Allocating thread (need dynamic amount)
	std::thread* t = new std::thread[threadnum];

    	unsigned int threadrows = std::ceil(static_cast<float>(bmpdata.height)/static_cast<float>(threadnum));
    	if (threadrows%2==1)
    	    threadrows++;
	for (unsigned int i=0;i<threadnum;i++)
    	{
    	    unsigned int rownum = i*threadrows;
    	    uint8_t* rcoord = &red[rownum*bmpdata.width];
    	    uint8_t* gcoord = &green[rownum*bmpdata.width];
    	    uint8_t* bcoord = &blue[rownum*bmpdata.width];
    	    uint8_t* ycoord = &i_ybuffer[rownum*bmpdata.width];
    	    uint8_t* ucoord = &i_ubuffer[rownum*(bmpdata.width/4)];
    	    uint8_t* vcoord = &i_vbuffer[rownum*(bmpdata.width/4)];
    	    if (rownum<bmpdata.height)
    	        t[i] = std::thread(do_rgb_to_yuv,std::ref(bmpdata),rcoord,gcoord,bcoord,ycoord,ucoord,vcoord,threadrows,rownum); //Actual function that does stuff
    	}
    	for (unsigned int i=0;i<threadnum;i++)
    	    t[i].join();
	//Overlaying image on video via threads. Technically could be done without threads, but this is actually the bottleneck of the program,
    	//profiling shows it gives about 4 times the speedup, 
    	//and it was easy to write due to threads being set up earlier for RGB->YUV conversion.
    	//Also, since the buffer is by default pretty large, we're really unlikely to run into false sharing.
    	for (unsigned int i=0;i<threadnum;i++)
    	    t[i] = std::thread(img_over_video,vbuffer,i_ybuffer,i_ubuffer,i_vbuffer,size,width,height,bmpdata.width,bmpdata.height,threadnum,i);
    	for (unsigned int i=0;i<threadnum;i++)
    	    t[i].join();

	//Deallocating threads
	delete [] t;
    }

    //Deallocating memory that was allocated in this function
    delete [] i_ybuffer;
    delete [] i_ubuffer;
    delete [] i_vbuffer;
    delete [] red;
    delete [] green;
    delete [] blue;
}

void help()
{
    std::cout << "yuv_to_rgb [options]" << std::endl <<
	"Options:" << std::endl <<
        "-h | --help		Print this help" << std::endl <<
        "-i | --input-video	The path to input YUV420 video" << std::endl <<
        "-o | --output-video	Where to put the resulting video" << std::endl <<
        "-a | --appended-image	The path to the RGM BMP image to append to video" << std::endl <<
        "--render		Open a separate window and render the resulting video to it" << std::endl <<
        "-F | --framerate	Framerate of input video in frames per second" << std::endl <<
        "-W | --width		Width of input video in pixels" << std::endl <<
        "-H | --height		Height of input video in pixels" << std::endl <<
        "-t | --threads		Force certain amount of threads used (default is dependent on your hardware configuration)" << std::endl;
}
