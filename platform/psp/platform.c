#include <pspkernel.h>
#include <psppower.h>
#include <SDL/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include "game.h"
#include "window.h"
PSP_MODULE_INFO("DescentGo",0,0,1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(256);
PSP_HEAP_SIZE_KB(40*1024);
static volatile int exiting,suspended,resumed;
extern void psp_input_init(void),psp_input_clear(void),psp_music_service(void),psp_music_stats(void);
extern void psp_timer_reset(void);
extern int32_t psp_input_probe[12];
extern int descent_main(int,char**),do_game_pause(void);
static int quit(int a,int b,void*p){exiting=1;return 0;}
static int power(int a,int flags,void*p){if(flags&(PSP_POWER_CB_SUSPENDING|PSP_POWER_CB_POWER_SWITCH))suspended=1;if(flags&PSP_POWER_CB_RESUME_COMPLETE){suspended=0;resumed=1;}return 0;}
static int callbacks(SceSize n,void*p){int c=sceKernelCreateCallback("Descent exit",quit,0);sceKernelRegisterExitCallback(c);c=sceKernelCreateCallback("Descent power",power,0);scePowerRegisterCallback(-1,c);for(;;)sceKernelSleepThreadCB();return 0;}
static void clocks(void){int rc=scePowerSetClockFrequency(333,333,166);printf("clock rc=%d CPU=%d BUS=%d kernel_free=%u heap_free=%d (40MiB reserved; target >=4MiB free)\n",rc,scePowerGetCpuClockFrequency(),scePowerGetBusClockFrequency(),sceKernelTotalFreeMemSize(),40*1024*1024-mallinfo().uordblks);}
void psp_lifecycle(void){if(exiting){psp_music_stats();exit(0);}if(suspended||resumed){SDL_PauseAudio(1);while(suspended&&!exiting)sceKernelDelayThread(10000);clocks();psp_input_clear();psp_timer_reset();resumed=0;SDL_PauseAudio(0);if(Game_wind&&window_get_front()==Game_wind)do_game_pause();}psp_music_service();}
extern void psp_telemetry_flush(void),psp_present_close(void);
void psp_error(const char*message){psp_present_close();pspDebugScreenInit();pspDebugScreenPrintf("Descent PSP Go\n\n%s\n\nSee logs/runtime.log\nCross exits.\n",message);SceCtrlData p;do{sceCtrlPeekBufferPositive(&p,1);sceKernelDelayThread(10000);}while(!(p.Buttons&PSP_CTRL_CROSS)&&!exiting);}
/* PSP libcglue supplies a weak exit() that goes straight to _exit().
 * Override it so engine/SDL atexit cleanup and telemetry actually run. */
extern void __call_exitprocs(int,void*);
void exit(int status){__call_exitprocs(status,NULL);fflush(NULL);_exit(status);}
static void finish(void){psp_music_stats();psp_telemetry_flush();fflush(NULL);}
int main(int argc,char**argv){char path[1024];snprintf(path,sizeof(path),"%s",argv[0]);char*p=strrchr(path,'/');if(p){*p=0;chdir(path);}mkdir("userdata",0777);mkdir("logs",0777);remove("logs/runtime.previous.log");rename("logs/runtime.log","logs/runtime.previous.log");freopen("logs/runtime.log","w",stdout);freopen("logs/error.log","w",stderr);setbuf(stdout,NULL);atexit(finish);clocks();int t=sceKernelCreateThread("Descent callbacks",callbacks,0x11,4096,PSP_THREAD_ATTR_USER,0);if(t>=0)sceKernelStartThread(t,0,0);psp_input_init();printf("input_probe=%p\n",(void*)psp_input_probe);int result=descent_main(argc,argv);exit(result);}
