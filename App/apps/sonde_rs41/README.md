# SONDE RS41

Overlay app: decode **Vaisala RS41** frames on the UV-K1 / UV-K5 v3.

Tune the VFO (FM wide, ~400–406 MHz), launch from Apps (F+7). GPS is shown
first, then altitude and temperature when available. Key `*` opens parameters
(speaker, view).

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

A sonde **too close** can clip the AF on PA4 and keep the UI on WAITING even
though you hear the frames. Raise ATT with key `3` until RSSI drops and decode
starts (same ladder as Fox Hunt).

## Limits (v0.1)

- Block CRC only (no Reed-Solomon) — needs a clean signal
- Temperature is a coarse placeholder once cal subframes 3–6 are seen

## Build

```bash
./compile-app.sh sonde_rs41
```
