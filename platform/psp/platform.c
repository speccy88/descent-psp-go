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
#include <pspiofilemgr.h>
#include "startup.h"
#include "game.h"
#include "window.h"
PSP_MODULE_INFO("DescentGo",0,0,1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(256);
PSP_HEAP_SIZE_KB(PSP_GAME_HEAP_KB);
static volatile int exiting,suspended,resumed;
int psp_boot_no_audio;
static char boot_log_path[1024];
static int boot_logging;

void psp_boot_stage(const char *stage)
{
    if (!boot_logging)
        return;

    /* Use kernel I/O so a startup crash cannot leave this in a stdio buffer. */
    SceUID fd = sceIoOpen(boot_log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, 0666);
    if (fd >= 0) {
        sceIoWrite(fd, stage, strlen(stage));
        sceIoWrite(fd, "\n", 1);
        sceIoClose(fd);
    }
}

void psp_boot_checkpoint(const char *stage)
{
    char line[224];
    snprintf(line, sizeof(line), "%s [ms=%u heap_free=%d kernel_free=%u stack_free=%d]",
             stage, (unsigned)(sceKernelGetSystemTimeLow() / 1000),
             PSP_GAME_HEAP_KB * 1024 - mallinfo().uordblks,
             sceKernelTotalFreeMemSize(), sceKernelCheckThreadStack());
    psp_boot_stage(line);
}

extern void psp_input_init(void),psp_input_clear(void),psp_music_service(void),psp_music_stats(void);
extern void psp_timer_reset(void);
extern int32_t psp_input_probe[12];
extern int descent_main(int,char**),do_game_pause(void);
static int quit(int a,int b,void*p){exiting=1;return 0;}
static int power(int a,int flags,void*p){if(flags&(PSP_POWER_CB_SUSPENDING|PSP_POWER_CB_POWER_SWITCH))suspended=1;if(flags&PSP_POWER_CB_RESUME_COMPLETE){suspended=0;resumed=1;}return 0;}
static int callbacks(SceSize n,void*p){int c=sceKernelCreateCallback("Descent exit",quit,0);sceKernelRegisterExitCallback(c);c=sceKernelCreateCallback("Descent power",power,0);scePowerRegisterCallback(-1,c);for(;;)sceKernelSleepThreadCB();return 0;}
static void clocks(void){int rc=scePowerSetClockFrequency(333,333,166);printf("clock rc=%d CPU=%d BUS=%d kernel_free=%u heap_free=%d (12 MiB reserved)\n",rc,scePowerGetCpuClockFrequency(),scePowerGetBusClockFrequency(),sceKernelTotalFreeMemSize(),PSP_GAME_HEAP_KB*1024-mallinfo().uordblks);}
void psp_lifecycle(void){if(exiting){psp_music_stats();exit(0);}if(suspended||resumed){SDL_PauseAudio(1);while(suspended&&!exiting)sceKernelDelayThread(10000);clocks();psp_input_clear();psp_timer_reset();resumed=0;SDL_PauseAudio(0);if(Game_wind&&window_get_front()==Game_wind)do_game_pause();}psp_music_service();}
extern void psp_telemetry_flush(void),psp_present_close(void);
void psp_error(const char*message){psp_boot_stage(message);psp_present_close();pspDebugScreenInit();pspDebugScreenPrintf("Descent PSP Go\n\n%s\n\nSee logs/runtime.log\nCross exits.\n",message);SceCtrlData p;do{sceCtrlPeekBufferPositive(&p,1);sceKernelDelayThread(10000);}while(!(p.Buttons&PSP_CTRL_CROSS)&&!exiting);}
/* PSP libcglue supplies a weak exit() that goes straight to _exit().
 * Override it so engine/SDL atexit cleanup and telemetry actually run. */
extern void __call_exitprocs(int,void*);
void exit(int status){__call_exitprocs(status,NULL);fflush(NULL);_exit(status);}
static void finish(void){psp_music_stats();psp_telemetry_flush();fflush(NULL);}
int main(int argc, char **argv)
{
    pspDebugScreenInit();
    pspDebugScreenPrintf("Descent PSP Go\nStarting...\n");

    char path[1024];
    if (argc < 1 || !argv[0] || strlen(argv[0]) >= sizeof(path)) {
        psp_error("Missing or invalid PSP launch path.");
        return 1;
    }
    strcpy(path, argv[0]);
    char *filename = strrchr(path, '/');
    int cwd_result = 0;
    if (filename) {
        *filename = 0;
        cwd_result = chdir(path);
    }

    mkdir("userdata", 0777);
    mkdir("logs", 0777);
    int length = snprintf(boot_log_path, sizeof(boot_log_path), "%s%slogs/boot.log",
                          filename ? path : "", filename ? "/" : "");
    SceUID boot_log = length > 0 && (size_t)length < sizeof(boot_log_path)
        ? sceIoOpen(boot_log_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0666) : -1;
    if (boot_log >= 0) {
        sceIoClose(boot_log);
        boot_logging = 1;
    }
    psp_boot_stage("main entered");
    if (cwd_result < 0)
        psp_boot_stage("chdir returned an error; using SDK launch directory");
    psp_boot_stage(argv[0]);

    remove("logs/runtime.previous.log");
    rename("logs/runtime.log", "logs/runtime.previous.log");
    freopen("logs/runtime.log", "w", stdout);
    freopen("logs/error.log", "w", stderr);
    setbuf(stdout, NULL);
    atexit(finish);

    psp_boot_stage("setting clock");
    clocks();
    int thread = sceKernelCreateThread("Descent callbacks", callbacks, 0x11,
                                       4096, PSP_THREAD_ATTR_USER, 0);
    if (thread >= 0)
        sceKernelStartThread(thread, 0, 0);
    psp_input_init();

    SceCtrlData pad = {0};
    sceCtrlPeekBufferPositive(&pad, 1);
    psp_boot_no_audio = (pad.Buttons & PSP_CTRL_LTRIGGER) != 0;
    psp_boot_stage(psp_boot_no_audio ? "recovery: audio disabled by L" : "audio enabled");
    printf("input_probe=%p\n", (void *)psp_input_probe);
    psp_boot_stage("entering engine");
    return descent_main(argc, argv);
}
