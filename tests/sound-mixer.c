/* Exercise the real backend with SDL dummy audio; no proprietary test assets.
 * Optional PIG argument also stress-plays all 70 original shareware effects. */
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "digi.h"
#include "sounds.h"
#include "digi_mixer.h"
#include "config.h"
#include "args.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
digi_sound GameSounds[MAX_SOUNDS];
struct Cfg GameCfg;
struct Arg GameArg;
int digi_volume, digi_sample_rate, SoundQ_channel = -1;
static int retired_object = -1, retired_queue;
int digi_xlat_sound(int sound) { return sound == 249 ? 0 : sound; }
void digi_end_soundobj(int obj) { retired_object = obj; }
void SoundQ_end(void) { SoundQ_channel = -1; ++retired_queue; }
void con_printf(int level, const char *fmt, ...) { (void)level; (void)fmt; }
void Error(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap); exit(2); }
void sound_decompress(unsigned char *, int, unsigned char *);

/* Fail any allocation originating in the linked backend while playing. SDL's
 * shared-library initialization allocations are deliberately outside this test. */
static int deny_allocations;
static unsigned allocation_attempts;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n) { if (deny_allocations) { ++allocation_attempts; return NULL; } return __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t s) { if (deny_allocations) { ++allocation_attempts; return NULL; } return __real_calloc(n, s); }
void *__wrap_realloc(void *p, size_t n) { if (deny_allocations) { ++allocation_attempts; return NULL; } return __real_realloc(p, n); }

