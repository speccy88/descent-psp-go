/*
 * SDL_mixer output with bounded, direct mixing of Descent's U8 mono effects.
 * Adapted from the D1X SDL backends (MD2211's 2006 SDL_mixer backend and
 * digi_audio.c). Music still uses SDL_mixer's independent music hook.
 *
 * Never expand/cache whole effects: the shareware set would require 13 MB at
 * 44.1 kHz S16 stereo. Slots borrow the engine's resident original samples.
 * Playback and the callback allocate nothing. Stop slots before freeing assets.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <SDL/SDL.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <SDL/SDL_mixer.h>
#else
#include <SDL_mixer/SDL_mixer.h>
#endif
#include "pstypes.h"
#include "dxxerror.h"
#include "digi.h"
#include "sounds.h"
#include "digi_mixer.h"
#include "console.h"
#include "config.h"
#include "args.h"

#define MAX_SOUND_SLOTS 64
#define SOUND_BUFFER_SIZE 1024
#define MIX_BLOCK_FRAMES 256

struct effect_slot {
    const Uint8 *data;
    unsigned length, position, phase, rate, loop_start, loop_end;
    int playing, looping, soundno, soundobj;
    int volume, pan, left_gain, right_gain;
};
static struct effect_slot slots[MAX_SOUND_SLOTS];
static Sint32 accumulation[MIX_BLOCK_FRAMES * 2];
static int digi_initialised, digi_max_channels = MAX_SOUND_SLOTS;
static unsigned starts, full_channels, peak_channels;

extern int SoundQ_channel;
extern void SoundQ_end(void);
extern void digi_end_soundobj(int soundobj);
extern int digi_xlat_sound(int soundno);

static int clamp(int value, int low, int high)
{
    return value < low ? low : value > high ? high : value;
}

/* Preserve the previous mixer's master/distance attenuation and linear pan,
 * without per-channel SDL effects (which themselves allocate scratch copies).
 * All gain calculations happen on the game thread, under the audio lock. */
static void update_gains(struct effect_slot *slot)
{
    int master = clamp(digi_volume, 0, 32768);
    int attenuation = slot->volume > F1_0 ? F1_0 :
        (int)((int64_t)master * clamp(slot->volume, 0, F1_0) / F1_0);
    int gain = (int)((int64_t)(master * 2) * attenuation / F1_0);
    int pan = clamp(slot->pan, 0, F1_0);
    slot->left_gain = (int)((int64_t)gain * (F1_0 - pan) / F1_0);
    slot->right_gain = (int)((int64_t)gain * pan / F1_0);
}

/* SDL already holds the audio lock here. No I/O, allocation, engine callbacks,
 * or nested locking is allowed. A small fixed buffer sums music plus effects
 * in 32 bits, then clips once to S16. Integer-rate repetition preserves the
 * original 11025 Hz samples at the 44100 Hz output rate. */
static void SDLCALL mix_effects(void *unused, Uint8 *stream, int bytes)
{
    Sint16 *output = (Sint16 *)stream;
    int remaining = bytes / (int)(2 * sizeof(*output));
    (void)unused;
    while (remaining > 0) {
        int frames = remaining > MIX_BLOCK_FRAMES ? MIX_BLOCK_FRAMES : remaining;
        int i, c;
        for (i = 0; i < frames * 2; ++i)
            accumulation[i] = output[i];
        for (c = 0; c < digi_max_channels; ++c) {
            struct effect_slot *slot = &slots[c];
            if (!slot->playing) continue;
            for (i = 0; i < frames && slot->playing; ++i) {
                int sample = (int)slot->data[slot->position] - 128;
                accumulation[i * 2] += sample * slot->left_gain / 256;
                accumulation[i * 2 + 1] += sample * slot->right_gain / 256;
                slot->phase += slot->rate;
                while (slot->phase >= (unsigned)digi_sample_rate) {
                    slot->phase -= digi_sample_rate;
                    ++slot->position;
                    if (slot->looping && slot->position >= slot->loop_end)
                        slot->position = slot->loop_start;
                    else if (slot->position >= slot->length) {
                        slot->playing = 0;
                        break;
                    }
                }
            }
        }
        for (i = 0; i < frames * 2; ++i)
            output[i] = (Sint16)clamp(accumulation[i], -32768, 32767);
        output += frames * 2;
        remaining -= frames;
    }
}

int digi_mixer_init(void)
{
    int rate, channels;
    Uint16 format;
    if (digi_initialised) return 0;
    digi_sample_rate = SAMPLE_RATE_44K;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        Error("SDL audio initialisation failed: %s.", SDL_GetError());
    if (Mix_OpenAudio(digi_sample_rate, AUDIO_S16SYS, 2, SOUND_BUFFER_SIZE)) {
        con_printf(CON_URGENT, "Couldn't open audio: %s\n", SDL_GetError());
        GameArg.SndNoSound = 1;
        return 1;
    }
    if (!Mix_QuerySpec(&rate, &format, &channels) ||
        rate != digi_sample_rate || format != AUDIO_S16SYS || channels != 2) {
        con_printf(CON_URGENT, "Unsupported sound output format\n");
        Mix_CloseAudio();
        GameArg.SndNoSound = 1;
        return 1;
    }
    memset(slots, 0, sizeof(slots));
    for (int i = 0; i < MAX_SOUND_SLOTS; ++i) slots[i].soundobj = -1;
    starts = full_channels = peak_channels = 0;
    digi_initialised = 1;
    digi_mixer_set_digi_volume((GameCfg.DigiVolume * 32768) / 8);
    Mix_SetPostMix(mix_effects, NULL);
    printf("SFX: direct U8 mixing, %u bytes fixed workspace, no conversion cache\n",
           (unsigned)(sizeof(slots) + sizeof(accumulation)));
    return 0;
}

