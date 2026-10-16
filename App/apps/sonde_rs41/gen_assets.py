#!/usr/bin/env python3
import os, sys
sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, ".."))
from app_assets import Assets

TITLE = "SONDE RS41"
UI = [
    ("T_TITLE",  TITLE),
    ("T_WAIT",   "WAIT"),
    ("T_NOPOS",  "no position"),
    ("T_ALT",    "ALT "),
    ("T_M",      "m"),
    ("T_TEMP",   "T "),
    ("T_C",      "C "),
    ("T_HPA",    "hPa"),
    ("T_FRAME",  "FRAME "),
    ("T_DBM",    "dBm"),
    ("T_KHZ",    "kHz"),
    ("T_PARAMS", "PARAMS"),
    ("T_SPK",    "Speaker"),
    ("T_ATT",    "ATT "),
    ("T_DB",     "dB"),
    ("T_BYP",    "BYP"),
    ("T_BYP_PLUS","BYP+"),
    ("T_ON",     "ON"),
    ("T_OFF",    "OFF"),
    ("T_HINT",   "UP/DN *=change EXIT=back"),
]
MASK = [
    0x96, 0x83, 0x3E, 0x51, 0xB1, 0x49, 0x08, 0x98,
    0x32, 0x05, 0x59, 0x0E, 0xF9, 0x44, 0xC6, 0x26,
    0x21, 0x60, 0xC2, 0xEA, 0x79, 0x5D, 0x6D, 0xA1,
    0x54, 0x69, 0x47, 0x0C, 0xDC, 0xE8, 0x5C, 0xF1,
    0xF7, 0x76, 0x82, 0x7F, 0x07, 0x99, 0xA2, 0x2C,
    0x93, 0x7C, 0x30, 0x63, 0xF5, 0x10, 0x2E, 0x61,
    0xD0, 0xBC, 0xB4, 0xB6, 0x06, 0xAA, 0xF4, 0x23,
    0x78, 0x6E, 0x3B, 0xAE, 0xBF, 0x7B, 0x4C, 0xC1,
]
a = Assets("SONDE_RS41")
ui_size = 0
for name, s in UI:
    pad = -(len(s) + 1) % 4
    a.text(name, s + "\0" * pad)
    ui_size += len(s) + 1 + pad
a.const("UI_SIZE", ui_size)
a.const("T_TITLE_CHARS", len(TITLE))
a.const("T_WAIT_CHARS", 4)
a.u8("RS41_MASK", MASK)
a.const("RS41_MASK_LEN", len(MASK))
SPK = [0x1c, 0x1c, 0x3e, 0x7f, 0x00, 0x22, 0x1c, 0x41, 0x22, 0x1c]
a.u8("BMP_SPK", SPK)
a.const("SPK_W", len(SPK))
# Front-end attenuator ladder (same REG_13 gains as FoxHunt)
a.u16("ATT_REG13", [0x03DF, 0x03DD, 0x03DB, 0x03D9, 0x0379, 0x0139])
a.u8("ATT_DB", [0, 6, 15, 27])
a.const("ATT_COUNT", 6)
a.const("ATT_BYP0", 4)
a.main()
