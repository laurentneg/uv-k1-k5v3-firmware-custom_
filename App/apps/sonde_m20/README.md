# SONDE M20

Overlay app: decode **Meteomodem M20** frames on the UV-K1 / UV-K5 v3.

Tune the VFO (FM wide, ~400–406 MHz), launch from Apps (F+7). GPS is shown
first, then altitude, temperature and humidity when available. Key `*` opens
parameters (speaker, view).

## Keys

| Key | Action |
|---|---|
| UP / DOWN | Scroll / history |
| `*` | Parameters (speaker / view); in menu, cycle value |
| 3 | Cycle front-end attenuator (0 / 6 / 15 / 27 dB / BYP / BYP+), shown on the bottom row |
| MENU | Clear history / apply menu |
| EXIT | Cancel menu / quit |
| 1 | Speaker |
| 4 / 6 / 5 | Tune −5 / +5 kHz / VFO reset |

A sonde **too close** (or a strong local TX on 70 cm) can clip the AF on PA4:
bottom-line `P` stays very high and frames show `4520` with CRC fail. Use key `3`
to raise ATT (`A27` = 27 dB, lowest `P`). Avoid `ABYP+` (max gain). Raise ATT
with key `3` until `P` drops and `S` increments (same ladder as Fox Hunt).

Decoder follows auto_rx `m20mod` softin (Manchester + diff, header `0x4520`).

## Limits (v0.1)

- M20 frame `0x45/0x20` (not M10); GPS×1e6 @ 0x1C/0x20; Manchester dpsk
- RH approx; coarse NTC temperature

## Build

```bash
./compile-app.sh sonde_m20
```