void digi_mixer_close(void)
{
    if (!digi_initialised) return;
    /* Detaching waits for any in-flight callback before slots can be cleared. */
    Mix_SetPostMix(NULL, NULL);
    digi_mixer_stop_all_channels();
    digi_initialised = 0;
    Mix_CloseAudio();
    printf("SFX: starts=%u peak_channels=%u full_channels=%u conversion_bytes=0\n",
           starts, peak_channels, full_channels);
}

int digi_mixer_start_sound(short soundnum, fix volume, int pan, int looping,
                          int loop_start, int loop_end, int soundobj)
{
    struct effect_slot *slot;
    digi_sound *sound;
    int channel = -1, active = 0;
    if (!digi_initialised || soundnum < 0 || soundnum >= MAX_SOUNDS) return -1;
    sound = &GameSounds[soundnum];
    if (!sound->data || sound->data == (void *)-1 || sound->length <= 0 ||
        sound->bits != 8 || sound->freq <= 0 || sound->freq > 192000) return -1;
    SDL_LockAudio();
    for (int i = 0; i < digi_max_channels; ++i) {
        if (slots[i].playing) ++active;
        else if (channel < 0) channel = i;
    }
    if (channel < 0) {
        ++full_channels;
        SDL_UnlockAudio();
        return -1;
    }
    slot = &slots[channel];
    /* Retire an expired engine reference before this channel is reused. These
     * functions only update engine bookkeeping; never run them on audio thread. */
    if (slot->soundobj > -1) digi_end_soundobj(slot->soundobj);
    if (SoundQ_channel == channel) SoundQ_end();
    slot->data = sound->data;
    slot->length = sound->length;
    slot->rate = sound->freq;
    slot->position = slot->phase = 0;
    slot->looping = looping != 0;
    slot->loop_start = 0;
    slot->loop_end = sound->length;
    if (loop_start >= 0 && loop_start < loop_end && loop_end <= sound->length) {
        slot->loop_start = loop_start;
        slot->loop_end = loop_end;
    }
    slot->volume = volume;
    slot->pan = pan;
    slot->soundno = soundnum;
    slot->soundobj = soundobj;
    update_gains(slot);
    slot->playing = 1;
    ++starts;
    if ((unsigned)(active + 1) > peak_channels) peak_channels = active + 1;
    SDL_UnlockAudio();
    return channel;
}

void digi_mixer_set_channel_volume(int channel, int volume)
{
    if (!digi_initialised || channel < 0 || channel >= digi_max_channels) return;
    SDL_LockAudio();
    slots[channel].volume = volume;
    update_gains(&slots[channel]);
    SDL_UnlockAudio();
}

void digi_mixer_set_channel_pan(int channel, int pan)
{
    if (!digi_initialised || channel < 0 || channel >= digi_max_channels) return;
    SDL_LockAudio();
    slots[channel].pan = pan;
    update_gains(&slots[channel]);
    SDL_UnlockAudio();
}

void digi_mixer_stop_sound(int channel)
{
    if (!digi_initialised || channel < 0 || channel >= digi_max_channels) return;
    SDL_LockAudio();
    slots[channel].playing = 0;
    slots[channel].soundobj = -1;
    SDL_UnlockAudio();
}

void digi_mixer_end_sound(int channel)
{
    /* An object going away releases ownership; its one-shot may finish. */
    if (!digi_initialised || channel < 0 || channel >= digi_max_channels) return;
    SDL_LockAudio();
    slots[channel].soundobj = -1;
    SDL_UnlockAudio();
}

void digi_mixer_set_digi_volume(int volume)
{
    if (digi_initialised) SDL_LockAudio();
    digi_volume = clamp(volume, 0, 32768);
    if (digi_initialised) {
        for (int i = 0; i < digi_max_channels; ++i) update_gains(&slots[i]);
        SDL_UnlockAudio();
    }
}

int digi_mixer_is_channel_playing(int channel)
{
    int playing;
    if (!digi_initialised || channel < 0 || channel >= digi_max_channels) return 0;
    SDL_LockAudio();
    playing = slots[channel].playing;
    SDL_UnlockAudio();
    return playing;
}

int digi_mixer_is_sound_playing(int soundno)
{
    int playing = 0;
    if (!digi_initialised || soundno < 0 || soundno >= MAX_SOUNDS) return 0;
    soundno = digi_xlat_sound(soundno);
    if (soundno < 0) return 0;
    SDL_LockAudio();
    for (int i = 0; i < digi_max_channels; ++i)
        if (slots[i].playing && slots[i].soundno == soundno) { playing = 1; break; }
    SDL_UnlockAudio();
    return playing;
}

void digi_mixer_stop_all_channels(void)
{
    if (!digi_initialised) return;
    SDL_LockAudio();
    for (int i = 0; i < MAX_SOUND_SLOTS; ++i) {
        slots[i].playing = 0;
        slots[i].soundobj = -1;
    }
    SDL_UnlockAudio();
}

void digi_mixer_reset(void) { digi_mixer_stop_all_channels(); }
void digi_mixer_set_max_channels(int count)
{
    digi_mixer_stop_all_channels();
    if (digi_initialised) SDL_LockAudio();
    digi_max_channels = clamp(count, 1, MAX_SOUND_SLOTS);
    if (digi_initialised) SDL_UnlockAudio();
}
int digi_mixer_get_max_channels(void) { return digi_max_channels; }
#ifndef NDEBUG
void digi_mixer_debug(void) { }
#endif
