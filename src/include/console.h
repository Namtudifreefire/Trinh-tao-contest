// console.h - Nhập/xuất trên console, hỗ trợ tiếng Việt (UTF-8) trên Windows.
#pragma once

#include <exception>
#include <string>

#include "common.h"

namespace ojgen::con {

// Ném ra khi người dùng đóng input (Ctrl+Z / Ctrl+D hoặc hết dữ liệu pipe).
struct InputClosed : std::exception {
    const char* what() const noexcept override { return "input closed"; }
};

void init();
bool stdoutIsConsole();
bool stdinIsConsole();

void print(const std::string& utf8);
void println(const std::string& utf8 = "");
void printErr(const std::string& utf8);  // ghi ra stderr, kèm xuống dòng
void flush();
// Xóa màn hình console (chỉ khi stdout là console thật; khi xuất ra file/pipe thì bỏ qua
// để log không bị chèn ký tự điều khiển).
void clearScreen();

// Đọc một dòng (UTF-8, đã bỏ \r\n). Trả về false nếu hết input.
bool readLine(std::string& out);

// Hỏi chuỗi; Enter = giá trị mặc định (hiển thị trong [ ]).
std::string ask(const std::string& prompt, const std::string& def = "");
// Hỏi số nguyên trong [lo, hi], lặp lại tới khi hợp lệ.
ll askInt(const std::string& prompt, ll def, ll lo, ll hi, bool hasDefault = true);
bool askYesNo(const std::string& prompt, bool def);
// Hỏi đường dẫn file. Bỏ dấu ngoặc kép (khi kéo-thả file vào console).
// Trên Windows: nhập * để mở hộp thoại chọn file.
std::string askPath(const std::string& prompt, const std::string& def = "");

// Mở hộp thoại chọn file (chỉ Windows). Trả về "" nếu hủy/không hỗ trợ.
std::string openFileDialog();

void pause(const std::string& msg = "Nhấn Enter để tiếp tục...");

}  // namespace ojgen::con
