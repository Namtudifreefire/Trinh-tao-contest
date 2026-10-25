# ojgen — Công cụ sinh test cho bài lập trình thi đấu

`ojgen` sinh bộ test (`test01.inp`, `test01.out`, ...) cho các bài lập trình thi đấu (OJ):

- chọn dạng bài, số lượng test và giới hạn dữ liệu;
- sinh input ngẫu nhiên đúng định dạng, **mỗi file được kiểm tra lại** định dạng và giới hạn;
- biên dịch và chạy **lời giải chuẩn** để tạo output thật (có timeout, giới hạn dung lượng output);
- đóng gói toàn bộ thành file ZIP và **tự kiểm tra lại** file ZIP;
- tái tạo đúng bộ test bằng **seed**;
- dễ thêm dạng bài mới.

Ưu tiên Windows + VS Code + MSYS2 MinGW-w64; cũng chạy trên Linux/macOS. Viết bằng C++17, chỉ dùng thư viện chuẩn (ZIP được tự cài đặt, không cần thư viện ngoài).

---

## Mục lục

1. [Cài đặt trình biên dịch](#1-cài-đặt-trình-biên-dịch)
2. [Build project](#2-build-project)
3. [Chạy menu](#3-chạy-menu)
4. [Các dạng bài (module)](#4-các-dạng-bài-module)
5. [Sinh input và output](#5-sinh-input-và-output)
6. [Seed — tái tạo bộ test](#6-seed--tái-tạo-bộ-test)
7. [Đóng gói ZIP](#7-đóng-gói-zip)
8. [Dòng lệnh (CLI)](#8-dòng-lệnh-cli)
9. [File cấu hình `ojgen_settings.ini`](#9-file-cấu-hình-ojgen_settingsini)
10. [Thêm module mới](#10-thêm-module-mới)
11. [Các lỗi thường gặp](#11-các-lỗi-thường-gặp)
12. [Cấu trúc project](#12-cấu-trúc-project)
13. [Kiểm thử](#13-kiểm-thử)
14. [Lưu ý an toàn và giới hạn đã biết](#14-lưu-ý-an-toàn-và-giới-hạn-đã-biết)

---

## 1. Cài đặt trình biên dịch

### Windows (MSYS2 MinGW-w64)

1. Tải và cài MSYS2 từ <https://www.msys2.org> (mặc định cài vào `C:\msys64`).
2. Mở **MSYS2 UCRT64** từ menu Start, chạy:
   ```bash
   pacman -Syu
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc
   ```
3. Thêm `C:\msys64\ucrt64\bin` vào biến môi trường **PATH**
   (Start → gõ "environment variables" → *Edit the system environment variables* → *Environment Variables* → chọn `Path` → *New*).
4. Mở **cửa sổ lệnh mới** và kiểm tra: `g++ --version`.

> Nếu chưa thêm PATH, `build.bat` và `ojgen.exe` vẫn tự tìm g++ trong `C:\msys64\ucrt64\bin` và `C:\msys64\mingw64\bin`.

### Linux

```bash
sudo apt install g++        # Ubuntu/Debian
```

---

## 2. Build project

| Cách | Lệnh | Kết quả |
|---|---|---|
| Windows (cmd/PowerShell) | `build.bat` | `ojgen.exe` |
| Linux / macOS / MSYS2 bash | `./build.sh` | `ojgen` |
| VS Code | `Ctrl+Shift+B` (task **ojgen: Build**) | như trên |

Lệnh build tương đương (nếu muốn tự gõ trên Windows):

```bat
g++ -std=c++17 -O2 -static -Isrc/include src\*.cpp src\core\*.cpp src\problems\*.cpp -o ojgen.exe -lcomdlg32 -lshell32
```

Các task VS Code có sẵn trong `.vscode/tasks.json` (*Terminal → Run Task...*):

- **ojgen: Build** — build (mặc định của `Ctrl+Shift+B`);
- **ojgen: Mở menu** — build rồi mở menu;
- **ojgen: Tự kiểm tra (--selftest)**;
- **ojgen: Kiểm thử đầy đủ** — chạy `tests/run_tests.sh` (cần bash: Linux hoặc MSYS2);
- **ojgen: Sinh test cho file đang mở (custom)** — dùng file `.cpp` đang mở làm lời giải chuẩn.

Gói phát hành có kèm sẵn `ojgen.exe` (build bằng MinGW-w64, liên kết tĩnh) để dùng ngay; vẫn nên tự build lại bằng `build.bat` sau khi cài MSYS2.

---

## 3. Chạy menu

Chạy `ojgen.exe` (hoặc `./ojgen`) **không kèm tham số** để mở menu:

```text
  1. Sinh test: một số nguyên n
  2. Sinh test: hai số nguyên
  3. Sinh test: mảng một chiều
  4. Sinh test: mảng và truy vấn
  5. Sinh test: xâu ký tự
  6. Sinh test tùy chỉnh: nhiều số nguyên, giới hạn chung hoặc riêng từng vị trí
  7. Sinh test: đồ thị
  8. Quản lý lời giải chuẩn và cấu hình chạy
  9. Hướng dẫn sử dụng
  0. Thoát
```

Mỗi khi chọn một mục (ở menu chính và menu con mục 8), màn hình được xóa và chuyển sang màn hình của mục đó; nhấn Enter ở cuối mỗi mục để quay lại menu. Tắt tính năng này ở menu 8 mục 8 hoặc đặt `clear_screen=0` trong `ojgen_settings.ini`. Khi output được chuyển sang file/pipe (vd `ojgen > log.txt`), ojgen không xóa màn hình để log không bị chèn ký tự điều khiển.

Mẹo nhập liệu:

- Nhấn **Enter** để giữ giá trị mặc định hiển thị trong `[ ]`.
- Số có thể nhập dạng `1000000000` hoặc `1e9`; số âm như `-20`.
- Đường dẫn lời giải: gõ trực tiếp, **kéo-thả file** vào cửa sổ (dấu ngoặc kép tự bỏ), hoặc gõ `*` để mở **hộp thoại chọn file** (Windows).

### Ví dụ mục 6 — giới hạn riêng từng vị trí

```text
Bước 1/6: Số lượng test case
Bạn muốn tạo bao nhiêu test case? [10]: 100

Bước 2/6: Số lượng số nguyên trong mỗi test
Mỗi test case có bao nhiêu số nguyên? [10]: 3

Bước 3/6: Chọn chế độ giới hạn:
1. Dùng chung giới hạn cho tất cả số
2. Đặt giới hạn riêng cho từng vị trí
Lựa chọn [1]: 2

Số thứ 1:
  Giá trị nhỏ nhất [1]: 1
  Giá trị lớn nhất [100]: 10

Số thứ 2:
  Giá trị nhỏ nhất [1]: 50
  Giá trị lớn nhất [50]: 100

Số thứ 3:
  Giá trị nhỏ nhất [50]: -20
  Giá trị lớn nhất [100]: 20

Thay đổi định dạng file input? (y/N):

Bước 4/6: Lời giải chuẩn
Bạn có muốn tạo file output bằng lời giải chuẩn? (Y/n): y
Đường dẫn file lời giải (.cpp/.exe; nhập * để mở hộp thoại chọn file): D:\OJ\solution.cpp

Bước 5/6: Sinh test (seed và thư mục lưu)
Seed (Enter = ngẫu nhiên; nhập lại seed cũ để tái tạo bộ test): 2024
Tên thư mục chứa test (trong output) [custom]:
...
Bước 6/6: Xuất file
KẾT QUẢ: HOÀN CHỈNH (đủ .inp và .out)
Tạo file ZIP chứa toàn bộ test? (Y/n):
Đã tạo ZIP: output\custom.zip
```

Kết quả: 100 test, mỗi test 3 số; số thứ 1 ∈ [1, 10], số thứ 2 ∈ [50, 100], số thứ 3 ∈ [−20, 20].

Với `n` lớn ở chế độ 2, nhấn Enter để dùng lại giới hạn của số ngay trước (mặc định trong `[ ]` là giới hạn của số trước).

### Mục 8 — Quản lý lời giải chuẩn

1. Đặt / đổi lời giải mặc định
2. Biên dịch thử lời giải (kiểm tra lỗi biên dịch)
3. Chạy thử lời giải với input nhập tay (kết thúc bằng dòng `.`)
4. Tạo lại `.out` cho một thư mục test có sẵn (vd sau khi sửa lời giải)
5. Đóng gói ZIP một thư mục test có sẵn
6. Đổi trình biên dịch / cờ biên dịch
7. Đổi timeout, dung lượng output tối đa, giới hạn bộ nhớ
8. Tùy chọn: nén ZIP, chuẩn hóa xuống dòng CRLF → LF, xóa màn hình khi chuyển mục

---

## 4. Các dạng bài (module)

Xem nhanh trong chương trình: `ojgen --list` và `ojgen --info <dạng>`.

### `single_n` — một số nguyên

```text
n
```

| Tham số | Ý nghĩa | Mặc định |
|---|---|---|
| `minVal`, `maxVal` | miền giá trị của n | 1, 10⁹ |

### `two_ints` — hai số nguyên

```text
a b
```

| Tham số | Ý nghĩa | Mặc định |
|---|---|---|
| `minA`, `maxA` | miền của a | 1, 10⁹ |
| `minB`, `maxB` | miền của b | 1, 10⁹ |
| `order` | 1 = bắt buộc a ≤ b | 0 |

### `array` — mảng một chiều

```text
n
a1 a2 ... an
```

| Tham số | Ý nghĩa | Mặc định |
|---|---|---|
| `minN`, `maxN` | số phần tử (1..10⁶) | 1, 10 |
| `minVal`, `maxVal` | giá trị phần tử | 1, 100 |
| `distinct` | 1 = đôi một khác nhau | 0 |
| `sorted` | 0 ngẫu nhiên, 1 không giảm, 2 không tăng | 0 |

### `queries` — mảng và truy vấn

```text
n q
a1 a2 ... an
q dòng: "1 i x" (gán a[i] = x) hoặc "2 l r" (hỏi đoạn [l, r], 1 ≤ l ≤ r ≤ n)
```

| Tham số | Ý nghĩa | Mặc định |
|---|---|---|
| `minN`, `maxN` | số phần tử | 1, 10 |
| `minVal`, `maxVal` | giá trị phần tử và x | 1, 100 |
| `minQ`, `maxQ` | số truy vấn | 1, 10 |
| `updatePct` | % truy vấn loại 1 (0..100). Nếu < 100, mỗi test có ít nhất một truy vấn loại 2 | 30 |

### `string` — xâu ký tự

```text
[L]     (chỉ khi printLen = 1)
s
```

| Tham số | Ý nghĩa | Mặc định |
|---|---|---|
| `minLen`, `maxLen` | độ dài xâu (1..10⁷) | 1, 20 |
| `charset` | `lower`, `upper`, `letters`, `digits`, `alnum`, `binary`, `dna`, hoặc tự liệt kê ký tự (vd `abc`; dùng `set:lower` nếu muốn đúng 5 ký tự l,o,w,e,r) | `lower` |
| `printLen` | 1 = ghi độ dài ở dòng đầu | 0 |

### `custom` — dãy n số, giới hạn chung hoặc riêng (mục 6)

```text
[n]                (chỉ khi header = 1)
x1 x2 ... xn       (layout = 1; layout = 2: mỗi số một dòng)
```

| Tham số | Ý nghĩa | Mặc định |
|---|---|---|
| `n` | số lượng số nguyên trong **mỗi** test (khác với số test) | 10 |
| `mode` | 1 = giới hạn chung, 2 = giới hạn riêng | 1 |
| `min`, `max` | giới hạn chung (mode 1) | 1, 100 |
| `bounds` | giới hạn riêng (mode 2): `"lo1:hi1,lo2:hi2,..."` | — |
| `header` | 1 = ghi n ở dòng đầu | 0 |
| `layout` | 1 = cùng một dòng, 2 = mỗi số một dòng | 1 |

Giá trị hỗ trợ toàn bộ phạm vi `long long` (−9 223 372 036 854 775 808 .. 9 223 372 036 854 775 807), kể cả số âm.

### `graph` — đồ thị

```text
n m
m dòng: u v      (hoặc u v w khi weighted = 1), 1 ≤ u, v ≤ n
```

| Tham số | Ý nghĩa | Mặc định |
|---|---|---|
| `minN`, `maxN` | số đỉnh (1..200 000) | 2, 8 |
| `minM`, `maxM` | số cạnh (0..10⁶) | 1, 10 |
| `directed` | 1 = có hướng | 0 |
| `simple` | 1 = đồ thị đơn: không khuyên, không cạnh lặp | 1 |
| `connected` | 1 = liên thông (có hướng: liên thông yếu), cần m ≥ n − 1 | 0 |
| `weighted`, `minW`, `maxW` | trọng số cạnh | 0, 1, 10⁹ |

ojgen kiểm tra tính khả thi trước khi sinh: đồ thị đơn vô hướng n đỉnh có tối đa n(n−1)/2 cạnh (có hướng: n(n−1)); nếu cấu hình không thể thỏa mãn sẽ báo lỗi rõ ràng.

---

## 5. Sinh input và output

Quy trình mỗi lần sinh:

1. Kiểm tra cấu hình (miền từng tham số, ràng buộc giữa các tham số).
2. Sinh đủ số test. Mỗi file `.inp` được **xác thực lại** (định dạng, giới hạn, ràng buộc như phần tử khác nhau, đồ thị đơn/liên thông). Test trùng với test trước sẽ được sinh lại; nếu miền dữ liệu quá nhỏ (vd 100 test cho n ∈ [1, 5]) ojgen cảnh báo.
3. Nếu có lời giải chuẩn:
   - kiểm tra file tồn tại;
   - biên dịch bằng g++ (`-O2 -std=c++17`, trên Windows thêm `-static -Wl,--stack=268435456` để có stack 256 MB);
   - chạy lời giải cho từng file `.inp` (input qua **stdin**), lấy **stdout** ghi vào `.out` tương ứng;
   - kiểm tra mã thoát, thời gian (mặc định 2000 ms/test), dung lượng output (mặc định 64 MB/test).
4. Ghi `ojgen_info.txt` trong thư mục test: cấu hình, seed, lời giải, trạng thái, danh sách test lỗi.
5. Hỏi / tạo file ZIP.

**Ba trạng thái kết quả:**

| Trạng thái | Ý nghĩa |
|---|---|
| HOÀN CHỈNH | mọi test có `.inp` và `.out` |
| CHỈ CÓ INPUT | không dùng lời giải chuẩn → chỉ có `.inp`, **không có** `.out` |
| CHƯA HOÀN CHỈNH | lời giải lỗi biên dịch hoặc lỗi ở một số test (RE/TLE/OLE). Test lỗi **không có** `.out`; ojgen liệt kê từng test lỗi |

ojgen **không bao giờ** tự đoán, sao chép input sang output hay tạo output giả.

**Yêu cầu với lời giải chuẩn:**

- đọc từ stdin (`cin`, `scanf`), ghi ra stdout (`cout`, `printf`), trả về 0;
- nếu code dùng `freopen` kiểu thi học sinh giỏi, bọc lại như sau (ojgen tự thêm `-DOJGEN` khi biên dịch):
  ```cpp
  #ifndef OJGEN
      freopen("BAI1.INP", "r", stdin);
      freopen("BAI1.OUT", "w", stdout);
  #endif
  ```
- chấp nhận cả `.cpp/.cc/.cxx` (ojgen tự biên dịch) và `.exe` đã biên dịch sẵn;
- `#include "file.h"` nằm cạnh file lời giải được hỗ trợ.

**Cảnh báo tự động:** ojgen báo khi mọi output giống hệt nhau (thường do lời giải không đọc stdin) hoặc có output rỗng.

**Xuống dòng:** file `.inp` dùng LF (`\n`). Trên Windows, chương trình in ra CRLF; ojgen mặc định đổi về LF cho `.out` (tắt ở menu 8 mục 8 hoặc `normalize_eol=0`).

**Thư mục:** test lưu ở `output/<tên>/`. Nếu thư mục đã có test, menu hỏi *ghi đè / tạo thư mục mới / hủy*; CLI mặc định tạo thư mục mới `<tên>_2`, `<tên>_3`... (dùng `--force` để ghi đè). Khi ghi đè, ojgen **chỉ xóa** các file `.inp`, `.out`, `ojgen_info.txt` và file ZIP cùng tên; các file khác giữ nguyên.

**Biên dịch lại:** ojgen lưu bản đã biên dịch trong `output/.work/`; nếu mã nguồn, trình biên dịch và cờ không đổi thì dùng lại. Nếu chỉ sửa file header đi kèm, dùng `--rebuild` hoặc menu 8 mục 2.

---

## 6. Seed — tái tạo bộ test

- Mỗi lần sinh, ojgen in ra **seed** và ghi vào `ojgen_info.txt`.
- **Cùng seed + cùng cấu hình + cùng số test ⇒ cùng bộ test, giống từng byte**, kể cả giữa bản Windows và Linux (bộ sinh dùng `std::mt19937_64` với hàm chia khoảng tự cài đặt, không phụ thuộc thư viện chuẩn của trình biên dịch).
- Menu: nhập seed ở Bước "Sinh test" (Enter = ngẫu nhiên). CLI: `--seed 2024`.

---

## 7. Đóng gói ZIP

- Menu hỏi sau khi sinh xong; CLI tự tạo (tắt bằng `--no-zip`).
- File ZIP đặt cạnh thư mục test: `output/custom/` → `output/custom.zip`.
- ZIP **chỉ** chứa `.inp` và `.out` (không có file thực thi, file tạm, `ojgen_info.txt`), xếp phẳng (không có thư mục con).
- Sau khi tạo, ojgen đọc lại ZIP, giải nén từng file và so CRC-32 + danh sách file. Nếu không đạt, ZIP bị xóa và báo lỗi.
- Bộ test **chỉ có input**: ZIP chỉ chứa `.inp` và có thông báo rõ.
- Bộ test **chưa hoàn chỉnh**: mặc định không tạo ZIP (menu hỏi lại; CLI dùng `--zip-incomplete`).
- Nén Deflate (tắt: `--no-compress` hoặc menu 8 mục 8). Giới hạn: tổng < 4 GB và < 65 535 file.
- Đóng gói lại thư mục có sẵn: `ojgen --zip-dir output/custom` hoặc menu 8 mục 5.

---

## 8. Dòng lệnh (CLI)

```text
ojgen                              Mở menu tương tác
ojgen --list                       Liệt kê các dạng bài
ojgen --info <dạng>                Định dạng input và tham số của một dạng bài
ojgen --problem <dạng> [tùy chọn]  Sinh bộ test
ojgen --regen-out <thư mục> --solution <file>   Tạo lại .out cho thư mục có sẵn
ojgen --zip-dir <thư mục>          Đóng gói ZIP thư mục có sẵn
ojgen --selftest                   Tự kiểm tra
ojgen --help | --version
```

| Tùy chọn | Ý nghĩa |
|---|---|
| `--tests N` | số test (1..10 000, mặc định 10) |
| `--seed S` | seed (số nguyên không âm) |
| `--solution FILE` | lời giải chuẩn `.cpp`/`.exe`; bỏ trống = chỉ tạo `.inp` |
| `--out DIR` | thư mục chứa test (mặc định `output/<dạng>`) |
| `--force` | ghi đè test cũ |
| `--no-zip` / `--zip-incomplete` | không tạo ZIP / vẫn tạo khi chưa hoàn chỉnh |
| `--rebuild` | biên dịch lại lời giải |
| `--timeout MS`, `--max-output-mb M`, `--memory-mb M` | giới hạn chạy (chỉ cho lần chạy này) |
| `--compiler PATH`, `--cxxflags "..."` | trình biên dịch và cờ |
| `--prefix P` | tiền tố tên file (mặc định `test`) |
| `--no-compress` | ZIP không nén |
| `--<tham số> GIÁ_TRỊ` | tham số của dạng bài, vd `--minN 5`, `--bounds "1:10,50:100"`. Có thể viết `--key=value` |

Ví dụ:

```bat
ojgen --list
ojgen --problem array --tests 20 --seed 2024
ojgen --problem custom --tests 100 --seed 2024
ojgen --problem custom --tests 100 --n 20 --min -1000 --max 1000 --solution sol.cpp
ojgen --problem custom --tests 50 --bounds "1:10,50:100,-20:20" --solution "D:\OJ\bai 1\sol.cpp"
ojgen --problem graph --tests 30 --minN 5 --maxN 1000 --minM 4 --maxM 5000 --connected 1 --weighted 1 --solution graph.cpp
ojgen --regen-out output\custom --solution sol_moi.cpp
```

Với `--bounds`, nếu không ghi `--n` thì n = số vị trí trong bounds, và `mode` tự chuyển thành 2.

**Mã thoát:** 0 = thành công (hoàn chỉnh hoặc chỉ input), 1 = lời giải lỗi ở một số test, 2 = lỗi tham số/cấu hình. Thuận tiện khi viết script.

---

## 9. File cấu hình `ojgen_settings.ini`

Tạo tự động trong thư mục chạy ojgen khi bạn đổi cấu hình ở menu 8 hoặc dùng lời giải lần đầu:

```ini
compiler=g++
cxxflags=                 # để trống = mặc định -O2 -std=c++17 -static -Wl,--stack=268435456
timeout_ms=2000
compile_timeout_sec=120
max_output_mb=64
memory_mb=0               # 0 = không giới hạn
normalize_eol=1           # đổi CRLF -> LF trong .out
zip_compress=1
clear_screen=1            # xóa màn hình khi chuyển giữa các mục menu
solution=D:\OJ\solution.cpp
solution.custom=D:\OJ\solution.cpp   # lời giải gần nhất theo từng dạng bài
output_root=output
prefix=test
```

Tùy chọn CLI như `--timeout` chỉ áp dụng cho lần chạy đó, không ghi vào file.

---

## 10. Thêm module mới

File mẫu hoàn chỉnh, build được: `templates/new_problem.cpp.txt` (module `pairs` — n cặp `l r` với `l ≤ r`).

1. Sao chép thành `src/problems/pairs.cpp`.
2. Sửa `id()`, `title()`, `formatDescription()`, `params()`, `validateConfig()`, `generate()`, `validateInput()`.
3. Khai báo trong `src/include/problem_list.h`:
   ```cpp
   std::unique_ptr<Problem> makePairs();
   ```
4. Đăng ký trong `src/problems/registry.cpp`:
   ```cpp
   v.push_back(makePairs());
   ```
5. Build lại. Module xuất hiện trong `ojgen --list` và dùng được ngay:
   `ojgen --problem pairs --tests 20 --maxR 1000`.
   (Muốn có trong menu chính: thêm id vào mảng `kMenuIds` trong `src/core/menu.cpp` và một dòng menu.)

Quy tắc:

- `generate()` trả về toàn bộ nội dung một file input; mỗi dòng kết thúc `\n`, các token cách nhau đúng một dấu cách.
- Chỉ dùng đối tượng `Rng` được truyền vào (`rng.range(lo, hi)`, `rng.below(k)`, `rng.shuffle(v)`, `rng.chance(pct)`) — không dùng `rand()` để giữ tính tái tạo bằng seed.
- `validateInput()` phải kiểm tra lại mọi ràng buộc; dùng `splitInputLines()` và `readIntToken()`.
- Nên thêm vài cấu hình vào `src/core/selftest.cpp` để `--selftest` kiểm tra module mới.

---

## 11. Các lỗi thường gặp

| Hiện tượng | Nguyên nhân / cách xử lý |
|---|---|
| `build.bat` báo *Khong tim thay g++* | Chưa cài MSYS2 hoặc chưa cài gói gcc — xem mục 1 |
| `'g++' is not recognized...` | Chưa thêm `C:\msys64\ucrt64\bin` vào PATH, hoặc chưa mở lại cửa sổ lệnh |
| *Không tìm thấy chương trình 'g++'* khi tạo output | Như trên; hoặc đặt đường dẫn đầy đủ ở menu 8 mục 6, vd `C:\msys64\ucrt64\bin\g++.exe` |
| Mã thoát `0xC0000135` (DLL_NOT_FOUND) | Lời giải `.exe` biên dịch không có `-static` → dùng cờ mặc định của ojgen |
| `0xC00000FD` (STACK_OVERFLOW) | Đệ quy quá sâu; cờ mặc định đã tăng stack lên 256 MB. Nếu tự đặt `cxxflags`, giữ `-Wl,--stack=268435456` |
| `0xC0000005` (ACCESS_VIOLATION) / SIGSEGV | Lời giải truy cập ngoài mảng, con trỏ sai — sửa lời giải |
| Mọi test TLE | Lời giải chờ đọc file thay vì stdin (freopen) hoặc thật sự chậm; tăng `--timeout` nếu cần |
| *Tất cả output giống hệt nhau* | Lời giải không đọc stdin — kiểm tra freopen / ifstream |
| *Không có số đỉnh n nào...* (graph) | Số cạnh vượt tối đa của đồ thị đơn hoặc không đủ để liên thông — nới `maxN` hoặc giảm `minM` |
| *Miền [minVal, maxVal] chỉ có ... giá trị* | `distinct = 1` nhưng miền giá trị nhỏ hơn số phần tử |
| Cảnh báo *test trùng nhau* | Miền dữ liệu quá nhỏ so với số test — nới giới hạn hoặc giảm số test |
| Chữ tiếng Việt lỗi font trong console | Dùng Windows Terminal hoặc VS Code terminal; ojgen tự chuyển console sang UTF-8 |
| `build.bat` lỗi khi đường dẫn project chứa dấu `!` | Hạn chế của cmd (delayed expansion) — đổi tên thư mục |
| Windows Defender chặn/quét lâu file `sol_*.exe` | Bình thường với file exe mới biên dịch; có thể loại trừ thư mục `output\.work` |

---

## 12. Cấu trúc project

```text
ojgen/
├── src/
│   ├── main.cpp                 điểm vào: không tham số -> menu, có tham số -> CLI
│   ├── include/                 các header
│   │   ├── common.h  rng.h  console.h  platform.h  problem.h  problem_list.h
│   │   └── settings.h  file_manager.h  runner.h  generator.h  zip_manager.h  cli.h
│   ├── core/
│   │   ├── cli.cpp              phân tích tham số dòng lệnh
│   │   ├── menu.cpp             menu tương tác
│   │   ├── generator.cpp        sinh .inp, chống trùng, xác thực từng file
│   │   ├── job.cpp              quy trình: sinh -> chạy lời giải -> thông tin -> ZIP
│   │   ├── runner.cpp           biên dịch + chạy lời giải (CreateProcessW/Job Object; fork/exec)
│   │   ├── file_manager.cpp     file/thư mục an toàn, đường dẫn UTF-8
│   │   ├── zip_manager.cpp      ZIP + Deflate + CRC-32 tự cài đặt, kiểm tra lại ZIP
│   │   ├── console.cpp          nhập/xuất tiếng Việt trên console Windows
│   │   ├── settings.cpp         ojgen_settings.ini
│   │   ├── problem_base.cpp     phần chung của module + hàm hỗ trợ xác thực
│   │   ├── selftest.cpp         ojgen --selftest
│   │   └── util.cpp
│   └── problems/                mỗi dạng bài một file
│       ├── single_n.cpp  two_ints.cpp  array.cpp  queries.cpp
│       ├── string.cpp  custom.cpp  graph.cpp
│       └── registry.cpp         danh sách module
├── templates/new_problem.cpp.txt   mẫu module mới
├── examples/solutions/          lời giải mẫu cho từng dạng bài
├── tests/                       lời giải lỗi cố ý + run_tests.sh
├── output/                      nơi lưu test (output/.work: file tạm, bản biên dịch)
├── build.bat  build.sh
├── .vscode/tasks.json
└── README.md
```

---

## 13. Kiểm thử

- `ojgen --selftest` — 229 kiểm tra nội bộ: 28 cấu hình hợp lệ trên cả 7 module (sinh, xác thực, tái tạo bằng seed, phát hiện input hỏng), 12 cấu hình sai phải bị từ chối, nén/giải nén Deflate, CRC-32, tạo và kiểm tra ZIP.
- `bash tests/run_tests.sh` — 46 kiểm tra đầu-cuối: lệnh ví dụ, seed, lời giải mẫu cho 7 module, giới hạn riêng từng vị trí, lỗi biên dịch, crash, TLE, output vô hạn, mã thoát ≠ 0, cảnh báo, tham số sai, chống ghi đè, ZIP (`unzip -t`).
- `examples/solutions/` có lời giải mẫu để thử ngay, vd:
  `ojgen --problem queries --tests 20 --solution examples/solutions/queries_sol.cpp`.

---

## 14. Lưu ý an toàn và giới hạn đã biết

- **An toàn:** lời giải chuẩn là code do bạn cung cấp và chạy **trực tiếp trên máy với quyền của bạn**. ojgen chỉ giới hạn thời gian, dung lượng output, (tùy chọn) bộ nhớ, đặt thư mục làm việc của tiến trình vào `output/.work` và dọn tiến trình con khi hết giờ. Đây **không phải** môi trường cách ly (sandbox) — chỉ chạy code bạn tin tưởng.
- Tiến trình được tạo bằng danh sách đối số (không qua shell), nên đường dẫn có dấu cách, `&`, `%`, dấu tiếng Việt được truyền nguyên vẹn. Mã nguồn được chép sang tên ASCII trong `output/.work` trước khi biên dịch để tránh lỗi đường dẫn Unicode ở một số bản g++ MinGW.
- Thời gian đo là thời gian thực (wall-clock), bao gồm thời gian khởi động tiến trình.
- Giới hạn bộ nhớ (`memory_mb`) trên Windows dùng Job Object, trên Linux dùng `RLIMIT_AS`; lỗi vượt bộ nhớ hiện ra dưới dạng RE kèm gợi ý "có thể đã vượt giới hạn bộ nhớ".
- ZIP không hỗ trợ ZIP64 (tổng < 4 GB).
- Menu chính có 7 dạng bài cố định; module mới dùng qua CLI (hoặc tự thêm mục menu).
- Chỉ hỗ trợ lời giải C++ (hoặc `.exe` đã biên dịch).
