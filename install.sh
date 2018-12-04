#! /usr/bin/env bash

set -e

SCRIPT_DIR="$(realpath -s "$(dirname "$0")")"

export CFLAGS='-O3 -fomit-frame-pointer -fstrict-aliasing'
export CXXFLAGS='-O3 -fomit-frame-pointer -fstrict-aliasing'

if [ -x "$(command -v clang 2>/dev/null)" ]; then
  export CC=clang
  export CXX=clang++
  export AR=llvm-ar
  export RANLIB=llvm-ranlib
  export CFLAGS="$CFLAGS -flto"
  export CXXFLAGS="$CXXFLAGS -flto"
  export LDFLAGS="$LDFLAGS -flto"
fi

sudo apt-get install -y --no-install-recommends libelf-dev libevdev-dev

cd "$SCRIPT_DIR/Reader"
rm -rf .build
mkdir -p .build
cd .build
cmake ../
make -j 4
cp -f KeyboardHookReader $HOME/bin

cd "$SCRIPT_DIR/Writer"
make clean || true
make
sudo cp -f keyboard_hook_writer.ko /lib/modules/`uname -r`/kernel/drivers/input
echo "keyboard_hook_writer" | sudo tee /etc/modules-load.d/keyboard_hook_writer.conf
sudo depmod
