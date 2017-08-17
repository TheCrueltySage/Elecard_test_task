#include <cmath>
#include "render.h"
using namespace std;

void img_over_video_thread (uint8_t* vbuffer, uint8_t* i_ycoord, uint8_t* i_ucoord, uint8_t* i_vcoord, streamsize vsize, unsigned int threads, unsigned int division, unsigned int vwidth, unsigned int vheight, unsigned int iwidth, unsigned int iheight)
{
    unsigned long long framelen = (vwidth*vheight)+((vwidth*vheight)/2);
    unsigned long total_frames = vsize/framelen;
    unsigned long frames = ceil(static_cast<float>(total_frames)/threads);
    uint8_t *yholder,*uholder,*vholder;
    for (unsigned long i=0;i<frames;i++)
    {
        if ((framelen*(division*frames+i)+(vwidth*vheight)+((vwidth*vheight)/2))>=static_cast<unsigned long long>(vsize))
            break;
        yholder=&vbuffer[framelen*(division*frames+i)];
        uholder=yholder+(vwidth*vheight);
        vholder=uholder+((vwidth*vheight)/4);
        img_over_frame(yholder,uholder,vholder,i_ycoord,i_ucoord,i_vcoord,vwidth,vheight,iwidth,iheight);
    }
}

void img_over_video (uint8_t* vbuffer, uint8_t* i_ycoord, uint8_t* i_ucoord, uint8_t* i_vcoord, streamsize vsize, unsigned int vwidth, unsigned int vheight, unsigned int iwidth, unsigned int iheight)
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
    unsigned long i_ypos = 0;
    abspos += hoffset*vwidth+woffset;
    for (unsigned int i=0;i<iheight;i++)
    {
        for (unsigned int j=0;j<iwidth;j++)
        {
            v_ybuffer[abspos]=i_ycoord[i_ypos];
            if ((i%2==0)&&(j%2==0))
            {
        	unsigned long vidpos = ((hoffset+i)/2)*(vwidth/2)+((woffset+j)/2);
        	unsigned long impos = (i/2)*(iwidth/2)+(j/2);
        	v_ubuffer[vidpos]=i_ucoord[impos];
        	v_vbuffer[vidpos]=i_vcoord[impos];
            }
            i_ypos++;
            abspos++;
        }
        abspos+=wdiff;
    }
}
