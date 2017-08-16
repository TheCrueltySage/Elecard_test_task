#include <cstdint>
#include "render.h"

void img_over_frame (uint8_t* v_ybuffer, uint8_t* v_ubuffer, uint8_t* v_vbuffer, uint8_t** i_ycoord, uint8_t** i_ucoord, uint8_t** i_vcoord, unsigned long divlen, unsigned int vwidth, unsigned int vheight, unsigned int iwidth, unsigned int iheight)
{
    unsigned int wdiff = vwidth-iwidth;
    unsigned int woffset = wdiff/2;
    if (woffset%2!=0)
        woffset++;
    unsigned int hoffset = (vheight-iheight)/2;
    if (hoffset%2!=0)
        hoffset++;
    unsigned long abspos = 0;
    unsigned long v_upos = vwidth*vheight;
    unsigned long v_vpos = (vwidth*vheight)+((vwidth*vheight)/4);
    unsigned long i_ypos = 0;
    abspos += hoffset*vwidth+woffset;
    for (unsigned int i=0;i<iheight;i++)
    {
        for (unsigned int j=0;j<iwidth;j++)
        {
            unsigned int divnum = i_ypos/divlen;
            unsigned int curdivpos = i_ypos%divlen;
            vbuffer[abspos]=i_ycoord[divnum][curdivpos];
            if ((i%2==0)&&(j%2==0))
            {
        	unsigned long vidpos = ((hoffset+i)/2)*(vwidth/2)+((woffset+j)/2);
        	unsigned long impos = (i/2)*(iwidth/2)+(j/2);
        	curdivpos = impos%(divlen/4);
        	vbuffer[v_upos+vidpos]=i_ucoord[divnum][curdivpos];
        	vbuffer[v_vpos+vidpos]=i_vcoord[divnum][curdivpos];
            }
            i_ypos++;
            abspos++;
        }
        abspos+=wdiff;
    }
    framepos+=(vwidth*vheight);
}