static void (*effect_callback)(void *, Uint8 *, int);
static void *effect_data;
static unsigned actual_callbacks;
static void instrumented_callback(void *data, Uint8 *stream, int bytes)
{
    ++actual_callbacks;
    effect_callback(data, stream, bytes);
}
void __real_Mix_SetPostMix(void (*)(void *, Uint8 *, int), void *);
void __wrap_Mix_SetPostMix(void (*cb)(void *, Uint8 *, int), void *data)
{
    /* Detach before changing a callback that the real SDL thread might use. */
    __real_Mix_SetPostMix(NULL, NULL);
    effect_callback = cb;
    effect_data = data;
    __real_Mix_SetPostMix(cb ? instrumented_callback : NULL, data);
}
static void render(Sint16 *pcm, int frames)
{
    SDL_LockAudio();
    CHECK(effect_callback != NULL);
    effect_callback(effect_data, (Uint8 *)pcm, frames * 4);
    SDL_UnlockAudio();
}
static int start(int sound, int loop, int pan)
{
    return digi_mixer_start_sound(sound, F1_0, pan, loop, -1, -1, -1);
}
static uint32_t le32(const unsigned char *p)
{
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void shareware_stress(const char *path)
{
    FILE *file = fopen(path, "rb");
    unsigned char header[20], *packed;
    Sint16 pcm[2048];
    unsigned bitmap_count, sound_count, data_start, total = 0;
    CHECK(file != NULL && fread(header, 1, 8, file) == 8);
    bitmap_count = le32(header); sound_count = le32(header + 4);
    CHECK(bitmap_count == 1152 && sound_count == 70);
    data_start = 8 + bitmap_count * 17 + sound_count * 20;
    for (unsigned i = 0; i < sound_count; ++i) {
        unsigned length, compressed, offset;
        CHECK(fseek(file, 8 + bitmap_count * 17 + i * 20, SEEK_SET) == 0);
        CHECK(fread(header, 1, 20, file) == 20);
        length = le32(header + 8); compressed = le32(header + 12); offset = le32(header + 16);
        CHECK(compressed * 2 >= length && compressed * 2 <= length + 1);
        packed = malloc(compressed);
        GameSounds[i] = (digi_sound){8, 11025, length, malloc(compressed * 2)};
        CHECK(packed && GameSounds[i].data);
        CHECK(fseek(file, data_start + offset, SEEK_SET) == 0);
        CHECK(fread(packed, 1, compressed, file) == compressed);
        sound_decompress(packed, compressed, GameSounds[i].data);
        free(packed);
        total += length;
    }
    fclose(file);
    deny_allocations = 1;
    /* Play every sample to completion 20 times, including the 69,976-byte
     * reactor explosion. This used to accumulate 13,240,720 converted bytes. */
    for (int pass = 0; pass < 20; ++pass) {
        for (unsigned i = 0; i < sound_count; ++i) {
            int channel = start(i, 0, F1_0 / 2);
            CHECK(channel >= 0);
            unsigned frames = (unsigned)GameSounds[i].length * 4;
            while (frames) {
                unsigned n = frames > 1024 ? 1024 : frames;
                memset(pcm, 0, sizeof(pcm)); render(pcm, n); frames -= n;
            }
            CHECK(!digi_mixer_is_channel_playing(channel));
        }
    }
    /* Keep 64 different effects looping and reject a 65th safely. */
    for (int i = 0; i < 64; ++i) CHECK(start(i, 1, F1_0 / 2) == i);
    CHECK(start(64, 0, 0) == -1);
    for (int i = 0; i < 2000; ++i) { memset(pcm, 0, sizeof(pcm)); render(pcm, 1024); }
    CHECK(allocation_attempts == 0);
    digi_mixer_stop_all_channels();
    deny_allocations = 0;
    for (unsigned i = 0; i < sound_count; ++i) { free(GameSounds[i].data); GameSounds[i].data = NULL; }
    printf("PASS: 1400 full shareware plays, 64 overlapping loops, %u resident source bytes, zero playback allocation attempts\n", total);
}

int main(int argc, char **argv)
{
    Uint8 sample[] = {128, 255, 0, 192};
    Sint16 pcm[2048];
    GameCfg.DigiVolume = 8;
    CHECK(digi_mixer_init() == 0);
    SDL_PauseAudio(1);
    GameSounds[0] = (digi_sound){8, 11025, 4, sample};
    deny_allocations = 1;
    CHECK(start(-1, 0, 0) == -1 && start(MAX_SOUNDS, 0, 0) == -1);
    GameSounds[1] = (digi_sound){8, 11025, 1, (void *)-1};
    CHECK(start(1, 0, 0) == -1);
    CHECK(start(0, 0, 0) == 0);
    CHECK(digi_mixer_is_channel_playing(0) && digi_mixer_is_sound_playing(249));
    /* A music-like input must survive; four output frames per source sample,
     * hard left, exact sample duration and no writes past this block. */
    for (int i = 0; i < 40; ++i) pcm[i] = 1000;
    render(pcm, 16);
    for (int i = 0; i < 16; ++i) {
        CHECK(pcm[i * 2] == 1000 + ((int)sample[i / 4] - 128) * 128);
        CHECK(pcm[i * 2 + 1] == 1000);
    }
    CHECK(pcm[32] == 1000 && !digi_mixer_is_channel_playing(0));
    CHECK(start(0, 1, F1_0) == 0);
    memset(pcm, 0, sizeof(pcm)); render(pcm, 1024);
    CHECK(pcm[8] == 0 && pcm[9] == 16256 && digi_mixer_is_channel_playing(0));
    digi_mixer_set_channel_volume(0, 0);
    memset(pcm, 0, sizeof(pcm)); render(pcm, 16);
    for (int i = 0; i < 32; ++i) CHECK(pcm[i] == 0);
    digi_mixer_set_channel_volume(0, F1_0);
    digi_mixer_set_channel_pan(0, 0);
    digi_mixer_set_digi_volume(0);
    render(pcm, 16);
    for (int i = 0; i < 32; ++i) CHECK(pcm[i] == 0);
    digi_mixer_set_digi_volume(32768);
    digi_mixer_stop_sound(0);
    /* Explicit loop boundaries and a non-divisor source rate. */
    CHECK(digi_mixer_start_sound(0, F1_0, 0, 1, 1, 3, -1) == 0);
    memset(pcm, 0, sizeof(pcm)); render(pcm, 20);
    CHECK(pcm[24] == 16256 && pcm[32] == -16384);
    digi_mixer_stop_all_channels();
    GameSounds[0].freq = 16000;
    CHECK(start(0, 0, 0) == 0);
    memset(pcm, 0, sizeof(pcm)); render(pcm, 12);
    for (int i = 0; i < 12; ++i) {
        unsigned pos = (unsigned)i * 16000 / 44100;
        CHECK(pcm[i * 2] == (pos < 4 ? ((int)sample[pos] - 128) * 128 : 0));
    }
    CHECK(!digi_mixer_is_channel_playing(0));
    GameSounds[0].freq = 11025;
    CHECK(digi_mixer_start_sound(0, F1_0, 0, 0, -1, -1, 7) == 0);
    SoundQ_channel = 0;
    memset(pcm, 0, sizeof(pcm)); render(pcm, 16);
    CHECK(start(0, 1, 0) == 0 && retired_object == 7 && retired_queue == 1);
    for (int i = 1; i < 64; ++i) CHECK(start(0, 1, 0) == i);
    CHECK(start(0, 0, 0) == -1);
    memset(pcm, 0, sizeof(pcm)); render(pcm, 16);
    CHECK(pcm[8] == 32767 && pcm[16] == -32768);
    digi_mixer_stop_sound(-1); digi_mixer_stop_sound(64);
    digi_mixer_set_channel_pan(-1, 0); digi_mixer_set_channel_volume(64, 0);
    CHECK(!digi_mixer_is_channel_playing(-1) && !digi_mixer_is_channel_playing(64));
    digi_mixer_reset();
    for (int i = 0; i < 64; ++i) CHECK(!digi_mixer_is_channel_playing(i));
    CHECK(allocation_attempts == 0);
    deny_allocations = 0;
    puts("PASS: sample duration/rates, music preservation, pan, volume, loops, clipping, channel ownership/capacity and invalid inputs");
    if (argc > 1) shareware_stress(argv[1]);
    /* Real asynchronous SDL callback plus repeated start/stop while it runs. */
    GameSounds[0] = (digi_sound){8, 11025, 4, sample};
    SDL_PauseAudio(0);
    for (int i = 0; i < 100; ++i) {
        int channel = start(0, 1, i & 1 ? F1_0 : 0);
        CHECK(channel >= 0); SDL_Delay(2); digi_mixer_stop_sound(channel);
    }
    SDL_LockAudio(); CHECK(actual_callbacks > 0); SDL_UnlockAudio();
    digi_mixer_close(); CHECK(effect_callback == NULL);
    CHECK(start(0, 0, 0) == -1);
    CHECK(digi_mixer_init() == 0);
    digi_mixer_close(); SDL_Quit();
    puts("PASS: asynchronous SDL output and repeated shutdown/reinitialization");
    return 0;
}
