#include <algorithm>
#include <thread>
#include <emmintrin.h>
#include "convert.h"
#include "bmp.h"

int clip(int n, int lower, int upper) {
  return std::max(lower, std::min(n, upper));
}

//Specific function to set up program-specific requirements for RGB->YUV conversion in threads. Not designed to be reused.
void rgb_to_yuv_thread (bmp_holder &bmpdata, unsigned int rownum, unsigned int dorows, uint8_t* ycoord, uint8_t* ucoord, uint8_t* vcoord)
{
    unsigned long rgblen = dorows*bmpdata.width;
    uint8_t *red = new uint8_t[rgblen];
    uint8_t *green = new uint8_t[rgblen];
    uint8_t *blue = new uint8_t[rgblen];
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
    #ifdef __SSE2__
      rgb_to_yuv_vector(realdorows,bmpdata.width,red,green,blue,ycoord,ucoord,vcoord);
    #else
      rgb_to_yuv_simple(realdorows,bmpdata.width,red,green,blue,ycoord,ucoord,vcoord);
    #endif
    delete [] red;
    delete [] green;
    delete [] blue;
}

//Non-vector conversion from RGB to YUV, compatible with threads.
void rgb_to_yuv_simple (unsigned int height, unsigned int width, uint8_t* red, uint8_t* green, uint8_t* blue, uint8_t* ycoord, uint8_t* ucoord, uint8_t* vcoord)
{
    const int16_t ycoeff[3] = { 66, 129, 25 };
    const int16_t ucoeff[3] = { -38, -74, 112 };
    const int16_t vcoeff[3] = { 112, -94, -18 };
    unsigned int curpos=0;
    unsigned int arrpos=0;
    for (unsigned int i=0;i<height;i+=2)
    {
	for (unsigned int j=0;j<width;j+=2)
	{
	    curpos = i*width+j;
            arrpos = (i/2)*(width/2)+(j/2); //width/2 because once we change rows, we have to add half of elements of previous row
	    ycoord[curpos] = clip((((ycoeff[0]*red[curpos]+ycoeff[1]*green[curpos]+ycoeff[2]*blue[curpos]+128)>>8)+16),0,255);
            //U and V only get written once every 2 pixels in this row
            ucoord[arrpos]=clip((((ucoeff[0]*red[curpos]+ucoeff[1]*green[curpos]+ucoeff[2]*blue[curpos]+128)>>8)+128),0,255);
            vcoord[arrpos]=clip((((vcoeff[0]*red[curpos]+vcoeff[1]*green[curpos]+vcoeff[2]*blue[curpos]+128)>>8)+128),0,255);
            if (j >= width)
		break;
            ycoord[curpos+1] = clip((((ycoeff[0]*red[curpos+1]+ycoeff[1]*green[curpos+1]+ycoeff[2]*blue[curpos+1]+128)>>8)+16),0,255);
	}
	for (unsigned int j=0;j<width;j++)
	{
	    curpos = (i+1)*width+j;
	    ycoord[curpos] = clip((((ycoeff[0]*red[curpos]+ycoeff[1]*green[curpos]+ycoeff[2]*blue[curpos]+128)>>8)+16),0,255);
	}
    }
}

