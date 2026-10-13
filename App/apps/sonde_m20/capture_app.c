/* M20 CAP — debug variant of SONDE M20: records the real PA4 audio around one
 * M20 frame into the framebuffer, so test/cap_grab.py can fetch it over
 * K5Viewer and test/feed_cap.c can decode it on the PC.
 * Ring: 896 fb bytes, 2 bit/sample (bit0 e>0, bit1 |e|>avg) = 3584 samples
 * (187 ms @ 19.2 kHz). Trigger: decoder header lock; 600 samples pre-trigger.
 * No lock within ~1.5 s: free-run capture (meta res=2).
 * Meta in status_line[112..123]. Keys: 2 AF profile, 3 ATT, 5 re-arm, EXIT. */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../app_api.h"
#include "sonde_info.h"
#include "decm20.c"

static const app_api_t *Gdiv;
uint64_t __aeabi_uidivmod(uint32_t n, uint32_t d){ return Gdiv->uidivmod(n, d); }
uint64_t __aeabi_uidiv(uint32_t n, uint32_t d) __attribute__((alias("__aeabi_uidivmod")));
uint64_t __aeabi_idivmod(int32_t n, int32_t d){ return Gdiv->idivmod(n, d); }
uint64_t __aeabi_idiv(int32_t n, int32_t d) __attribute__((alias("__aeabi_idivmod")));

#define SYST_LOAD   (*(volatile uint32_t *)0xE000E014u)
#define SYST_VAL    (*(volatile uint32_t *)0xE000E018u)
#define ADC_SR      (*(volatile uint32_t *)0x40012400u)
#define ADC_CR2     (*(volatile uint32_t *)0x40012408u)
#define ADC_SMPR3   (*(volatile uint32_t *)0x40012414u)
#define ADC_SQR3    (*(volatile uint32_t *)0x40012438u)
#define ADC_DR      (*(volatile uint32_t *)0x40012450u)
#define GPIOA_MODER (*(volatile uint32_t *)0x50000000u)
#define DAC_CR      (*(volatile uint32_t *)0x40007400u)
#define DAC_SWTRIGR (*(volatile uint32_t *)0x40007404u)
#define DAC_DHR12R1 (*(volatile uint32_t *)0x40007408u)
#define RCC_APBENR1 (*(volatile uint32_t *)0x4002103Cu)
#define RCC_DACEN   (1u << 29)
#define DAC_CR_BIAS ((1u << 0)|(1u << 1)|(1u << 2)|(7u << 3))
#define ADC_SR_EOC     (1u << 1)
#define ADC_CR2_START  ((1u << 22)|(1u << 20))
#define SMP8_POS 24u
#define SMP4_POS 12u
#define CYC_PER_SAMPLE (48000000u / 19200u)
#define RING_BYTES 896u
#define RING_N     (RING_BYTES * 4u)
#define PRE_N      600u
#define HOUSE_EVERY 4800u
#define FREE_RUN   6u      /* housekeeping slots (~1.5 s) before a free-run capture */
#define META       112u
#define NPROF      7u

typedef struct { uint32_t sqr3, smpr3, moder, dac, rcc, dhr; } saved_hw_t;

static const uint16_t ATTG[6] = { 0x03DF, 0x03DD, 0x03DB, 0x03D9, 0x0379, 0x0139 };
static const char *const ATTN[6] = { "A0", "A6", "A15", "A27", "BYP", "BYP+" };
/* REG_47 AF select per profile; profile 4 = firmware BYP (REG_3D 0x2AAB). */
static const uint8_t PROF_AF[NPROF] = { 1, 4, 5, 9, 1, 13, 14 };

static struct {
    const app_api_t *A;
    uint8_t att, nCap, prof;
    uint16_t pp, r3d, r48;
} g;

static void applyProf(void)
{
    g.A->bk_write(0x47, (uint16_t)(0x6042u | ((uint16_t)PROF_AF[g.prof] << 8)));
    g.A->bk_write(0x3D, g.prof == 4u ? 0x2AABu : g.r3d);
    g.A->bk_write(0x48, g.prof == 4u ? (uint16_t)(g.r48 | 0x000Fu) : g.r48);
}

