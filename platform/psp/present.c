/* PSP indexed framebuffer presentation. Engine pixels stay opaque, including 255. */
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <psputility.h>
#include <pspkernel.h>
#include <SDL/SDL.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "startup.h"
static unsigned int list[4096] __attribute__((aligned(64)));
static unsigned char tex[512*256] __attribute__((aligned(64)));
static uint32_t palette[256] __attribute__((aligned(64)));
static int initialized;
static int first_frame;
extern void psp_error(const char *message);
int psp_aspect_mode=0,psp_frame_cap=60;
static struct{uint32_t interval,work,ge_wait;} samples[8192];
static unsigned sample_count,missed;static uint64_t last_present,frame_start;
void psp_frame_begin(void){frame_start=sceKernelGetSystemTimeWide();}
void psp_telemetry_flush(void){FILE*f=fopen("logs/frames.csv","w");if(f){fputs("interval_us,work_us,ge_wait_us\n",f);for(unsigned i=0;i<sample_count;i++)fprintf(f,"%u,%u,%u\n",samples[i].interval,samples[i].work,samples[i].ge_wait);fclose(f);}struct mallinfo m=mallinfo();printf("Telemetry frames=%u missed=%u heap_used=%d estimated_reserved_free=%d EDRAM=%u\n",sample_count,missed,m.uordblks,PSP_GAME_HEAP_KB*1024-m.uordblks,sceGeEdramGetSize());}
struct vertex{float u,v,x,y,z;};
void psp_present(SDL_Surface*s){
 if (!initialized) {
  psp_boot_stage("initializing GE");
  if (sceGuInit() < 0) {
   psp_error("Graphics engine initialization failed.");
   exit(1);
  }
  initialized = 1;
  first_frame = 1;
  if (sceGuStart(GU_DIRECT, list) < 0) {
   psp_error("Could not submit graphics initialization commands.");
   exit(1);
  }
  sceGuDrawBuffer(GU_PSM_8888,0,512);sceGuDispBuffer(480,272,(void*)(512*272*4),512);sceGuOffset(2048-240,2048-136);sceGuViewport(2048,2048,480,272);sceGuScissor(0,0,480,272);sceGuEnable(GU_SCISSOR_TEST);sceGuDisable(GU_DEPTH_TEST);sceGuDisable(GU_CULL_FACE);sceGuDisable(GU_BLEND);sceGuDisable(GU_ALPHA_TEST);sceGuFinish();sceGuSync(0,0);
  psp_boot_stage("GE initialized; preparing first frame");
 }
 /* Sync before reusing CPU texture, CLUT and list memory consumed by GE. */
 sceGuSync(0,0);
 for(int y=0;y<200;y++)memcpy(tex+y*512,(unsigned char*)s->pixels+y*s->pitch,320);
 for(int i=0;i<256;i++){SDL_Color c=s->format->palette->colors[i];palette[i]=0xff000000u|c.r|(c.g<<8)|(c.b<<16);}
 sceKernelDcacheWritebackRange(tex,sizeof(tex));sceKernelDcacheWritebackRange(palette,sizeof(palette));
 if (sceGuStart(GU_DIRECT, list) < 0) {
  psp_error("Could not submit graphics frame.");
  exit(1);
 }
 sceGuClearColor(0xff000000);sceGuClear(GU_COLOR_BUFFER_BIT);
 sceGuEnable(GU_TEXTURE_2D);sceGuTexMode(GU_PSM_T8,0,0,0);sceGuClutMode(GU_PSM_8888,0,255,0);sceGuClutLoad(32,palette);sceGuTexImage(0,512,256,512,tex);sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGB);sceGuTexFilter(GU_NEAREST,GU_NEAREST);sceGuTexWrap(GU_CLAMP,GU_CLAMP);
 /* Slices avoid PSP texture interpolation precision artifacts. */
 struct vertex*v=sceGuGetMemory(20*sizeof(*v));
 float left=60,top=1,width=360,height=270;
 if(psp_aspect_mode==1){left=80;top=36;width=320;height=200;}else if(psp_aspect_mode==2){left=top=0;width=480;height=272;}
 for(int i=0;i<10;i++){v[i*2]=(struct vertex){i*32,0,left+i*width/10,top,0};v[i*2+1]=(struct vertex){(i+1)*32,200,left+(i+1)*width/10,top+height,0};}
 sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_2D,20,0,v);
 uint64_t work_end=sceKernelGetSystemTimeWide();sceGuFinish();sceGuSync(0,0);uint64_t ge_end=sceKernelGetSystemTimeWide();
 sceDisplayWaitVblankStart();if(psp_frame_cap==30)sceDisplayWaitVblankStart();sceGuSwapBuffers();
 if (first_frame) {
  sceGuDisplay(GU_TRUE);
  psp_boot_stage("first graphics frame displayed");
  first_frame = 0;
 }
 uint64_t now=sceKernelGetSystemTimeWide();
 if(last_present&&sample_count<8192){samples[sample_count].interval=now-last_present;samples[sample_count].work=work_end-frame_start;samples[sample_count].ge_wait=ge_end-work_end;sample_count++;if(now-last_present>(psp_frame_cap==30?34000:17500))missed++;}last_present=now;
}
void psp_present_close(void){if(initialized){sceGuSync(0,0);sceGuTerm();initialized=0;}}

void psp_osk_frame(int visible){sceGuStart(GU_DIRECT,list);sceGuClearColor(0xff181818);sceGuClear(GU_COLOR_BUFFER_BIT);sceGuFinish();sceGuSync(0,0);if(visible)sceUtilityOskUpdate(1);sceDisplayWaitVblankStart();sceGuSwapBuffers();}
