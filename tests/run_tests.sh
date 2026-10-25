#!/usr/bin/env bash
# tests/run_tests.sh - Kiểm thử đầu-cuối cho ojgen (Linux, macOS, hoặc MSYS2 bash trên Windows).
# Chạy từ thư mục gốc project:  bash tests/run_tests.sh
# Thư mục output thử nghiệm: output/_tests (bị xóa và tạo lại mỗi lần chạy).
set -u
cd "$(dirname "$0")/.."

# Trên Windows (MSYS2/Git Bash) dùng ojgen.exe, nơi khác dùng ./ojgen.
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) BIN=./ojgen.exe ;;
    *) BIN=./ojgen ;;
esac
PASS=0
FAIL=0
ok()   { PASS=$((PASS + 1)); echo "  [OK]   $1"; }
bad()  { FAIL=$((FAIL + 1)); echo "  [FAIL] $1"; }
expect_exit() {  # expect_exit <mã mong đợi> <mô tả> <lệnh...>
    local want=$1 what=$2; shift 2
    "$@" > "$LOG" 2>&1
    local got=$?
    if [[ $got -eq $want ]]; then ok "$what (exit $got)"; else bad "$what: exit $got, mong đợi $want (xem $LOG)"; fi
}
count() { ls "$1" 2>/dev/null | grep -c "$2$" || true; }

echo "== Build"
bash ./build.sh > /dev/null || { echo "Build thất bại"; exit 1; }

T=output/_tests
rm -rf "$T"
mkdir -p "$T"
LOG="$T/last.log"
COMMON=(--no-zip)

echo "== Tự kiểm tra nội bộ"
expect_exit 0 "--selftest" "$BIN" --selftest

echo "== Lệnh ví dụ trong yêu cầu"
expect_exit 0 "--list" "$BIN" --list
expect_exit 0 "--problem array --tests 20 --seed 2024" "$BIN" --problem array --tests 20 --seed 2024 --out "$T/array"
[[ $(count "$T/array" .inp) -eq 20 && -f "$T/array.zip" ]] && ok "array: 20 file .inp + ZIP" || bad "array: thiếu file"
expect_exit 0 "--problem custom --tests 100 --seed 2024" "$BIN" --problem custom --tests 100 --seed 2024 --out "$T/custom"
[[ $(count "$T/custom" .inp) -eq 100 ]] && ok "custom: 100 file .inp" || bad "custom: số file sai"

echo "== Tái tạo bằng seed"
"$BIN" --problem custom --tests 100 --seed 2024 --out "$T/custom_again" "${COMMON[@]}" > /dev/null
diff -r -x ojgen_info.txt -x '*.zip' "$T/custom" "$T/custom_again" > /dev/null && ok "cùng seed -> cùng bộ test" || bad "cùng seed nhưng khác dữ liệu"

echo "== Lời giải chuẩn cho từng module"
for m in single_n two_ints array queries string custom graph; do
    expect_exit 0 "$m + lời giải mẫu" "$BIN" --problem $m --tests 15 --seed 1 --solution "examples/solutions/${m}_sol.cpp" --out "$T/sol_$m"
    [[ $(count "$T/sol_$m" .out) -eq 15 && -f "$T/sol_$m.zip" ]] && ok "$m: 15 file .out + ZIP" || bad "$m: thiếu .out/ZIP"
done

