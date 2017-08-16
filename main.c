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
using namespace std;

void help();

int main(int argc, char *argv[])
{
    int c;
    string input_filename, image_filename, output_filename = "";
    unsigned int width = 0, height = 0, frames = 0;
    static int render_flag = 0;

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
	    {"help",		no_argument,		    0, 'h'}
	};

	int index = 0;
	c = getopt_long (argc, argv, "i:a:o:W:H:F:h", long_options, &index);

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
	streamsize size = get_size(input_filename);
	if (size == -1)
	{
	    cerr << "Error opening the original video. Aborting." << endl;
	    return 0;
	}
	uint8_t *vbuffer = new uint8_t[size];
	bool check;
	check = file_to_memory(input_filename, reinterpret_cast<char*>(vbuffer), size);
	if (!check)
	{
	    cerr << "Error opening the original video. Aborting." << endl;
	    return 0;
	}
	if (image_filename != "")
	{
	    bmp_holder bmpdata (image_filename);
	    if ((width==0)||(height==0))
	    {
		cerr << "Error: must define video dimensions to merge with image" << endl;
		return 0;
	    }
	    if ((bmpdata.height<height)||(bmpdata.width<width))
	    {
		cerr << "Error: image must be smaller in dimensions than video" << endl;
		return 0;
	    }
	    unsigned int threadnum = thread::hardware_concurrency();
	    if (threadnum==0)
		threadnum=4;
	    thread t[threadnum];
	    unsigned int threadrows = ceil(bmpdata.height/threadnum);
	    if (threadrows%2==1)
		threadrows++;
	    uint8_t** ycoord = new uint8_t*[threadnum];
	    uint8_t** ucoord = new uint8_t*[threadnum];
	    uint8_t** vcoord = new uint8_t*[threadnum];
	    unsigned long yuvlen = threadrows*bmpdata.width;
	    for (unsigned int i=0;i<threadnum;i++)
	    {
		unsigned int rownum = i*threadrows;
		ycoord[i] = new uint8_t[yuvlen];
	    	ucoord[i] = new uint8_t[yuvlen/4];
	    	vcoord[i] = new uint8_t[yuvlen/4];
		t[i] = thread(rgb_to_yuv_thread,ref(bmpdata),rownum,threadrows,ycoord[i],ucoord[i],vcoord[i]);
	    }
	    uint8_t* i_ybuffer = new uint8_t[bmpdata.width*bmpdata.height];
	    unsigned long abspos = 0;
	    for (unsigned int i=0;i<threadnum;i++)
	    {
	        t[i].join();
	        for (unsigned int j=0;j<yuvlen;j++)
	        {
	            if (((i*yuvlen)+j)>=(bmpdata.width*bmpdata.height))
	        	break;
	            i_ybuffer[abspos]=ycoord[i][j];
	            abspos++;
	        }
	        delete [] ycoord[i];
	    }
	    delete [] ycoord;
	    uint8_t* i_ubuffer = new uint8_t[(bmpdata.width*bmpdata.height)/4];
	    abspos = 0;
	    for (unsigned int i=0;i<threadnum;i++)
	    {
	        for (unsigned int j=0;j<(yuvlen/4);j++)
	        {
	            if (((i*(yuvlen/4))+j)>=((bmpdata.width*bmpdata.height)/4))
	        	break;
	            i_ubuffer[abspos]=ucoord[i][j];
	            abspos++;
	        }
	    	delete [] ucoord[i];
	    }
	    delete [] ucoord;
	    uint8_t* i_vbuffer = new uint8_t[(bmpdata.width*bmpdata.height)/4];
	    abspos = 0;
	    for (unsigned int i=0;i<threadnum;i++)
	    {
	        for (unsigned int j=0;j<(yuvlen/4);j++)
	        {
	            if (((i*(yuvlen/4))+j)>=((bmpdata.width*bmpdata.height)/4))
	        	break;
	            i_vbuffer[abspos]=vcoord[i][j];
	            abspos++;
	        }
	    	delete [] vcoord[i];
	    }
	    delete [] vcoord;
	    img_over_video(vbuffer,i_ybuffer,i_ubuffer,i_vbuffer,size,width,height,bmpdata.width,bmpdata.height);
	}
	if (render_flag)
	{
	    if (frames==0)
	    {
		cerr << "Error: must define video framerate to render" << endl;
		return 0;
	    }
	    //will do later
	}
	check = memory_to_file(output_filename, reinterpret_cast<char*>(vbuffer), size);
	if (!check)
	{
	    cerr << "Error writing the output file." << endl;
	}
    }
    else
    {
	cerr << "Need at least two arguments: input video and output file" << endl;
	help();
	return 0;
    }
}

void help()
{
    cout << "yuv_to_rgb [options]" << endl <<
	"Options:" << endl <<
        "-h | --help		Print this help" << endl <<
        "-i | --input-video	The path to input YUV420 video" << endl <<
        "-o | --output-video	Where to put the resulting video" << endl <<
        "-a | --appended-image	The path to the RGM BMP image to append to video" << endl <<
        "-W | --width		Width of input video in pixels" << endl <<
        "-H | --height		Height of input video in pixels" << endl <<
        "-F | --framerate	Framerate of input video in frames per second" << endl <<
        "-r | --render         	Open a separate window and render the resulting video to it" << endl;
}
	    //for (unsigned int i=0;i<threadnum;i++)
	    //{
	    //    t[i].join();
	    //}
	    //for (unsigned int i=0;i<threadnum;i++)
	    //{
	    //    delete [] ycoord[i];
	    //	delete [] ucoord[i];
	    //	delete [] vcoord[i];
	    //}
	    //delete [] ycoord;
	    //delete [] ucoord;
	    //delete [] vcoord;
