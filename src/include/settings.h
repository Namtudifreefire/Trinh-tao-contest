// settings.h - Cấu hình dùng chung (trình biên dịch, timeout, lời giải mặc định...).
// Lưu tại file ojgen_settings.ini trong thư mục làm việc hiện tại.
#pragma once

#include <map>
#include <string>

#include "common.h"

namespace ojgen {

struct Settings {
    std::string compiler = "g++";
    std::string cxxflags;          // rỗng = dùng defaultFlags()
    int timeoutMs = 2000;          // thời gian tối đa cho mỗi test
    int compileTimeoutSec = 120;
    ll maxOutputMB = 64;           // dung lượng output tối đa mỗi test
    ll memoryMB = 0;               // 0 = không giới hạn bộ nhớ
    bool normalizeEol = true;      // chuyển CRLF -> LF trong file .out
    bool zipCompress = true;       // nén Deflate (false = chỉ lưu, không nén)
    bool clearScreen = true;       // xóa màn hình khi chuyển giữa các mục menu
    std::string solution;          // đường dẫn lời giải mặc định gần nhất
    std::map<std::string, std::string> solutionByProblem;  // lời giải gần nhất theo từng dạng bài

    // Lời giải mặc định cho một dạng bài (rỗng nếu chưa dùng lần nào).
    std::string solutionFor(const std::string& problemId) const;
    std::string outputRoot = "output";
    std::string prefix = "test";

    std::string effectiveFlags() const;
    static std::string defaultFlags();

    static const char* fileName() { return "ojgen_settings.ini"; }
    // Đọc settings; nếu file chưa có thì giữ mặc định.
    static Settings load(std::string* warning = nullptr);
    bool save(std::string* err = nullptr) const;
    std::string describe() const;
};

}  // namespace ojgen
