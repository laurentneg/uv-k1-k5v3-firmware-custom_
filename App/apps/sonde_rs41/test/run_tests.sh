#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
gcc -std=c11 -Wall -Wextra -O2 -o host_decrs41 host_decrs41.c
./host_decrs41
echo "sonde_rs41 host tests passed"
