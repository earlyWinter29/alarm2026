#!/usr/bin/env python3
"""Test di non regressione sul peso del firmware.

Fallisce se l'immagine da flashare supera il budget. Se una nuova funzione
lo giustifica, alza il budget a mano in questo file (e scrivi perché).

    pio run -e esp32dev && python3 tools/check_size.py
"""
import os
import sys

# Byte di firmware.bin dopo l'ottimizzazione (prima: 1835888)
# +4336 B: presenza telefono via BLE (scansione GAP + AES per gli indirizzi RPA)
BUDGET = 1836368
APP_PARTITION = 2000 * 1024  # partitions.csv: app0 2000K

path = sys.argv[1] if len(sys.argv) > 1 else ".pio/build/esp32dev/firmware.bin"
size = os.path.getsize(path)
print(f"firmware.bin: {size} B (budget {BUDGET} B, {size - BUDGET:+d}); "
      f"partizione app usata al {100 * size / APP_PARTITION:.1f}%")
sys.exit(0 if size <= BUDGET else 1)
