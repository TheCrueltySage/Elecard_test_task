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
    	if (imageinfo.biBitCount == 16)
    	{
    	    throw "Error: 16-bit RGB not supported";
    	}
    	if ((imageinfo.biBitCount != 24) && (imageinfo.biBitCount != 32))
    	{
    	    throw "Error: BMP file not RGB; conversion for different color formats not supported";
    	}
    	if (imageinfo.biCompression != 0)
    	{
    	    throw "Error: compressed file, but decompression not supported";
    	}
	//The size of one row in bmp file. Formula taken straight from Microsoft.
    	rgbsize = (((imageinfo.biBitCount)*(imageinfo.biWidth)+31)/32)*abs(imageinfo.biHeight)*4;
    	rgbdata = new uint8_t[rgbsize];
    	if (!file_to_memory(filename,reinterpret_cast<char*>(rgbdata),rgbsize,fileinfo.bfOffBytes)) //getting the bitmap data
	    throw;
	//Copying variables so that users don't have to wade through fileheader structs for simple information.
	width = imageinfo.biWidth;
	//Height can also be negative in some cases, and that means things about structure of BMP.
	//We hide that, since this variable is more for outside use than for parsing BMP files.
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

//Puts one row of image into three arrays (red, green and blue). Accounts for line reversal in postitive-height BMP files.
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
