#include <cmath>
#include "render.h"

//Overlays image over video, supposed to be spawned in a thread.
//This actually takes only four buffers, the first one serving as continous buffer for the entire video. The last three are coordinate buffers for the image.
//Also takes size of video (to figure out amount of frames), and dimensions of both video and image.
//Passes most of the parameters through to img_over_frame unchanged.
//Compatible with threading: uses the last two arguments (number of overall threads and number of thread it was spawned in) to evenly distribute the work by itself.
void img_over_video (uint8_t* vbuffer, uint8_t* i_ycoord, uint8_t* i_ucoord, uint8_t* i_vcoord, std::streamsize vsize, unsigned int vwidth, unsigned int vheight, unsigned int iwidth, unsigned int iheight, unsigned int threads, unsigned int division)
{
    unsigned long long framelen = (vwidth*vheight)+((vwidth*vheight)/2);
    unsigned long total_frames = vsize/framelen;
    unsigned long frames;
    if (threads==0)
        frames = total_frames;
    else
        frames = std::ceil(static_cast<float>(total_frames)/threads);
    uint8_t *yholder,*uholder,*vholder;
    for (unsigned long i=0;i<frames;i++)
    {
        //Casting streamsize to unsigned to suppress a warning. Since streamsize is very rarely negative, this should be mostly safe.
        //Oh, also this thing checks whether we passed the last element of video buffer. This will probably happen once, in last spawned thread,
        //in case work is distributed unevenly.
        if ((framelen*(division*frames+i+1))>=static_cast<unsigned long long>(vsize))
            break;
        yholder=&vbuffer[framelen*(division*frames+i)];
        uholder=yholder+(vwidth*vheight);
        vholder=uholder+((vwidth*vheight)/4);
        img_over_frame(yholder,uholder,vholder,i_ycoord,i_ucoord,i_vcoord,vwidth,vheight,iwidth,iheight);
    }
}

//Overlays image over frame of video (or larger image, in case that's what you want).
//Takes six buffers in total (first three for frame, last three for image that we overlay). Also, dimensions of video and image.
//Overlays everything in the middle, but this can be changed by modifying woffset and hoffset (they could be parameters too, in principle,
//but this would pollute parameter list even more).
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
    unsigned long vidpos = 0;
    unsigned long impos = 0;
    unsigned long i_ypos = 0;
    abspos += hoffset*vwidth+woffset;
    for (unsigned int i=0;i<iheight;i+=2)
    {
        for (unsigned int j=0;j<iwidth;j+=2)
        {
            v_ybuffer[abspos]=i_ycoord[i_ypos];
            vidpos = ((hoffset+i)/2)*(vwidth/2)+((woffset+j)/2);
            impos = (i/2)*(iwidth/2)+(j/2);
            v_ubuffer[vidpos]=i_ucoord[impos];
            v_vbuffer[vidpos]=i_vcoord[impos];
            i_ypos++;
            abspos++;
            if (j>=iwidth)
                break;
            v_ybuffer[abspos]=i_ycoord[i_ypos];
            i_ypos++;
            abspos++;
        }
        abspos+=wdiff;
        if (i>=iheight)
            break;
        for (unsigned int j=0;j<iwidth;j++)
        {
            v_ybuffer[abspos]=i_ycoord[i_ypos];
            i_ypos++;
            abspos++;
        }
        abspos+=wdiff;
    }
}
