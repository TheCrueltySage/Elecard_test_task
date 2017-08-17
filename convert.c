#include <algorithm>
#include <thread>
#include "convert.h"
#include "bmp.h"
using namespace std;

int clip(int n, int lower, int upper) {
  return max(lower, min(n, upper));
}

//Specific function to set up program-specific requirements for RGB->YUV conversion in threads. Not designed to be reused.
void rgb_to_yuv_thread (bmp_holder &bmpdata, unsigned int rownum, unsigned int dorows, uint8_t* ycoord, uint8_t* ucoord, uint8_t* vcoord)
{
    unsigned long rgblen = dorows*bmpdata.width;
    uint8_t* red = new uint8_t[rgblen];
    uint8_t* green = new uint8_t[rgblen];
    uint8_t* blue = new uint8_t[rgblen];
    unsigned int realdorows = dorows;
    for (unsigned int i=0;i<dorows;i++)
    {
    	bmpdata.get_rgb_row((rownum+i),&red[bmpdata.width*i],&green[bmpdata.width*i],&blue[bmpdata.width*i]);
	if ((rownum+i+1)==bmpdata.height)
	{
	    realdorows = i;
	    break;
	}
    }
    rgblen = realdorows*bmpdata.width;
    rgb_to_yuv_simple(rgblen,bmpdata.width,red,green,blue,ycoord,ucoord,vcoord);
    delete [] red;
    delete [] green;
    delete [] blue;
}

//Non-vector conversion from RGB to YUV, compatible with threads.
void rgb_to_yuv_simple (unsigned long pixelnum, unsigned int width, uint8_t* red, uint8_t* green, uint8_t* blue, uint8_t* ycoord, uint8_t* ucoord, uint8_t* vcoord)
{
    const float ycoeff[3] = { 0.299, 0.587, 0.114 };
    const float ucoeff[3] = { -0.147, -0.289, 0.436 };
    const float vcoeff[3] = { 0.615, -0.515, -0.100 };
    unsigned int offset = 0; //which position on the row we're on; assuming we begin from start of row
    unsigned int row = 0; //which row we're in; assuming we begin from an even row (if we count from 0)
    for (unsigned long i=0;i<pixelnum;i++)
    {
	ycoord[i]=clip((ycoeff[0]*red[i]+ycoeff[1]*green[i]+ycoeff[2]*blue[i]),0,255);
	//U and V only get written once for every 4 pixels
	if ((offset%2==0)&&(row%2==0))
	{
	    int arrpos = (row/2)*(width/2)+(offset/2); //width/2 because once we change rows, we have to add half of elements of previous row
	    ucoord[arrpos]=clip((ucoeff[0]*red[i]+ucoeff[1]*green[i]+ucoeff[2]*blue[i]+128),0,255);
	    vcoord[arrpos]=clip((vcoeff[0]*red[i]+vcoeff[1]*green[i]+vcoeff[2]*blue[i]+128),0,255);
	}
	offset++;
	if (offset >= width)
	{
	    offset = 0;
	    row++;
	}
    }
}

//Vector conversion from RGB to YUV, compatible with threads.
//void rgb_to_yuv_vector (unsigned long pixelnum, unsigned int width, uint8_t* red, uint8_t* green, uint8_t* blue, uint8_t* ycoord, uint8_t* ucoord, uint8_t* vcoord)
//{
//    //const float ycoeff[3] = { 0.299, 0.587, 0.114 };
//    //const float ucoeff = 0.713;
//    //const float vcoeff = 0.564;
//    //int offset = 0; //which position on the row we're on; assuming we begin from start of row
//    //int row = 0; //which row we're in; assuming we begin from an even row (if we count from 0)
//    //for (long i=0;i<pixelnum;i++)
//    //{
//    //    ycoord[i]=ycoeff[0]*red[i]+ycoeff[1]*green[i]+ycoeff[2]*blue[i];
//    //    int offset = i%width;
//    //    int row = i/width;
//    //    if ((offset%2==0)&&(row%2==0))
//    //    {
//    //        int arrpos = (row/2)*(width/2)+(offset/2); //width/2 because once we change rows, we have to add half of elements of previous row
//    //        ucoord[arrpos] = ucoeff*(red[i]-ycoord[i]);
//    //        vcoord[arrpos] = vcoeff*(blue[i]-ycoord[i]);
//    //    }
//    //    offset++;
//    //    if (offset >= width)
//    //    {
//    //        offset = 0;
//    //        row++;
//    //    }
//    //}
//}
