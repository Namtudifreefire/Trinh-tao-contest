// file_manager.h - Thao tác file/thư mục an toàn (UTF-8, không ghi đè ngoài ý muốn).
#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "common.h"

namespace ojgen {

namespace fs = std::filesystem;

// Chuyển chuỗi UTF-8 <-> fs::path (quan trọng trên Windows với tên có dấu).
fs::path u8path(const std::string& utf8);
std::string pathStr(const fs::path& p);

bool readFile(const fs::path& p, std::string& out, std::string* err = nullptr);
// Ghi nhị phân (giữ nguyên \n, không chuyển thành \r\n trên Windows).
bool writeFile(const fs::path& p, const std::string& data, std::string* err = nullptr);
// Di chuyển file; nếu khác ổ đĩa thì sao chép rồi xóa. Ghi đè đích nếu có.
bool moveFile(const fs::path& from, const fs::path& to, std::string* err = nullptr);

// Tên file test: prefix + số thứ tự có số 0 ở đầu, vd test01, test001.
int testNumberWidth(int testCount);
std::string testName(const std::string& prefix, int index, int width);

// Liệt kê file theo đuôi (vd ".inp") trong một thư mục (không đệ quy),
// sắp xếp tự nhiên (test2 trước test10).
std::vector<fs::path> listFilesWithExt(const fs::path& dir, const std::string& ext);

// Đếm file .inp/.out hiện có trong thư mục.
size_t countTestFiles(const fs::path& dir);
// Xóa CHỈ các file .inp, .out và file thông tin ojgen trong thư mục (không đệ quy).
// Trả về số file đã xóa, -1 nếu lỗi.
long clearTestFiles(const fs::path& dir, std::string* err = nullptr);
// Tìm tên thư mục chưa tồn tại: base, base_2, base_3, ...
fs::path uniqueDir(const fs::path& base);

constexpr const char* kInfoFile = "ojgen_info.txt";

}  // namespace ojgen
