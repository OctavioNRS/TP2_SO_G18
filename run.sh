#!/bin/bash
#  Uso:  ./run.sh [all|buddy]   (por defecto: all)

clear
rm -f Bootloader/Pure64/pure64.sys

if [ "$1" = "buddy" ]; then
  make clean buddy
else
  make clean all
fi

qemu-system-x86_64 \
  -hda Image/x64BareBonesImage.qcow2 \
  -m 4096 \
  -audiodev pa,id=speaker \
  -machine pcspk-audiodev=speaker

make clean
