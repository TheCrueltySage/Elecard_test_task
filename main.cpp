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
    //unsigned int width = 0, height = 0, frames = 0;
    //static int render_flag = 0;
    unsigned int width = 0, height = 0, threadnum = 0;

    while (1)
    {
	static struct option long_options[] =
	{
	    //{"render",		no_argument,		    &render_flag, 1},
	    {"input-video",	required_argument,	    0, 'i'},
	    {"appended-image",	required_argument,	    0, 'a'},
	    {"output",		required_argument,	    0, 'o'},
	    {"width",		required_argument,	    0, 'W'},
	    {"height",		required_argument,	    0, 'H'},
	    //{"framerate",	required_argument,	    0, 'F'},
	    {"threads",		required_argument,	    0, 't'},
	    {"help",		no_argument,		    0, 'h'}
	};

	int index = 0;
	//c = getopt_long (argc, argv, "i:a:o:W:H:F:h", long_options, &index);
	c = getopt_long (argc, argv, "i:a:o:W:H:t:h", long_options, &index);

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
	    case 'o':
		threadnum = optarg;
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
    	if (threadnum!=1)
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
	//Remains here for possible second bonus task.
	//if (render_flag)
	//{
	//    if (frames==0)
	//    {
	//	std::cerr << "Error: must define video framerate to render" << std::endl;
	//	return 0;
	//    }
	//    //will do later
	//}
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

    uint8_t* i_ybuffer = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* i_ubuffer = new uint8_t[(bmpdata.width*bmpdata.height)/4];
    uint8_t* i_vbuffer = new uint8_t[(bmpdata.width*bmpdata.height)/4];

    //Setting up the threads
    if (threadnum>bmpdata.height)
        threadnum=bmpdata.height;

    if ((threadnum==0)||(threadnum==1))
    {
	rgb_to_yuv_thread(bmpdata,rownum,threadrows,ycoord,ucoord,vcoord);
    	img_over_video(vbuffer,i_ybuffer,i_ubuffer,i_vbuffer,size,width,height,bmpdata.width,bmpdata.height);
    }
    else
    {
	std::thread* t = new std::thread[threadnum];

    	unsigned int threadrows = std::ceil(static_cast<float>(bmpdata.height)/static_cast<float>(threadnum));
    	if (threadrows%2==1)
    	    threadrows++;
	for (unsigned int i=0;i<threadnum;i++)
    	{
    	    unsigned int rownum = i*threadrows;
    	    ycoord = &i_ybuffer[rownum*bmpdata.width];
    	    ucoord = &i_ubuffer[rownum*(bmpdata.width/4)];
    	    vcoord = &i_vbuffer[rownum*(bmpdata.width/4)];
    	    if (rownum<bmpdata.height)
    	        t[i] = std::thread(rgb_to_yuv_thread,std::ref(bmpdata),rownum,threadrows,ycoord,ucoord,vcoord); //Actual function that does stuff
    	}
	//Overlaying image on video via threads. Technically could be done without threads, but this is actually the bottleneck of the program,
    	//profiling shows it gives about 4 times the speedup, 
    	//and it was easy to write due to threads being set up earlier for RGB->YUV conversion.
    	//Also, since the buffer is by default pretty large, we're really unlikely to run into false sharing.
    	for (unsigned int i=0;i<threadnum;i++)
    	    t[i] = std::thread(img_over_video_thread,vbuffer,i_ybuffer,i_ubuffer,i_vbuffer,size,threadnum,i,width,height,bmpdata.width,bmpdata.height);
    	for (unsigned int i=0;i<threadnum;i++)
    	    t[i].join();

	delete [] t;
    }

    delete [] i_ybuffer;
    delete [] i_ubuffer;
    delete [] i_vbuffer;
}

void help()
{
    std::cout << "yuv_to_rgb [options]" << std::endl <<
	"Options:" << std::endl <<
        "-h | --help		Print this help" << std::endl <<
        "-i | --input-video	The path to input YUV420 video" << std::endl <<
        "-o | --output-video	Where to put the resulting video" << std::endl <<
        "-a | --appended-image	The path to the RGM BMP image to append to video" << std::endl <<
        //"-r | --render        Open a separate window and render the resulting video to it" << std::endl <<
        //"-F | --framerate	Framerate of input video in frames per second" << std::endl <<
        "-W | --width		Width of input video in pixels" << std::endl <<
        "-H | --height		Height of input video in pixels" << std::endl;
}
