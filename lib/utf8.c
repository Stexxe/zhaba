#include <stdint.h>

typedef uint32_t rune;

rune read_rune(unsigned char *buf, int *outsize) {
    rune r = 0;

    if (buf[0] >> 7 == 0) {
        *outsize = 1;
        r = (rune) buf[0];
    } else if (buf[0] >> 5 == 0b110) {
        *outsize = 2;
        r |= (buf[0] & 0x1F) << 6;
        r |= buf[1] & 0x3F;
    } else if (buf[0] >> 4 == 0b1110) {
        *outsize = 3;
        r |= (buf[0] & 0xF) << 12;
        r |= (buf[1] & 0x3F) << 6;
        r |= buf[2] & 0x3F;
    } else if (buf[0] >> 3 == 0b11110) {
        *outsize = 4;
        r |= (buf[0] & 0x7) << 18;
        r |= (buf[1] & 0x3F) << 12;
        r |= (buf[2] & 0x3F) << 6;
        r |= buf[3] & 0x3F;
    }

    return r;
}
