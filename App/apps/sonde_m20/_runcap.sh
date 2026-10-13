#!/bin/bash
# Build the M20 CAP debug app (capture_app.c) -> M20CAP.app
set -euo pipefail
GCCBIN="/mnt/c/Program Files (x86)/Cypress/EZ-USB FX3 SDK/1.3/ARM GCC/bin"
cd /mnt/c/DEV/uv-k1-k5v3-firmware-custom/App/apps/sonde_m20
CC="$GCCBIN/arm-none-eabi-gcc.exe"
OBJCOPY="$GCCBIN/arm-none-eabi-objcopy.exe"
CFLAGS="-mcpu=cortex-m0plus -mthumb -Os -fno-jump-tables -std=gnu11 -ffreestanding -fno-builtin -fno-common \
  -fomit-frame-pointer -ffunction-sections -fdata-sections -Wall -Wextra"
LDFLAGS="-nostdlib -nostartfiles -T _local.ld -Wl,--gc-sections -Wl,--build-id=none"
"$CC" $CFLAGS $LDFLAGS capture_app.c -lgcc -o m20cap.elf
"$OBJCOPY" -O binary m20cap.elf m20cap.bin
python3 ../pack_app.py m20cap.bin M20CAP.app --name "M20 CAP" --ver 0.2 \
  --api-min 2 --vma 0x20000280 --shortcut none >/dev/null
wc -c m20cap.bin
echo DONE
