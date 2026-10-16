/* RS41 soft-bit collector + CRC-gated TLV parser (no Reed-Solomon). */
#include "decrs41.h"

#define FS       9600u
#define SPB_Q8   ((int32_t)((FS << 8) / RS41_BAUD))
#define MEAN_SH  6
enum { ST_SRCH = 0, ST_DATA = 1, ST_DONE = 2 };

static uint16_t crc16(const uint8_t *p, uint8_t n)
{
    uint16_t c = 0xFFFFu;
    while (n--) {
        c ^= (uint16_t)*p++ << 8;
        for (uint8_t k = 0; k < 8u; k++)
            c = (c & 0x8000u) ? (uint16_t)((c << 1) ^ 0x1021u) : (uint16_t)(c << 1);
    }
    return c;
}

static uint32_t isqrt(uint32_t x)
{
    uint32_t r = 0, b = 1u << 30;
    while (b > x) b >>= 2;
    while (b) {
        if (x >= r + b) { x -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return r;
}

/* atan2 → millidegrees */
static int32_t atan2md(int32_t y, int32_t x)
{
    uint32_t ax = (uint32_t)(x < 0 ? -x : x);
    uint32_t ay = (uint32_t)(y < 0 ? -y : y);
    uint32_t lo = ax < ay ? ax : ay, hi = ax < ay ? ay : ax;
    if (!hi) return 0;
    uint32_t t = (lo << 16) / hi, t2 = (t * t) >> 16;
    uint32_t a = (t * 90000u) / (65536u + ((t2 * 18350u) >> 16));
    if (ay > ax) a = 90000u - a;
    int32_t r = (int32_t)a;
    if (x < 0) r = 180000 - r;
    if (y < 0) r = -r;
    return r;
}

static void ecef2lla(int32_t xc, int32_t yc, int32_t zc,
                     int32_t *latS, int32_t *lonS, int32_t *altm)
{
    int32_t xk = xc / 100000, yk = yc / 100000, zk = zc / 100000;
    int32_t lon = atan2md(yk, xk);
    uint32_t p2 = (uint32_t)(xk * xk + yk * yk);
    uint32_t p = isqrt(p2);
    int32_t lat = atan2md(zk, (int32_t)((p * 162u) / 163u));
    *lonS = lon * 36 / 10;
    *latS = lat * 36 / 10;
    *altm = (int32_t)(isqrt(p2 + (uint32_t)(zk * zk)) * 1000u) - 6378137;
}

static int32_t i32le(const uint8_t *p)
{
    return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                     ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24));
}

static void gotByte(decrs41_t *d, uint8_t v)
{
    if (d->state == ST_SRCH) {
        d->frm[0] = d->frm[1]; d->frm[1] = d->frm[2];
        d->frm[2] = d->frm[3]; d->frm[3] = v;
        uint8_t a = d->frm[0], b = d->frm[1], c = d->frm[2], e = d->frm[3];
        if (d->inv) { a = ~a; b = ~b; c = ~c; e = ~e; }
        if (a == 0x10u && b == 0xB6u && c == 0xCAu && e == 0x11u) {
            d->state = ST_DATA; d->nbyte = 4;
            d->frm[0] = 0x10; d->frm[1] = 0xB6; d->frm[2] = 0xCA; d->frm[3] = 0x11;
            return;
        }
        if (a == (uint8_t)~0x10u && b == (uint8_t)~0xB6u &&
            c == (uint8_t)~0xCAu && e == (uint8_t)~0x11u) {
            d->inv = 1u;
            d->frm[0] = 0x10; d->frm[1] = 0xB6; d->frm[2] = 0xCA; d->frm[3] = 0x11;
            d->state = ST_DATA; d->nbyte = 4;
        }
        return;
    }
    if (d->state != ST_DATA) return;
    if (d->inv) v = (uint8_t)~v;
    if (d->nbyte < RS41_FRAME_LEN) d->frm[d->nbyte++] = v;
    if (d->nbyte >= RS41_FRAME_LEN) d->state = ST_DONE;
}

