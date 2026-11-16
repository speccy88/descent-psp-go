/* Regression coverage for file fields and arrays at unaligned addresses. */
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "maths.h"
#include "dxxerror.h"

extern int Pof_addr, Pof_file_end;
extern int pof_read_int(ubyte *);
extern short pof_read_short(ubyte *);
extern void get4fix(void *);
extern void adjust_field_of_view(void *);
static jmp_buf expected_error;
static int error_expected;

void Error(const char *format, ...)
{
    (void)format;
    assert(error_expected);
    longjmp(expected_error, 1);
}

static void test_model_fields(void)
{
    union { int alignment; ubyte bytes[32]; } storage;
    for (int offset = 0; offset < 4; ++offset) {
        /* Two signed little-endian fields, including a negative value. */
        const ubyte fields[] = {0x78, 0x56, 0x34, 0x12, 0xfe, 0xff};
        memcpy(storage.bytes + offset, fields, sizeof fields);
        Pof_addr = offset;
        Pof_file_end = offset + sizeof fields;
        assert(pof_read_int(storage.bytes) == 0x12345678);
        assert(pof_read_short(storage.bytes) == -2);
        assert(Pof_addr == Pof_file_end);
    }
    for (int width = 2; width <= 4; width += 2) {
        for (int remaining = 0; remaining < width; ++remaining) {
            Pof_addr = 1;
            Pof_file_end = 1 + remaining;
            error_expected = 1;
            if (!setjmp(expected_error)) {
                if (width == 4) pof_read_int(storage.bytes);
                else pof_read_short(storage.bytes);
                assert(!"Truncated field accepted");
            }
            assert(Pof_addr == 1);
            error_expected = 0;
        }
    }
}

static void test_robot_arrays(void)
{
    union { int alignment; ubyte bytes[32]; } storage;
    const fix expected[] = {0, 65536, -65536, 32768, -32768};
    const fix angles[] = {0, -90 * 65536, 90 * 65536, -179 * 65536, 179 * 65536};
    for (int offset = 0; offset < 4; ++offset) {
        char text[] = "field 0 1 -1 0.5 -0.5";
        fix values[5];
        memset(storage.bytes, 0xa5, sizeof storage.bytes);
        strtok(text, " ");
        get4fix(storage.bytes + offset);
        memcpy(values, storage.bytes + offset, sizeof values);
        assert(memcmp(values, expected, sizeof values) == 0);
        memcpy(storage.bytes + offset, angles, sizeof angles);
        adjust_field_of_view(storage.bytes + offset);
        memcpy(values, storage.bytes + offset, sizeof values);
        assert(values[0] == 65536);
        assert(values[1] == 0 && values[2] == 0);
        assert(values[3] < -65000 && values[4] < -65000);
        for (int i = 0; i < offset; ++i) assert(storage.bytes[i] == 0xa5);
        for (unsigned i = offset + sizeof values; i < sizeof storage.bytes; ++i)
            assert(storage.bytes[i] == 0xa5);
    }
}

int main(void)
{
    test_model_fields();
    test_robot_arrays();
    puts("Unaligned model fields and robot arrays: PASS");
    return 0;
}
