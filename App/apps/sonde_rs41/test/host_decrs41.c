#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../sonde_info.h"
#include "../decrs41.h"
#include "../decrs41.c"

static const uint8_t MASK[64] = {
    0x96, 0x83, 0x3E, 0x51, 0xB1, 0x49, 0x08, 0x98,
    0x32, 0x05, 0x59, 0x0E, 0xF9, 0x44, 0xC6, 0x26,
    0x21, 0x60, 0xC2, 0xEA, 0x79, 0x5D, 0x6D, 0xA1,
    0x54, 0x69, 0x47, 0x0C, 0xDC, 0xE8, 0x5C, 0xF1,
    0xF7, 0x76, 0x82, 0x7F, 0x07, 0x99, 0xA2, 0x2C,
    0x93, 0x7C, 0x30, 0x63, 0xF5, 0x10, 0x2E, 0x61,
    0xD0, 0xBC, 0xB4, 0xB6, 0x06, 0xAA, 0xF4, 0x23,
    0x78, 0x6E, 0x3B, 0xAE, 0xBF, 0x7B, 0x4C, 0xC1,
};

static uint16_t mk_crc16(const uint8_t *p, uint8_t n)
{
    uint16_t c = 0xFFFFu;
    while (n--) {
        c ^= (uint16_t)*p++ << 8;
        for (uint8_t k = 0; k < 8u; k++)
            c = (c & 0x8000u) ? (uint16_t)((c << 1) ^ 0x1021u) : (uint16_t)(c << 1);
    }
    return c;
}

static void put_block(uint8_t *f, uint16_t *pos, uint8_t id, uint8_t len, const uint8_t *data)
{
    f[(*pos)++] = id; f[(*pos)++] = len;
    memcpy(f + *pos, data, len);
    *pos = (uint16_t)(*pos + len);
    uint16_t c = mk_crc16(data, len);
    f[(*pos)++] = (uint8_t)(c & 0xFF);
    f[(*pos)++] = (uint8_t)(c >> 8);
}

int main(void)
{
    uint8_t raw[RS41_FRAME_LEN];
    memset(raw, 0, sizeof raw);
    memcpy(raw, (uint8_t[]){ 0x10, 0xB6, 0xCA, 0x11, 0x22, 0x96, 0x12, 0xF8 }, 8);
    raw[56] = 0x0F;
    uint8_t status[0x28]; memset(status, 0, sizeof status);
    status[0] = 1; memcpy(status + 2, "N1234567", 8); status[0x14] = 3;
    uint8_t gps[0x15]; memset(gps, 0, sizeof gps);
    int32_t x = 420100000, y = 16800000, z = 478000000;
    memcpy(gps, &x, 4); memcpy(gps + 4, &y, 4); memcpy(gps + 8, &z, 4);
    uint16_t pos = 57;
    put_block(raw, &pos, 0x79, 0x28, status);
    put_block(raw, &pos, 0x7B, 0x15, gps);
    uint8_t air[RS41_FRAME_LEN];
    for (int i = 0; i < (int)RS41_FRAME_LEN; i++) air[i] = (uint8_t)(raw[i] ^ MASK[i & 63]);
    decrs41_t d; decrs41_init(&d);
    memcpy(d.frm, air, RS41_FRAME_LEN); d.state = 2; d.nbyte = RS41_FRAME_LEN;
    sonde_info_t out;
    if (!decrs41_parse(&d, MASK, &out)) { fprintf(stderr, "fail\n"); return 1; }
    printf("id=%s flags=%02x latS=%d lonS=%d alt=%d\n", out.id, out.flags, out.latS, out.lonS, out.alt_m);
    if (!(out.flags & SONDE_HAS_ID) || !(out.flags & SONDE_HAS_POS)) return 2;
    if (strcmp(out.id, "N1234567")) return 3;
    puts("OK"); return 0;
}
