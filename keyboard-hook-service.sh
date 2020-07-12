#! /usr/bin/env bash

device_ids=$(KeyboardHookReader | grep -i "keyboard" | cut -d' ' -f2 | perl -pe 's/.*([0-9]+)$/\1/')

for i in $device_ids; do
    echo "/dev/input/event$i"
    KeyboardHookReader -i $i &
done