static void applyAtt(void)
{
    uint16_t r = g.A->bk_read(0x13);
    g.A->bk_write(0x13, (uint16_t)((r & ~0x03FFu) | ATTG[g.att]));
    g.A->set_agc(false);
}
static void adcSelPA4(const saved_hw_t *s)
{
    ADC_SMPR3 = (s->smpr3 & ~(7u << SMP4_POS)) | (((s->smpr3 >> SMP8_POS) & 7u) << SMP4_POS);
    ADC_SQR3 = (s->sqr3 & ~0x1Fu) | 4u;
}
static void adcRestore(const saved_hw_t *s){ ADC_SQR3 = s->sqr3; ADC_SMPR3 = s->smpr3; }
static uint16_t adcRead(void)
{
    ADC_CR2 |= ADC_CR2_START;
    for (uint16_t n = 0; !(ADC_SR & ADC_SR_EOC) && n < 2000u; n++) {}
    return (uint16_t)(ADC_DR & 0x0FFFu);
}

static char *put(char *o, const char *s){ while (*s) *o++ = *s++; return o; }
static char *putu(char *o, uint32_t v)
{
    char t[6]; uint8_t n = 0;
    do { t[n++] = (char)('0' + v % 10u); v /= 10u; } while (v && n < 5u);
    while (n) *o++ = t[--n];
    return o;
}
static void status(const char *a, uint32_t v)
{
    char s[24], *o = s;
    g.A->status_clear();
    o = put(o, a); o = putu(o, v);
    *o++ = ' '; o = put(o, ATTN[g.att]);
    *o++ = ' '; *o++ = 'F'; o = putu(o, g.prof);
    *o++ = ' '; *o++ = 'n'; o = putu(o, g.nCap);
    *o = 0;
    g.A->print_tiny(s, 0, 1, true, true);
}

