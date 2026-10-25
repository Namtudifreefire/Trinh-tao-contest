// settings.cpp - Đọc/ghi ojgen_settings.ini (định dạng key=value, UTF-8).
#include "settings.h"

#include <sstream>

#include "file_manager.h"

namespace ojgen {

std::string Settings::defaultFlags() {
#ifdef _WIN32
    // -static: không cần libstdc++-6.dll khi chạy; --stack: tăng stack lên 256 MB
    // (mặc định của Windows chỉ 1-2 MB, dễ lỗi đệ quy sâu).
    return "-O2 -std=c++17 -static -Wl,--stack=268435456";
#else
    return "-O2 -std=c++17";
#endif
}

std::string Settings::solutionFor(const std::string& id) const {
    auto it = solutionByProblem.find(id);
    return it == solutionByProblem.end() ? std::string() : it->second;
}

std::string Settings::effectiveFlags() const { return trim(cxxflags).empty() ? defaultFlags() : cxxflags; }

Settings Settings::load(std::string* warning) {
    Settings s;
    std::string data;
    if (!fs::exists(u8path(fileName()))) return s;
    if (!readFile(u8path(fileName()), data)) {
        if (warning) *warning = std::string("Không đọc được ") + fileName() + ", dùng cấu hình mặc định.";
        return s;
    }
    std::istringstream is(data);
    std::string line;
    int lineNo = 0;
    std::string warn;
    while (std::getline(is, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (lineNo == 1 && line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) line.erase(0, 3);
        std::string t = trim(line);
        if (t.empty() || t[0] == '#' || t[0] == ';' || t[0] == '[') continue;
        size_t eq = t.find('=');
        if (eq == std::string::npos) continue;
        std::string k = trim(t.substr(0, eq)), v = trim(t.substr(eq + 1));
        ll num = 0;
        bool isNum = parseLL(v, num);
        if (k == "compiler") s.compiler = v;
        else if (k == "cxxflags") s.cxxflags = v;
        else if (k == "timeout_ms" && isNum && num >= 1 && num <= 3600000) s.timeoutMs = static_cast<int>(num);
        else if (k == "compile_timeout_sec" && isNum && num >= 1 && num <= 3600) s.compileTimeoutSec = static_cast<int>(num);
        else if (k == "max_output_mb" && isNum && num >= 1 && num <= 4096) s.maxOutputMB = num;
        else if (k == "memory_mb" && isNum && num >= 0 && num <= 1048576) s.memoryMB = num;
        else if (k == "normalize_eol" && isNum) s.normalizeEol = num != 0;
        else if (k == "zip_compress" && isNum) s.zipCompress = num != 0;
        else if (k == "clear_screen" && isNum) s.clearScreen = num != 0;
        else if (k == "solution") s.solution = v;
        else if (k.rfind("solution.", 0) == 0 && k.size() > 9) s.solutionByProblem[k.substr(9)] = v;
        else if (k == "output_root" && !v.empty()) s.outputRoot = v;
        else if (k == "prefix" && !v.empty()) s.prefix = v;
        else warn += "  dòng " + std::to_string(lineNo) + ": bỏ qua '" + t + "'\n";
    }
    if (warning && !warn.empty()) *warning = std::string("Cảnh báo trong ") + fileName() + ":\n" + warn;
    return s;
}

bool Settings::save(std::string* err) const {
    std::ostringstream os;
    os << "# Cấu hình ojgen (sửa trực tiếp hoặc qua menu 8). Dòng bắt đầu bằng # là chú thích.\n";
    os << "compiler=" << compiler << "\n";
    os << "# cxxflags để trống = dùng mặc định: " << defaultFlags() << "\n";
    os << "cxxflags=" << cxxflags << "\n";
    os << "timeout_ms=" << timeoutMs << "\n";
    os << "compile_timeout_sec=" << compileTimeoutSec << "\n";
    os << "max_output_mb=" << maxOutputMB << "\n";
    os << "memory_mb=" << memoryMB << "\n";
    os << "normalize_eol=" << (normalizeEol ? 1 : 0) << "\n";
    os << "zip_compress=" << (zipCompress ? 1 : 0) << "\n";
    os << "clear_screen=" << (clearScreen ? 1 : 0) << "\n";
    os << "solution=" << solution << "\n";
    for (const auto& kv : solutionByProblem) os << "solution." << kv.first << "=" << kv.second << "\n";
    os << "output_root=" << outputRoot << "\n";
    os << "prefix=" << prefix << "\n";
    return writeFile(u8path(fileName()), os.str(), err);
}

std::string Settings::describe() const {
    std::ostringstream os;
    os << "  Trình biên dịch       : " << compiler << "\n";
    os << "  Cờ biên dịch          : " << effectiveFlags() << (trim(cxxflags).empty() ? "  (mặc định)" : "") << "\n";
    os << "  Timeout mỗi test      : " << timeoutMs << " ms\n";
    os << "  Timeout biên dịch     : " << compileTimeoutSec << " giây\n";
    os << "  Output tối đa mỗi test: " << maxOutputMB << " MB\n";
    os << "  Giới hạn bộ nhớ       : " << (memoryMB > 0 ? std::to_string(memoryMB) + " MB" : "không giới hạn") << "\n";
    os << "  Chuẩn hóa CRLF -> LF  : " << (normalizeEol ? "có" : "không") << "\n";
    os << "  Nén ZIP (Deflate)     : " << (zipCompress ? "có" : "không (chỉ lưu)") << "\n";
    os << "  Xóa màn hình ở menu   : " << (clearScreen ? "có" : "không") << "\n";
    os << "  Lời giải mặc định     : " << (solution.empty() ? "(chưa đặt)" : solution) << "\n";
    os << "  Thư mục output        : " << outputRoot << "\n";
    os << "  Tiền tố tên test      : " << prefix << "\n";
    return os.str();
}

}  // namespace ojgen