//Vector conversion from RGB to YUV, compatible with threads.
//Written in SSE2.
void rgb_to_yuv_vector (unsigned int height, unsigned int width, uint8_t* red, uint8_t* green, uint8_t* blue, uint8_t* ycoord, uint8_t* ucoord, uint8_t* vcoord)
{
    __m128i rvec, gvec, bvec, yvec, uvec, vvec, rcoeffvec, gcoeffvec, bcoeffvec, offcoeffvec;
    const int16_t ycoeff[3] = { 66, 129, 25 };
    const int16_t ucoeff[3] = { -38, -74, 112 };
    const int16_t vcoeff[3] = { 112, -94, -18 };
    offcoeffvec = _mm_set1_epi16(128);
    for (unsigned int i=0;i<height;i+=2)
    {
	unsigned long curpos = i*width;
	for (unsigned int j=0;j<width;j+=16)
	{
	    int arrpos = (i/2)*(width/2)+(j/2);

	    //Calculating Y.
	    //Loading data.
	    rvec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&red[curpos]));
	    gvec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&green[curpos]));
	    bvec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&blue[curpos]));
	    rcoeffvec = _mm_set1_epi16(ycoeff[0]);
	    gcoeffvec = _mm_set1_epi16(ycoeff[1]);
	    bcoeffvec = _mm_set1_epi16(ycoeff[2]);

	    //Splitting Y into even and odd parts through some bitshifts. They will be handled separately.
	    //We need that since SIMD instructions mostly don't concern themselves with 8-bit integers. So we need to pretend we're 16-bit.
	    //That's actually pretty handy for multiplication purposes.
	    //Also handy in that we actually need odd parts only to calculate U and V.
	    __m128i rvec_e = _mm_srli_epi16(rvec,8);
	    __m128i gvec_e = _mm_srli_epi16(gvec,8);
	    __m128i bvec_e = _mm_srli_epi16(bvec,8);
	    __m128i rvec_o = _mm_srli_epi16(_mm_slli_epi16(rvec,8),8);
	    __m128i gvec_o = _mm_srli_epi16(_mm_slli_epi16(gvec,8),8);
	    __m128i bvec_o = _mm_srli_epi16(_mm_slli_epi16(bvec,8),8);

	    //Multiplication results for RGB vectors.
	    __m128i rvec_er = _mm_mullo_epi16(rvec_e,rcoeffvec);
	    __m128i gvec_er = _mm_mullo_epi16(gvec_e,gcoeffvec);
	    __m128i bvec_er = _mm_mullo_epi16(bvec_e,bcoeffvec);
	    __m128i rvec_or = _mm_mullo_epi16(rvec_o,rcoeffvec);
	    __m128i gvec_or = _mm_mullo_epi16(gvec_o,gcoeffvec);
	    __m128i bvec_or = _mm_mullo_epi16(bvec_o,bcoeffvec);

	    //Adding up RGB vectors, then 128 to all elements, then bitshifting to the right by 8.
	    //Make sure unsigned functions are used. Otherwise there will be loss of data due to buffer overflow.
	    __m128i e_result = _mm_adds_epu16(_mm_srli_epi16(_mm_adds_epu16(_mm_adds_epu16(_mm_adds_epu16(rvec_er,gvec_er),bvec_er),offcoeffvec),8),_mm_set1_epi16(16));
	    __m128i o_result = _mm_adds_epu16(_mm_srli_epi16(_mm_adds_epu16(_mm_adds_epu16(_mm_adds_epu16(rvec_or,gvec_or),bvec_or),offcoeffvec),8),_mm_set1_epi16(16));

	    //Merging the vectors into one vector through some bitshifts and "or" operation. Afterwards, storing.
	    yvec = _mm_or_si128(_mm_slli_epi16(e_result,8), _mm_srli_epi16(_mm_slli_epi16(o_result,8),8));
	    _mm_storeu_si128(reinterpret_cast<__m128i*>(&ycoord[curpos]),yvec);


	    //Calculating U.
	    rcoeffvec = _mm_set1_epi16(ucoeff[0]);
	    gcoeffvec = _mm_set1_epi16(ucoeff[1]);
	    bcoeffvec = _mm_set1_epi16(ucoeff[2]);

	    //We only need to multiply odd RGB parts here.
	    __m128i rvec_u = _mm_mullo_epi16(rvec_o,rcoeffvec);
	    __m128i gvec_u = _mm_mullo_epi16(gvec_o,gcoeffvec);
	    __m128i bvec_u = _mm_mullo_epi16(bvec_o,bcoeffvec);

	    //Adding up vectors.
	    //Make sure signed function are used here, otherwise there will be loss of data due to buffer underflow,
	    //since U and V values are expected to become negative during calculations.
	    __m128i u_result = _mm_adds_epi16(_mm_srai_epi16(_mm_adds_epi16(_mm_adds_epi16(_mm_adds_epi16(rvec_u,gvec_u),bvec_u),offcoeffvec),8),offcoeffvec);
	    //Packing the answer into a 16-bit vector by an instruction that is pretty much designed to do just that. Afterwards, storing the lower half,
	    //pretending to be a 64-bit integer.
	    uvec = _mm_packus_epi16(u_result,u_result);
	    _mm_storel_epi64(reinterpret_cast<__m128i*>(&ucoord[arrpos]),uvec);

	    //Calculating V. Analogous to U.
	    rcoeffvec = _mm_set1_epi16(vcoeff[0]);
	    gcoeffvec = _mm_set1_epi16(vcoeff[1]);
	    bcoeffvec = _mm_set1_epi16(vcoeff[2]);

	    __m128i rvec_v = _mm_mullo_epi16(rvec_o,rcoeffvec);
	    __m128i gvec_v = _mm_mullo_epi16(gvec_o,gcoeffvec);
	    __m128i bvec_v = _mm_mullo_epi16(bvec_o,bcoeffvec);

	    //Remember to use signed functions here.
	    __m128i v_result = _mm_adds_epi16(_mm_srai_epi16(_mm_adds_epi16(_mm_adds_epi16(_mm_adds_epi16(rvec_v,gvec_v),bvec_v),offcoeffvec),8),offcoeffvec);
	    vvec = _mm_packus_epi16(v_result,v_result);
	    _mm_storel_epi64(reinterpret_cast<__m128i*>(&vcoord[arrpos]),vvec);

	    curpos +=16;
	    //If width is not divisible by 32 and we reached the last contigous 16-bit block in the row, calculating the rest of the row through normal methods.
	    if ((j+32)>width)
	    {
		j+=16;
		while (j<width)
		{
		    curpos++;
		    j++;
		    ycoord[curpos]=clip((((ycoeff[0]*red[curpos]+ycoeff[1]*green[curpos]+ycoeff[2]*blue[curpos]+128)>>8)+128),0,255);
		    arrpos = (i/2)*(width/2)+(j/2);
		    ucoord[arrpos]=clip((((ucoeff[0]*red[curpos]+ucoeff[1]*green[curpos]+ucoeff[2]*blue[curpos]+128)>>8)+128),0,255);
		    vcoord[arrpos]=clip((((vcoeff[0]*red[curpos]+vcoeff[1]*green[curpos]+vcoeff[2]*blue[curpos]+128)>>8)+128),0,255);
		    curpos++;
		    j++;
		    ycoord[curpos]=clip((((ycoeff[0]*red[curpos]+ycoeff[1]*green[curpos]+ycoeff[2]*blue[curpos]+128)>>8)+128),0,255);
		}
	    }
	}
	//If we were given an uneven number of rows and we reached the end, quit.
	if ((i+1)>=height)
	    break;
	//Second row, only calculating Y here. Analogous to the first row.
	curpos = (i+1)*width;
	for (unsigned int j=0;j<width;j+=16)
	{
	    rvec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&red[curpos]));
	    gvec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&green[curpos]));
	    bvec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&blue[curpos]));
	    rcoeffvec = _mm_set1_epi16(ycoeff[0]);
	    gcoeffvec = _mm_set1_epi16(ycoeff[1]);
	    bcoeffvec = _mm_set1_epi16(ycoeff[2]);

	    __m128i rvec_e = _mm_srli_epi16(rvec,8);
	    __m128i gvec_e = _mm_srli_epi16(gvec,8);
	    __m128i bvec_e = _mm_srli_epi16(bvec,8);
	    __m128i rvec_o = _mm_srli_epi16(_mm_slli_epi16(rvec,8),8);
	    __m128i gvec_o = _mm_srli_epi16(_mm_slli_epi16(gvec,8),8);
	    __m128i bvec_o = _mm_srli_epi16(_mm_slli_epi16(bvec,8),8);

	    __m128i rvec_er = _mm_mullo_epi16(rvec_e,rcoeffvec);
	    __m128i rvec_or = _mm_mullo_epi16(rvec_o,rcoeffvec);
	    __m128i gvec_er = _mm_mullo_epi16(gvec_e,gcoeffvec);
	    __m128i gvec_or = _mm_mullo_epi16(gvec_o,gcoeffvec);
	    __m128i bvec_er = _mm_mullo_epi16(bvec_e,bcoeffvec);
	    __m128i bvec_or = _mm_mullo_epi16(bvec_o,bcoeffvec);

	    //Remember to use unsigned functions here.
	    __m128i e_result = _mm_adds_epu16(_mm_srli_epi16(_mm_adds_epu16(_mm_adds_epu16(_mm_adds_epu16(rvec_er,gvec_er),bvec_er),offcoeffvec),8),_mm_set1_epi16(16));
	    __m128i o_result = _mm_adds_epu16(_mm_srli_epi16(_mm_adds_epu16(_mm_adds_epu16(_mm_adds_epu16(rvec_or,gvec_or),bvec_or),offcoeffvec),8),_mm_set1_epi16(16));

	    yvec = _mm_or_si128(_mm_slli_epi16(e_result,8), o_result);
	    _mm_storeu_si128(reinterpret_cast<__m128i*>(&ycoord[curpos]),yvec);

	    curpos +=16;
	    if ((j+32)>width)
	    {
		j+=16;
		while (j<width)
		{
		    curpos++;
		    j++;
		    ycoord[curpos]=clip((((ycoeff[0]*red[curpos]+ycoeff[1]*green[curpos]+ycoeff[2]*blue[curpos]+128)>>8)+128),0,255);
		}
	    }
	}
    }
}
