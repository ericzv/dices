#!/usr/bin/env bash
# Grava so o pacote de dados, sem recompilar o firmware.
# Uso:  tools/flash_data.sh [porta]
set -euo pipefail
PORTA="${1:-/dev/ttyACM0}"
OFFSET=0x210000        # tem de bater com a linha "tabnet" de partitions.csv
PACK=build/tabnet.pack

[ -f "$PACK" ] || { echo "rode build_pack.py antes"; exit 1; }
TAM=$(stat -c%s "$PACK")
echo "gravando $PACK ($TAM B) em $OFFSET via $PORTA"
esptool.py --chip esp32s3 --port "$PORTA" --baud 921600 \
    write_flash "$OFFSET" "$PACK"
echo "pronto - o Cardputer le a particao no proximo boot"
