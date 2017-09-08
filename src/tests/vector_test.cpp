#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <chrono>
#include <ratio>
#include "fs.h"
#include "bmp.h"
#include "convert.h"

int main()
{
    std::string image_filename = "test.bmp";
    bool identical = true;
    std::chrono::time_point<std::chrono::steady_clock> start_algorithm, end_algorithm;
    std::chrono::duration<float,std::micro> execution_time_simple, execution_time_vector;

    bmp_holder bmpdata (image_filename);

    //Allocating (requires dynamic size)
    uint8_t* red = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* green = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* blue = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* i_ybuffer = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* i_ubuffer = new uint8_t[(bmpdata.width*bmpdata.height)/4];
    uint8_t* i_vbuffer = new uint8_t[(bmpdata.width*bmpdata.height)/4];
    uint8_t* i_ybuffer2 = new uint8_t[bmpdata.width*bmpdata.height];
    uint8_t* i_ubuffer2 = new uint8_t[(bmpdata.width*bmpdata.height)/4];
    uint8_t* i_vbuffer2 = new uint8_t[(bmpdata.width*bmpdata.height)/4];

    for (unsigned int i=0;i<bmpdata.height;i++)
    	bmpdata.get_rgb_row(i,&red[bmpdata.width*i],&green[bmpdata.width*i],&blue[bmpdata.width*i]);

    start_algorithm = std::chrono::steady_clock::now();
    rgb_to_yuv_simple(bmpdata.height,bmpdata.width,red,green,blue,i_ybuffer,i_ubuffer,i_vbuffer);
    end_algorithm = std::chrono::steady_clock::now();
    execution_time_simple = std::chrono::duration_cast<std::chrono::milliseconds>(end_algorithm-start_algorithm);

    start_algorithm = std::chrono::steady_clock::now();
    rgb_to_yuv_vector(bmpdata.height,bmpdata.width,red,green,blue,i_ybuffer2,i_ubuffer2,i_vbuffer2);
    end_algorithm = std::chrono::steady_clock::now();
    execution_time_vector = std::chrono::duration_cast<std::chrono::microseconds>(end_algorithm-start_algorithm);

    if ((memcmp(reinterpret_cast<void*>(i_ybuffer),reinterpret_cast<void*>(i_ybuffer2),(bmpdata.width*bmpdata.height)))!=0)
    {
	identical = false;
	std::cout << 0 << std::endl;
    }
    if ((memcmp(reinterpret_cast<void*>(i_ubuffer),reinterpret_cast<void*>(i_ubuffer2),((bmpdata.width*bmpdata.height)/4)))!=0)
    {
	identical = false;
	std::cout << 1 << std::endl;
    }
    if ((memcmp(reinterpret_cast<void*>(i_vbuffer),reinterpret_cast<void*>(i_vbuffer2),((bmpdata.width*bmpdata.height)/4)))!=0)
    {
	identical = false;
	std::cout << 2 << std::endl;
    }

    std::cout << "Naive algorithm worked in " << execution_time_simple.count() << " microseconds." << std::endl;
    std::cout << "Vector algorithm worked in " << execution_time_vector.count() << " microseconds." << std::endl;
    std::cout << "Speedup is " << execution_time_simple.count()/execution_time_vector.count() << "x." << std::endl;

    //Deallocating memory that was allocated in this function
    delete [] i_ybuffer2;
    delete [] i_ubuffer2;
    delete [] i_vbuffer2;
    delete [] i_ybuffer;
    delete [] i_ubuffer;
    delete [] i_vbuffer;
    delete [] red;
    delete [] green;
    delete [] blue;

    if (identical==true)
    {
	std::cout << "Naive algorithm and vector algorithm work identically." << std::endl;
	return 0;
    }
    else
    {
	std::cout << "Naive algorithm and vector algorithm don't work identically." << std::endl;
	return 1;
    }
}