/* Returns false on EXIT. */
static bool capture(const saved_hw_t *sv)
{
    decm20_t m;
    uint8_t *ring = &g.A->fb[0][0];
    int32_t meanQ8 = -1, absQ8 = 8 << 8;
    uint16_t head = 0, post = 0, cnt = 0, amin = 4095, amax = 0;
    uint8_t trig = 0, res = 2, b0 = 0, b1 = 0, slots = 0, prevKey = APP_KEY_INVALID;
    uint32_t prev, cyc = 0, next = 0;

    decm20_init(&m);
    for (uint16_t i = 0; i < RING_BYTES; i++) ring[i] = 0;
    status("ARM P", g.pp);
    g.A->blit_status();
    adcSelPA4(sv);
    prev = SYST_VAL;
    for (;;) {
        uint32_t v;
        uint16_t s;
        do {
            v = SYST_VAL;
            cyc += (v <= prev) ? prev - v : prev + (SYST_LOAD + 1u - v);
            prev = v;
        } while (cyc < next);
        next += CYC_PER_SAMPLE;
        s = adcRead();
        if (s < amin) amin = s;
        if (s > amax) amax = s;
        {
            int32_t xq = (int32_t)s << 8, e, a;
            uint8_t c, sh = (uint8_t)((head & 3u) * 2u);
            uint8_t *p = ring + (head >> 2);
            if (meanQ8 < 0) meanQ8 = xq;
            e = xq - meanQ8;
            meanQ8 += (e + 32) >> 6;
            a = e < 0 ? -e : e;
            c = (uint8_t)((e > 0 ? 1u : 0u) | (a > absQ8 ? 2u : 0u));
            absQ8 += (a - absQ8) >> 8;
            *p = (uint8_t)((*p & ~(3u << sh)) | (c << sh));
            if (++head >= RING_N) head = 0;
        }
        {
            bool was = decm20_busy(&m);
            if (decm20_push(&m, s)) {
                sonde_info_t inf;
                bool ok;
                inf.flags = 0;
                ok = decm20_parse(&m, &inf);
                if (trig && res == 2u) { res = ok ? 1u : 0u; b0 = m.frm[0]; b1 = m.frm[1]; }
                decm20_rearm(&m);
            }
            if (!trig && !was && decm20_busy(&m)) { trig = 1u; post = RING_N - PRE_N; }
        }
        if (trig) { if (--post == 0u) break; continue; }
        if (decm20_busy(&m) || ++cnt < HOUSE_EVERY) continue;
        cnt = 0;
        g.pp = (uint16_t)(amax - amin);
        amin = 4095; amax = 0;
        adcRestore(sv);
        {
            uint8_t k = g.A->get_key();
            if (k == APP_KEY_EXIT) return false;
            if (k != prevKey && k == APP_KEY_3) { g.att = (uint8_t)((g.att + 1u) % 6u); applyAtt(); }
            prevKey = k;
        }
        status("ARM P", g.pp);
        g.A->blit_status();
        adcSelPA4(sv);
        next = cyc;
        if (++slots >= FREE_RUN) { trig = 1u; post = RING_N; }
    }
    adcRestore(sv);
    g.nCap++;
    status(res == 1u ? "CRC OK P" : "CRC KO P", g.pp);
    {
        uint8_t *st = g.A->status_line + META;
        st[0] = 0xA5u; st[1] = 0x5Au;
        st[2] = (uint8_t)head; st[3] = (uint8_t)(head >> 8);
        st[4] = res; st[5] = b0; st[6] = b1; st[7] = g.att;
        st[8] = (uint8_t)g.pp; st[9] = (uint8_t)(g.pp >> 8);
        st[10] = g.nCap; st[11] = g.prof;
    }
    prevKey = g.A->get_key();
    for (;;) {
        uint8_t k;
        g.A->blit_status();
        g.A->blit_full();
        g.A->delay_ms(100);
        k = g.A->get_key();
        if (k == APP_KEY_EXIT) return false;
        if (k != prevKey) {
            if (k == APP_KEY_5) return true;
            if (k == APP_KEY_2) { g.prof = (uint8_t)((g.prof + 1u) % NPROF); applyProf(); return true; }
        }
        prevKey = k;
    }
}

__attribute__((section(".text.entry"), used))
void app_main(const app_api_t *api)
{
    saved_hw_t sv;
    uint16_t r43;
    Gdiv = api; g.A = api; g.att = 3u;
    sv.sqr3 = ADC_SQR3; sv.smpr3 = ADC_SMPR3; sv.moder = GPIOA_MODER;
    sv.dac = DAC_CR; sv.rcc = RCC_APBENR1; sv.dhr = DAC_DHR12R1;
    GPIOA_MODER = sv.moder | (3u << 8);
    RCC_APBENR1 = sv.rcc | RCC_DACEN;
    DAC_CR = DAC_CR_BIAS; DAC_DHR12R1 = 2048u; DAC_SWTRIGR = 1u;
    api->backlight_on();
    api->display_clear();
    api->bk_write(0x2B, (uint16_t)((api->bk_read(0x2B) | 0x0700u) & ~0x0007u));
    api->bk_write(0x73, (uint16_t)(api->bk_read(0x73) | 0x0010u));
    r43 = api->bk_read(0x43);
    api->bk_write(0x43, 0x7E2Cu);
    applyAtt();
    api->audio_path(false);
    g.r3d = api->bk_read(0x3D);
    g.r48 = api->bk_read(0x48);
    applyProf();
    api->delay_ms(50);
    while (capture(&sv)) {}
    api->bk_write(0x3D, g.r3d);
    api->bk_write(0x48, g.r48);
    api->bk_write(0x43, r43);
    DAC_CR = sv.dac; DAC_DHR12R1 = sv.dhr; RCC_APBENR1 = sv.rcc; GPIOA_MODER = sv.moder;
    api->set_af(APP_AF_MUTE);
}
