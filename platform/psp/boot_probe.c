#include <pspctrl.h>
#include <pspdebug.h>
#include <pspkernel.h>
#include <pspiofilemgr.h>
#include <stdio.h>
#include <string.h>

PSP_MODULE_INFO("DescentBootProbe", 0, 0, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(32);
PSP_HEAP_SIZE_KB(2048);

int main(int argc, char **argv)
{
    const char *launch = argc > 0 ? argv[0] : "";
    pspDebugScreenInit();
    pspDebugScreenPrintf("Descent Boot Diagnostic\n\n"
                         "The EBOOT entered main and the display is working.\n"
                         "Launch path: %s\n\nPress Cross to exit.\n", launch);

    char path[512];
    const char *filename = strrchr(launch, '/');
    if (filename && (size_t)(filename - launch) + sizeof("/diagnostic.txt") <= sizeof(path)) {
        snprintf(path, sizeof(path), "%.*s/diagnostic.txt", (int)(filename - launch), launch);
        SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0666);
        if (fd >= 0) {
            static const char message[] = "Diagnostic EBOOT reached main and initialized the display.\n";
            sceIoWrite(fd, message, sizeof(message) - 1);
            sceIoClose(fd);
            pspDebugScreenPrintf("Diagnostic log written beside EBOOT.PBP.\n");
        } else {
            pspDebugScreenPrintf("Diagnostic log open failed: %08x\n", (unsigned)fd);
        }
    }

    SceCtrlData pad;
    do {
        sceCtrlReadBufferPositive(&pad, 1);
    } while (pad.Buttons & PSP_CTRL_CROSS);
    do {
        sceCtrlReadBufferPositive(&pad, 1);
    } while (!(pad.Buttons & PSP_CTRL_CROSS));
    sceKernelExitGame();
    return 0;
}
