#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <psppower.h>
#include <stdio.h>
#include <math.h>
PSP_MODULE_INFO("DescentDiagnostic",0,0,1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(8*1024);
static volatile int running=1;
static short pcm[2048] __attribute__((aligned(64)));
static int quit(int a,int b,void*p){running=0;return 0;}
static int callbacks(SceSize n,void*p){int c=sceKernelCreateCallback("exit",quit,0);sceKernelRegisterExitCallback(c);while(running)sceKernelSleepThreadCB();return 0;}
static int audio(SceSize n,void*p){int c=sceAudioChReserve(-1,1024,PSP_AUDIO_FORMAT_STEREO);if(c<0)return c;while(running)sceAudioOutputPannedBlocking(c,0x1800,0x1800,pcm);sceAudioChRelease(c);return 0;}
int main(int argc,char**argv){
 pspDebugScreenInit();sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
 int cb=sceKernelCreateThread("callbacks",callbacks,0x11,4096,PSP_THREAD_ATTR_USER,0);sceKernelStartThread(cb,0,0);
 for(int i=0;i<1024;i++)pcm[i*2]=pcm[i*2+1]=(short)(sin(i*6.283185307*10/1024)*4000);
 int at=sceKernelCreateThread("audio",audio,0x12,8192,PSP_THREAD_ATTR_USER,0);sceKernelStartThread(at,0,0);
 FILE*f=fopen("diagnostic.txt","w");int fs=f!=NULL;if(f){fprintf(f,"Descent diagnostic launch=%s clock=%d free=%u\n",argv[0],scePowerGetCpuClockFrequency(),sceKernelTotalFreeMemSize());fclose(f);}
 while(running){SceCtrlData p;sceCtrlReadBufferPositive(&p,1);pspDebugScreenSetXY(0,0);pspDebugScreenPrintf("Descent PSP diagnostic\nLaunch: %s\nFile write: %s\nButtons: %08x analog: %u %u\nAudio: periodic stereo test tone\nHOME exits.\n",argv[0],fs?"PASS":"FAIL",p.Buttons,p.Lx,p.Ly);sceDisplayWaitVblankStart();}
 sceKernelWaitThreadEnd(at,0);sceKernelExitGame();return 0;
}
