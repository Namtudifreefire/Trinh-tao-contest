#!/usr/bin/env bash
# build.sh - Build ojgen trên Linux / macOS / MSYS2 shell.
#   ./build.sh            build bản phát hành (ojgen)
#   ./build.sh debug      build có thông tin debug + sanitizer
#   CXX=clang++ ./build.sh   dùng trình biên dịch khác
set -euo pipefail
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
if ! command -v "$CXX" >/dev/null 2>&1; then
    echo "LOI: khong tim thay trinh bien dich '$CXX'. Cai dat: sudo apt install g++  (hoac pacman -S mingw-w64-ucrt-x86_64-gcc trong MSYS2)" >&2
    exit 1
fi

FLAGS=(-std=c++17 -O2 -Wall -Wextra -Isrc/include)
OUT=ojgen
LIBS=()
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) OUT=ojgen.exe; FLAGS+=(-static); LIBS=(-lcomdlg32 -lshell32) ;;
esac
if [[ "${1:-}" == "debug" ]]; then
    FLAGS=(-std=c++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined -Isrc/include)
fi

mapfile -t SRCS < <(find src -name '*.cpp' | sort)
echo "Bien dich ${#SRCS[@]} file nguon bang $CXX ..."
"$CXX" "${FLAGS[@]}" "${SRCS[@]}" -o "$OUT" "${LIBS[@]}"
echo "OK: da tao $OUT"
echo "Chay menu: ./$OUT    |    Tro giup: ./$OUT --help    |    Tu kiem tra: ./$OUT --selftest"
