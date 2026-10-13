#!/bin/bash
set -euo pipefail
GCCBIN="/mnt/c/Program Files (x86)/Cypress/EZ-USB FX3 SDK/1.3/ARM GCC/bin"
cd /mnt/c/DEV/uv-k1-k5v3-firmware-custom/App/apps/sonde_m20
mkdir -p /tmp/m20tc
ln -sf "$GCCBIN/arm-none-eabi-gcc.exe" /tmp/m20tc/arm-none-eabi-gcc
ln -sf "$GCCBIN/arm-none-eabi-objcopy.exe" /tmp/m20tc/arm-none-eabi-objcopy
export PATH="/tmp/m20tc:$PATH"
tr -d '\r' < build.sh \
  | sed 's/ -Wl,--no-warn-rwx-segments//' \
  | sed 's/-T app\.ld/-T _local.ld/' \
  > /tmp/m20build.sh
bash /tmp/m20build.sh
mkdir -p /mnt/c/DEV/uv-k1-k5v3-firmware-custom/build/Apps
cp -f SONDEM20.app /mnt/c/DEV/uv-k1-k5v3-firmware-custom/build/Apps/
wc -c sonde_m20.bin
echo DONE
