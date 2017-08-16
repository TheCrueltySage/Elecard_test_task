#include <iostream>
#include <cstddef>
#include <cstring>
#include <cmath>
#include "fs.h"
#include "bmp.h"
using namespace std;

bmp_holder::bmp_holder(string const & filename)
{
    try {
	//A quick check for magic number in the beginning.
    	//If it's not "BM", no point in reading the rest.
    	char filecheck[3];
    	if (!file_to_memory(filename, filecheck, 2, 0))
	    throw;
    	filecheck[2] = 0;
    	if (strcmp(filecheck,"BM") != 0)
    	{
    	    throw "Error: not a BMP file or ill-formatted header.";
    	}
    	if (!file_to_memory(filename,reinterpret_cast<char*>(&fileinfo),14))
	    throw;
    	//Is the filesize shorter than our two headers should be?
    	if (fileinfo.bfSize <= 55)
    	{
    	    throw "Error: malformed file - stated filesize less than headers' minimal summary size";
    	}
    	if (!file_to_memory(filename,reinterpret_cast<char*>(&imageinfo),40,14)) //getting the second header
	    throw;
    	//Checking for unexpected field values
    	if ((imageinfo.biBitCount != 24) && (imageinfo.biBitCount != 32))
    	{
    	    throw "Error: BMP file not RGB; conversion for different color formats not implemented";
    	}
    	if (imageinfo.biCompression != 0)
    	{
    	    throw "Error: compressed file, but decompression not implemented";
    	}
    	rgbsize = (((imageinfo.biBitCount)*(imageinfo.biWidth)+31)/32)*abs(imageinfo.biHeight)*4;
    	rgbdata = new uint8_t[rgbsize];
    	if (!file_to_memory(filename,reinterpret_cast<char*>(rgbdata),rgbsize,fileinfo.bfOffBytes)) //getting the bitmap data
	    throw;
	width = imageinfo.biWidth;
	height = abs(imageinfo.biHeight);
    }
    catch (const char* msg)
    {
	cerr << msg << endl;
    }
    catch (...)
    {
	cerr <<"Unable to handle BMP." << endl;
    }
}

bmp_holder::~bmp_holder()
{
    delete [] rgbdata;
}

void bmp_holder::get_rgb_row(unsigned int number, uint8_t* red, uint8_t* green, uint8_t* blue)
{
    unsigned int row_size = width; //How big the array we need to hold 
    int row_jump = rgbsize/height; //How big are rows with padding
    unsigned int cur; //Where the row we're processing in rgbdata begins
    if (imageinfo.biHeight>0)
    {
	//Height is positive, jumping from end to beginning by internal row length
	//We're beginning at the end row
	cur = rgbsize-row_jump;
	//We're jumping backwards, so the amount we jump by is negative
	row_jump = row_jump*(-1);
    }
    else
    {
	//Height is negative, jumping from beginning to end by internal row length
	//We're beginning at the first row
	cur = 0;
    }
    if (number >= height)
    {
	cout << "Error: row number does not exist" << endl;
	return;
    }
    for (unsigned int i=0;i<number;i++)
	cur += row_jump;
    unsigned int pixelnum = 0;
    unsigned int rowpos = 0;
    while (pixelnum < row_size)
    {
	blue[pixelnum] = rgbdata[cur+rowpos];
	rowpos++;
	green[pixelnum] = rgbdata[cur+rowpos];
	rowpos++;
	red[pixelnum] = rgbdata[cur+rowpos];
	rowpos++;
        if (imageinfo.biBitCount == 32)
	   rowpos++;
	pixelnum++;
    }
    return;
}

//Probably not gonna go that way. Still, keeping the function just in case.
//
//void bmp_holder::rgb_to_yuv(int threads)
//{
//    unsigned int row_size = imageinfo.biWidth*3; //How big the array we need to hold 
//						 //rgb values row once we strip everything
//    int row_jump = (((imageinfo.biBitCount)*(imageinfo.biWidth)+31)/32)*4; //How big are 
//								    //rows with padding
//    unsigned int cur; //Where the row we're processing in rgbdata begins
//    unsigned int end; //Where is the position of the last row
//    int* rgb_holder = new int[row_size]; //Array to hold the current row
//
//    if (imageinfo.biHeight>0)
//    {
//	//Height is positive, jumping from end to beginning by internal row length
//	//We're beginning at the end row
//	cur = rgbsize-row_jump;
//	//We're ending at the first row
//	end = 0;
//	//We're jumping backwards, so the amount we jump by is negative
//	row_jump = row_jump*(-1);
//    }
//    else
//    {
//	//Height is negative, jumping from beginning to end by internal row length
//	//We're beginning at the first row
//	cur = 0;
//	//We're ending at the last row
//	end = rgbsize-row_jump;
//    }
//
//    //Constructing a correct row of rgb values - correct rgb positions and no padding.
//    //Afterwards we feed the row to a thread which processes it into YUV.
//    //This takes CPU cycles and memory, but makes it possible to easily extend class
//    //for other RGB configurations.
//    //Not really necessary, but the infrastructure was already there, so I went for it.
//    while (cur != end)
//    {
//	unsigned int newrowpos = 2; //position in rgb_holder - where to copy final rgb
//	unsigned int oldrowpos = 0; //relative position in rgbdata - where we're copying from
//	//Setting up lines can in theory be concurrent, but this might lead to
//	//a "false sharing" problem, so I leave it alone until proven it's worth it
//	while (newrowpos < row_size)
//	{
//	    for (int i=0; i<3; i++)
//	    {
//		rgb_holder[newrowpos] = rgbdata[cur+oldrowpos];
//	    	oldrowpos++;
//	    	newrowpos = newrowpos-1;
//	    }
//	    oldrowpos++;
//	    newrowpos+=3;
//	    if (imageinfo.biBitCount == 32)
//		oldrowpos++;
//	}
//	cur += row_jump;
//    }
//
//    delete [] row_holder;
//    return;
//}
//
