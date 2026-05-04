#!/bin/bash
clear
rm -f Bootloader/Pure64/pure64.sys
make

qemu-system-x86_64 \
  -hda Image/x64BareBonesImage.qcow2 \
  -m 2048 \
  -audiodev pa,id=speaker \
  -machine pcspk-audiodev=speaker