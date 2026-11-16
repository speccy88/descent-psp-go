#include "startup.h"
// Holds the main init and de-init functions for arch-related program parts

#include <SDL/SDL.h>
#include "songs.h"
#include "key.h"
#include "digi.h"
#include "mouse.h"
#include "joy.h"
#include "gr.h"
#include "dxxerror.h"
#include "text.h"
#include "args.h"
#include "config.h"

void arch_close(void)
{
	songs_uninit();

	gr_close();

	if (!GameArg.CtlNoJoystick)
		joy_close();

	mouse_close();

	if (!GameArg.SndNoSound)
	{
		digi_close();
	}

	key_close();

	SDL_Quit();
}

void arch_init(void)
{
	int t;

	PSP_BOOT_STAGE("initializing SDL");
	if (SDL_Init(
#ifdef PSP_PORT
0
#else
SDL_INIT_VIDEO
#endif
) < 0)
		Error("SDL library initialisation failed: %s.",SDL_GetError());

	key_init();

	digi_select_system( GameArg.SndDisableSdlMixer ? SDLAUDIO_SYSTEM : SDLMIXER_SYSTEM );

	if (!GameArg.SndNoSound) {
		PSP_BOOT_STAGE("opening audio");
		digi_init();
		PSP_BOOT_STAGE("audio opened");
	}

	mouse_init();

	if (!GameArg.CtlNoJoystick)
		joy_init();

	PSP_BOOT_STAGE("allocating video canvas");
	if ((t = gr_init(0)) != 0)
		Error(TXT_CANT_INIT_GFX,t);

	atexit(arch_close);
}
