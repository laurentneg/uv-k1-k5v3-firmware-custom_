/* Common decoded radiosonde fields (RS41 app). */
#ifndef SONDE_INFO_H
#define SONDE_INFO_H
#include <stdint.h>

typedef struct {
    char     id[10];
    int32_t  latS, lonS;      /* arc seconds, N/E positive */
    int32_t  alt_m;
    int16_t  temp_c10;        /* 0.1 C */
    uint16_t rh10;            /* 0.1 % */
    uint16_t pres_hpa10;      /* 0.1 hPa */
    uint8_t  flags;
    uint16_t frame;
} sonde_info_t;

enum {
    SONDE_HAS_POS  = 1u,
    SONDE_HAS_ALT  = 2u,
    SONDE_HAS_TEMP = 4u,
    SONDE_HAS_RH   = 8u,
    SONDE_HAS_PRES = 16u,
    SONDE_HAS_ID   = 32u
};
#endif