void decrs41_init(decrs41_t *d)
{
    d->meanQ8 = -1;
    d->ph = SPB_Q8 / 2;
    d->nbyte = 0;
    d->state = ST_SRCH;
    d->bitc = d->byte = d->inv = d->sign = d->calOk = 0;
    d->frm[0] = d->frm[1] = d->frm[2] = d->frm[3] = 0;
}

void decrs41_rearm(decrs41_t *d)
{
    uint8_t cal = d->calOk;
    int32_t mean = d->meanQ8;
    decrs41_init(d);
    d->meanQ8 = mean;
    d->calOk = cal;
}

bool decrs41_push(decrs41_t *d, uint16_t sample)
{
    if (d->state == ST_DONE) return true;
    int32_t xq = (int32_t)sample << 8;
    if (d->meanQ8 < 0) d->meanQ8 = xq;
    int32_t e = xq - d->meanQ8;
    d->meanQ8 += (e + (1 << (MEAN_SH - 1))) >> MEAN_SH;
    uint8_t s = e > 0 ? 1u : 0u;
    d->ph += 256;
    if (s != d->sign) {
        d->sign = s;
        d->ph -= (d->ph - SPB_Q8 / 2) >> 3;
    }
    if (d->ph < SPB_Q8) return false;
    d->ph -= SPB_Q8;
    d->byte |= (uint8_t)(s << d->bitc);
    if (++d->bitc >= 8u) {
        d->bitc = 0;
        gotByte(d, d->byte);
        d->byte = 0;
    }
    return d->state == ST_DONE;
}

static bool blkOk(const uint8_t *f, uint16_t pos, uint8_t id, uint8_t len)
{
    if ((uint16_t)(pos + 4u + len) > RS41_FRAME_LEN) return false;
    if (f[pos] != id || f[pos + 1u] != len) return false;
    uint16_t got = (uint16_t)(f[pos + 2u + len] | (f[pos + 3u + len] << 8));
    return got == crc16(f + pos + 2u, len);
}

bool decrs41_parse(decrs41_t *d, const uint8_t *mask64, sonde_info_t *out)
{
    uint8_t f[RS41_FRAME_LEN];
    for (uint16_t i = 0; i < RS41_FRAME_LEN; i++)
        f[i] = (uint8_t)(d->frm[i] ^ mask64[i & 63u]);

    out->flags = 0;
    out->id[0] = 0;
    out->frame = 0;

    uint16_t pos = 57;
    while (pos + 4u < RS41_FRAME_LEN) {
        uint8_t id = f[pos], len = f[pos + 1u];
        if (!len || (uint16_t)(pos + 4u + len) > RS41_FRAME_LEN) break;
        if (!blkOk(f, pos, id, len)) { pos++; continue; }
        const uint8_t *b = f + pos + 2u;

        if (id == 0x79u && len >= 0x28u) {
            out->frame = (uint16_t)(b[0] | (b[1] << 8));
            for (uint8_t i = 0; i < 8u; i++) {
                char c = (char)b[2u + i];
                out->id[i] = (c >= 32 && c < 127) ? c : '?';
            }
            out->id[8] = 0;
            out->flags |= SONDE_HAS_ID;
            if (b[0x14] >= 3u && b[0x14] <= 6u)
                d->calOk |= (uint8_t)(1u << (b[0x14] - 3u));
        } else if (id == 0x7Bu && len >= 0x15u) {
            int32_t x = i32le(b), y = i32le(b + 4), z = i32le(b + 8);
            if (x | y | z) {
                ecef2lla(x, y, z, &out->latS, &out->lonS, &out->alt_m);
                out->flags |= SONDE_HAS_POS | SONDE_HAS_ALT;
            }
        } else if (id == 0x7Au && len >= 10u && (d->calOk & 0x0Fu) == 0x0Fu) {
            uint32_t tadc = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16);
            if (tadc > 1000u && tadc < 0xF00000u) {
                out->temp_c10 = (int16_t)(250 - (int32_t)(tadc / 5000u));
                out->flags |= SONDE_HAS_TEMP;
            }
        }
        pos = (uint16_t)(pos + 4u + len);
    }
    return (out->flags & (SONDE_HAS_ID | SONDE_HAS_POS)) != 0;
}
