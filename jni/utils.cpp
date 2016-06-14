#include "utils.h"
#include "log.h"

#include <fcntl.h>

#define CLAMP(a, b, c) (a > c ? c : (a < b ? b : a))
#define MY(a,b,c) (( int(a)*  0.2989  + int(b)*  0.5866  + int(c)*  0.1145))
#define MU(a,b,c) (( int(a)*(-0.1688) + int(b)*(-0.3312) + int(c)*  0.5000 + 128))
#define MV(a,b,c) (( int(a)*  0.5000  + int(b)*(-0.4184) + int(c)*(-0.0816) + 128))

#define DY(a) CLAMP(MY(a&0xff, a>>8&0xff, a>>16&0xff), 0, 255)//(MY(a,b,c) > 255 ? 255 : (MY(a,b,c) < 0 ? 0 : MY(a,b,c)))
#define DU(a) CLAMP(MU(a&0xff, a>>8&0xff, a>>16&0xff), 0, 255)//(MU(a,b,c) > 255 ? 255 : (MU(a,b,c) < 0 ? 0 : MU(a,b,c)))
#define DV(a) CLAMP(MV(a&0xff, a>>8&0xff, a>>16&0xff), 0, 255)//(MV(a,b,c) > 255 ? 255 : (MV(a,b,c) < 0 ? 0 : MV(a,b,c)))

void RGBA2YUV420SP(int width, int height, char *RGB, char *YUV)
{
    int i,x,y,j;
    char *Y = NULL;
    char *UV = NULL;

    Y = YUV;
    UV = YUV + width*height;

	int a = *((int*)RGB);
	LOGD("first point, r-g-b: %.8x, %d-%d-%d-%d", a, a&0xff, a>>8&0xff, a>>16&0xff, a>>24&0xff);

    for(y=0; y < height; y++) {
        for(x=0; x < width; x++) {
            j = y*width + x;
            i = j*4;

            Y[j] = (unsigned char)(DY(*((int*)(RGB+i))));

            if(x%2 == 1 && y%2 == 1)
            {
                j = (width>>1) * (y>>1) + (x>>1);

                UV[j*2] = (unsigned char)
                       ((DU(*((int*)(RGB+i))) +
                         DU(*((int*)(RGB+i) - 1)) +
                         DU(*((int*)(RGB+i) - width)) +
                         DU(*((int*)(RGB+i) - width - 1)))/4);

                UV[j*2+1] = (unsigned char)
						((DV(*((int*)(RGB+i))) +
						  DV(*((int*)(RGB+i) - 1)) +
						  DV(*((int*)(RGB+i) - width)) +
						  DV(*((int*)(RGB+i) - width - 1)))/4);

            }

        }
    }
}

int rga_copy_and_scale(int src_w, int src_h, int src_addr, int src_fmt, int dst_w, int dst_h, int dst_addr, int dst_fmt)
{

    int gfd_rga = -1;
    struct rga_req  Rga_Request;
    memset(&Rga_Request,0x0,sizeof(Rga_Request));

    gfd_rga = open("/dev/rga",O_RDWR,0);
    if(gfd_rga < 0)
    {
        //ALOGE(" rga open err");
        return -1;
    }

    if (src_fmt != dst_fmt) {
        Rga_Request.yuv2rgb_mode |= 1<<4;
    }

    Rga_Request.src.uv_addr =  (int)src_addr;
    Rga_Request.src.vir_w = src_w;
    Rga_Request.src.vir_h = src_h;
    Rga_Request.src.format = src_fmt;

    Rga_Request.src.act_w = Rga_Request.src.vir_w;
    Rga_Request.src.act_h = Rga_Request.src.vir_h;
    Rga_Request.src.x_offset = 0;
    Rga_Request.src.y_offset = 0;

    Rga_Request.dst.uv_addr =(int)dst_addr;
    Rga_Request.dst.vir_w = dst_w;
    Rga_Request.dst.vir_h = dst_h;
    Rga_Request.dst.act_w = Rga_Request.dst.vir_w;
    Rga_Request.dst.act_h = Rga_Request.dst.vir_h;

    Rga_Request.dst.format = dst_fmt;
    Rga_Request.clip.xmin = 0;
    Rga_Request.clip.xmax = Rga_Request.dst.vir_w - 1;
    Rga_Request.clip.ymin = 0;
    Rga_Request.clip.ymax = Rga_Request.dst.vir_h - 1;
    Rga_Request.dst.x_offset = 0;
    Rga_Request.dst.y_offset = 0;

    Rga_Request.sina = 0;
    Rga_Request.cosa = 0x10000;
	
    /*if(Rga_Request.src.act_w != Rga_Request.dst.act_w
        || Rga_Request.src.act_h != Rga_Request.dst.act_h)
    {*/
        Rga_Request.scale_mode = 1;
        Rga_Request.rotate_mode = 1;
    //}
    //Rga_Request.render_mode = pre_scaling_mode;
    Rga_Request.alpha_rop_flag |= (1 << 5);

    Rga_Request.mmu_info.mmu_en    = 1;
    Rga_Request.mmu_info.mmu_flag  = ((2 & 0x3) << 4) | 1;

    if(ioctl(gfd_rga, RGA_BLIT_SYNC, &Rga_Request) != 0)
    {
/*
        ALOGE("%s(%d):  RGA_BLIT_ASYNC Failed ", __FUNCTION__, __LINE__);
        ALOGE("src info: yrgb_addr=%x, uv_addr=%x,v_addr=%x,"
             "vir_w=%d,vir_h=%d,format=%d,"
             "act_x_y_w_h [%d,%d,%d,%d] ",
                Rga_Request.src.yrgb_addr, Rga_Request.src.uv_addr ,Rga_Request.src.v_addr,
                Rga_Request.src.vir_w ,Rga_Request.src.vir_h ,Rga_Request.src.format ,
                Rga_Request.src.x_offset ,
                Rga_Request.src.y_offset,
                Rga_Request.src.act_w ,
                Rga_Request.src.act_h
            );

        ALOGE("dst info: yrgb_addr=%x, uv_addr=%x,v_addr=%x,"
             "vir_w=%d,vir_h=%d,format=%d,"
             "clip[%d,%d,%d,%d], "
             "act_x_y_w_h [%d,%d,%d,%d] ",

                Rga_Request.dst.yrgb_addr, Rga_Request.dst.uv_addr ,Rga_Request.dst.v_addr,
                Rga_Request.dst.vir_w ,Rga_Request.dst.vir_h ,Rga_Request.dst.format,
                Rga_Request.clip.xmin,
                Rga_Request.clip.xmax,
                Rga_Request.clip.ymin,
                Rga_Request.clip.ymax,
                Rga_Request.dst.x_offset ,
                Rga_Request.dst.y_offset,
                Rga_Request.dst.act_w ,
                Rga_Request.dst.act_h

            );
*/
        close(gfd_rga);
        return -1;


    }
    close(gfd_rga);
    return 0;
}
	