echo "== Giới hạn riêng từng vị trí"
expect_exit 0 "custom --bounds 1:10,50:100,-20:20" "$BIN" --problem custom --tests 60 --seed 5 --bounds "1:10,50:100,-20:20" --out "$T/bounds" "${COMMON[@]}"
viol=$(cat "$T"/bounds/*.inp | awk '{ if ($1<1||$1>10||$2<50||$2>100||$3<-20||$3>20||NF!=3) c++ } END { print c+0 }')
[[ "$viol" == "0" ]] && ok "60 test đều đúng giới hạn từng vị trí" || bad "$viol dòng vi phạm giới hạn"

echo "== Lời giải lỗi"
expect_exit 1 "lỗi biên dịch" "$BIN" --problem custom --tests 3 --solution tests/bad_compile.cpp --out "$T/ce" "${COMMON[@]}"
grep -q "BIÊN DỊCH THẤT BẠI" "$LOG" && [[ $(count "$T/ce" .out) -eq 0 ]] && ok "báo lỗi biên dịch, không tạo .out" || bad "xử lý lỗi biên dịch"
expect_exit 1 "crash ở một số test" "$BIN" --problem custom --tests 12 --seed 7 --n 3 --min 1 --max 8 --solution tests/crash_sol.cpp --out "$T/re" "${COMMON[@]}"
nin=$(count "$T/re" .inp); nout=$(count "$T/re" .out)
[[ $nin -eq 12 && $nout -lt 12 && $nout -gt 0 ]] && ok "test lỗi không có .out ($nout/12 có .out)" || bad "crash: $nout/12 .out"
expect_exit 1 "quá thời gian" "$BIN" --problem custom --tests 4 --seed 3 --n 2 --min 1 --max 9 --timeout 500 --solution tests/tle_sol.cpp --out "$T/tle" "${COMMON[@]}"
grep -q "TLE" "$LOG" && ok "báo TLE" || bad "không báo TLE"
expect_exit 1 "output vô hạn" "$BIN" --problem single_n --tests 2 --max-output-mb 2 --solution tests/big_output.cpp --out "$T/ole" "${COMMON[@]}"
grep -q "OLE" "$LOG" && ok "báo OLE" || bad "không báo OLE"
expect_exit 1 "mã thoát khác 0" "$BIN" --problem single_n --tests 2 --solution tests/nonzero_exit.cpp --out "$T/nz" "${COMMON[@]}"
expect_exit 0 "lời giải bỏ qua input (cảnh báo)" "$BIN" --problem single_n --tests 5 --solution tests/stdin_ignored.cpp --out "$T/ign" "${COMMON[@]}"
grep -q "giống hệt nhau" "$LOG" && ok "cảnh báo output giống hệt nhau" || bad "thiếu cảnh báo output giống nhau"
expect_exit 0 "freopen bọc #ifndef OJGEN" "$BIN" --problem custom --tests 3 --solution tests/freopen_sol.cpp --out "$T/fre" "${COMMON[@]}"

echo "== Tham số sai (phải báo lỗi, mã thoát 2)"
expect_exit 2 "--tests 0" "$BIN" --problem custom --tests 0
expect_exit 2 "min > max" "$BIN" --problem custom --min 10 --max 1
expect_exit 2 "bounds sai cú pháp" "$BIN" --problem custom --bounds "1:10,50:x"
expect_exit 2 "tham số không tồn tại" "$BIN" --problem array --foo 1
expect_exit 2 "đồ thị quá nhiều cạnh" "$BIN" --problem graph --minN 4 --maxN 4 --minM 7 --maxM 7
expect_exit 2 "dạng bài không tồn tại" "$BIN" --problem khong_co

echo "== Không ghi đè ngoài ý muốn"
"$BIN" --problem single_n --tests 3 --out "$T/keep" "${COMMON[@]}" > /dev/null
echo "file cua toi" > "$T/keep/ghi_chu.txt"
"$BIN" --problem single_n --tests 3 --out "$T/keep" "${COMMON[@]}" > /dev/null
[[ -d "$T/keep_2" ]] && ok "lần 2 tự tạo thư mục mới keep_2" || bad "không tạo thư mục mới"
"$BIN" --problem single_n --tests 2 --out "$T/keep" --force "${COMMON[@]}" > /dev/null
[[ -f "$T/keep/ghi_chu.txt" && $(count "$T/keep" .inp) -eq 2 ]] && ok "--force chỉ xóa .inp/.out, giữ file khác" || bad "--force xóa nhầm file"

echo "== ZIP"
if command -v unzip > /dev/null; then
    unzip -tq "$T/sol_custom.zip" > /dev/null && ok "unzip -t: ZIP hợp lệ" || bad "unzip -t báo lỗi"
    [[ $(unzip -Z1 "$T/sol_custom.zip" | wc -l) -eq 30 ]] && ok "ZIP có đúng 30 file" || bad "ZIP sai số file"
else
    echo "  (bỏ qua: không có lệnh unzip)"
fi
expect_exit 0 "--zip-dir" "$BIN" --zip-dir "$T/re"

echo
echo "Tổng: $PASS đạt, $FAIL thất bại."
[[ $FAIL -eq 0 ]]
