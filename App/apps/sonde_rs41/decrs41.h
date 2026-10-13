#ifndef DECRS41_H
#define DECRS41_H
#include <stdint.h>
#include <stdbool.h>
#include "sonde_info.h"

#define RS41_FRAME_LEN 320u
#define RS41_BAUD      4800u

enum { RS41_SRCH = 0, RS41_DATA = 1, RS41_DONE = 2 };

typedef struct {
    int32_t  meanQ8, ph, acc;
    uint16_t nbyte;
    uint8_t  state, bitc, byte, inv, sign, calOk, hdrN;
    uint8_t  frm[RS41_FRAME_LEN];
} decrs41_t;

void decrs41_init(decrs41_t *d);
void decrs41_rearm(decrs41_t *d);
bool decrs41_push(decrs41_t *d, uint16_t sample);
bool decrs41_parse(decrs41_t *d, const uint8_t *mask64, sonde_info_t *out);
#endif
