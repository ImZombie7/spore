#!/bin/sh
set -eu

if [ "$#" -ne 3 ]; then
  echo "usage: build.sh SOURCE_ROOT BUILD_DIR OUTPUT" >&2
  exit 2
fi

root=$1
build=$2
out=$3

jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cleaner="$build/elf-clean-runpath"
dash_src="$root/userland/third_party/dash"
dash_work="$build/dash-src"
dash_build="$build/dash-build"
stamp="$build/.dash.stamp"
stamp_new="$build/.dash.stamp.new"

mkdir -p "$build"
cc "$root/tools/src/elf_clean_runpath.c" -o "$cleaner"
{
  git -C "$dash_src" rev-parse HEAD
  cksum "$0" "$root/tools/src/elf_clean_runpath.c"
} >"$stamp_new"
if [ -f "$out" ] && [ -f "$stamp" ] && cmp -s "$stamp_new" "$stamp"; then
  rm -f "$stamp_new"
  exit 0
fi

rm -rf "$dash_work" "$dash_build"
mkdir -p "$dash_work" "$dash_build"
git -C "$dash_src" archive --format=tar HEAD | tar -x -C "$dash_work"

(
  cd "$dash_work"
  ./autogen.sh >/dev/null
)
(
  cd "$dash_build"
  "$dash_work/configure" \
    --host=aarch64-unknown-linux-musl \
    --prefix=/usr \
    --without-libedit \
    LDFLAGS="-Wl,-dynamic-linker,/lib/ld-musl-aarch64.so.1" \
    CC=aarch64-unknown-linux-musl-gcc >/dev/null
)
make -C "$dash_build" -j"$jobs" >/dev/null
aarch64-unknown-linux-musl-strip -o "$out" "$dash_build/src/dash"
"$cleaner" "$out"
mv "$stamp_new" "$stamp"
