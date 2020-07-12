#! /usr/bin/env bash

set -e

sudo apt-get install -y --no-install-recommends libboost-all-dev libelf-dev libevdev-dev

SCRIPT_DIR="$(realpath -s "$(dirname "$0")")"

cd "$SCRIPT_DIR/Reader"
rm -rf .build
mkdir -p .build
cd .build
cmake ../
make -j 4
sudo cp -f KeyboardHookReader /usr/bin

cd "$SCRIPT_DIR/Writer"
make clean || true
make
sudo cp -f keyboard_hook_writer.ko /lib/modules/`uname -r`/kernel/drivers/input
echo "keyboard_hook_writer" | sudo tee /etc/modules-load.d/keyboard_hook_writer.conf
sudo depmod

cd "$SCRIPT_DIR"

sudo cp keyboard-hook-service.sh /etc
sudo cp keyboard-hook.service /etc/systemd/system
sudo systemctl enable --now keyboard-hook
