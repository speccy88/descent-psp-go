/* Exercise the real PLX reader and check outputs passed into the C library. */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "playsave.h"
#include "args.h"
#include "physfsx.h"

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL: %s at line %d\n", #c, __LINE__); exit(1); } } while (0)
struct Arg GameArg;
player Players[MAX_PLAYERS];
int Player_num;
static unsigned checked_scans;
extern int read_player_d1x(char *filename);

/* glibc itself is not alignment-instrumented. Check the varargs boundary. */
int __wrap___isoc99_sscanf(const char *text, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (!strcmp(format, "%i,%i,%i,%i")) {
        va_list inspect;
        va_copy(inspect, args);
        for (int i = 0; i < 4; ++i) {
            int *output = va_arg(inspect, int *);
            CHECK((uintptr_t)output % _Alignof(int) == 0);
        }
        va_end(inspect);
        ++checked_scans;
    }
    int result = vsscanf(text, format, args);
    va_end(args);
    return result;
}

int __wrap___isoc23_sscanf(const char *, const char *, ...)
    __attribute__((alias("__wrap___isoc99_sscanf")));

int main(int argc, char **argv)
{
    char directory[] = "build/tests/pilot-settings-XXXXXX";
    char filename[256];
    const char *values[] = {"0,32,0,0", "7,8,nope", "invalid"};
    const int expected[][4] = {{0,32,0,0}, {7,8,3,4}, {1,2,3,4}};
    (void)argc;
    CHECK(mkdtemp(directory));
    CHECK(PHYSFS_init(argv[0]));
    CHECK(PHYSFS_addToSearchPath(directory, 0));
    strcpy(Players[0].callsign, "pilot");
    snprintf(filename, sizeof filename, "%s/pilot.plx", directory);
    for (unsigned test = 0; test < sizeof(values)/sizeof(*values); ++test) {
        for (int i=0; i<4; ++i) PlayerCfg.ReticleRGBA[i] = i+1;
        FILE *file = fopen(filename, "w");
        CHECK(file);
        fprintf(file, "[D1X Options]\n[cockpit]\nretrgba=%s\n[end]\n[end]\n", values[test]);
        CHECK(fclose(file)==0);
        CHECK(read_player_d1x("pilot.plx")==0);
        for (int i=0; i<4; ++i) CHECK(PlayerCfg.ReticleRGBA[i] == expected[test][i]);
    }
    CHECK(checked_scans==3);
    CHECK(PHYSFS_deinit());
    CHECK(unlink(filename)==0);
    CHECK(rmdir(directory)==0);
    puts("Pilot settings: aligned library outputs and valid/partial/invalid colours PASS");
    return 0;
}
