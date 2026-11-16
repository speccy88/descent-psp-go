#ifndef DESCENT_PSP_STARTUP_H
#define DESCENT_PSP_STARTUP_H

enum { PSP_GAME_HEAP_KB = 12 * 1024 };

#ifdef PSP_PORT
void psp_boot_stage(const char *stage);
extern int psp_boot_no_audio;
#define PSP_BOOT_STAGE(stage) psp_boot_stage(stage)
#else
#define PSP_BOOT_STAGE(stage) ((void)0)
#endif

#endif
