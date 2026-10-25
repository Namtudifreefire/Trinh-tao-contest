// console.cpp - Nhập/xuất console UTF-8 (Windows dùng WriteConsoleW/ReadConsoleW
// để hiển thị và nhập tiếng Việt có dấu chính xác).
#include "console.h"

#include <cstdio>
#include <cwchar>
#include <iostream>

#include "platform.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#else
#include <unistd.h>
#endif

namespace ojgen {

#ifdef _WIN32
namespace platform {
std::wstring widen(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &w[0], n);
    return w;
}
std::string narrow(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
    return s;
}
}  // namespace platform
#endif

namespace con {

#ifdef _WIN32
static bool isConsoleHandle(DWORD which) {
    HANDLE h = GetStdHandle(which);
    DWORD mode = 0;
    return h != INVALID_HANDLE_VALUE && h != nullptr && GetConsoleMode(h, &mode);
}
#endif

void init() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::ios::sync_with_stdio(true);
}

bool stdoutIsConsole() {
#ifdef _WIN32
    return isConsoleHandle(STD_OUTPUT_HANDLE);
#else
    return isatty(STDOUT_FILENO);
#endif
}

bool stdinIsConsole() {
#ifdef _WIN32
    return isConsoleHandle(STD_INPUT_HANDLE);
#else
    return isatty(STDIN_FILENO);
#endif
}

static void writeTo(FILE* f, const std::string& s) {
#ifdef _WIN32
    DWORD which = (f == stderr) ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
    if (isConsoleHandle(which)) {
        std::fflush(f);
        std::wstring w = platform::widen(s);
        DWORD written = 0;
        WriteConsoleW(GetStdHandle(which), w.data(), static_cast<DWORD>(w.size()), &written, nullptr);
        return;
    }
#endif
    std::fwrite(s.data(), 1, s.size(), f);
}

void print(const std::string& s) { writeTo(stdout, s); }
void println(const std::string& s) { writeTo(stdout, s + "\n"); }
void printErr(const std::string& s) {
    std::fflush(stdout);
    writeTo(stderr, s + "\n");
}
void flush() { std::fflush(stdout); }

void clearScreen() {
    if (!stdoutIsConsole()) return;
    std::fflush(stdout);
#ifdef _WIN32
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    // Windows 10+, Windows Terminal, VS Code: dùng mã điều khiển VT (xóa cả phần cuộn lên).
    if (GetConsoleMode(h, &mode) &&
        ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) || SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))) {
        const wchar_t seq[] = L"\x1b[2J\x1b[3J\x1b[H";
        DWORD written = 0;
        WriteConsoleW(h, seq, static_cast<DWORD>(wcslen(seq)), &written, nullptr);
        return;
    }
    // Console cũ (Windows 7/8): tô trắng toàn bộ bộ đệm rồi đưa con trỏ về góc trên.
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(h, &csbi)) return;
    DWORD cells = static_cast<DWORD>(csbi.dwSize.X) * static_cast<DWORD>(csbi.dwSize.Y), written = 0;
    COORD home = {0, 0};
    FillConsoleOutputCharacterW(h, L' ', cells, home, &written);
    FillConsoleOutputAttribute(h, csbi.wAttributes, cells, home, &written);
    SetConsoleCursorPosition(h, home);
#else
    const char seq[] = "\033[H\033[2J\033[3J";
    std::fwrite(seq, 1, sizeof seq - 1, stdout);
    std::fflush(stdout);
#endif
}

bool readLine(std::string& out) {
    flush();
    out.clear();
#ifdef _WIN32
    if (stdinIsConsole()) {
        HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
        std::wstring line;
        wchar_t buf[256];
        for (;;) {
            DWORD got = 0;
            if (!ReadConsoleW(h, buf, 256, &got, nullptr) || got == 0) {
                if (line.empty()) return false;
                break;
            }
            line.append(buf, got);
            if (!line.empty() && line.back() == L'\n') break;
        }
        while (!line.empty() && (line.back() == L'\n' || line.back() == L'\r')) line.pop_back();
        if (!line.empty() && line[0] == 0x1A) return false;  // Ctrl+Z
        out = platform::narrow(line);
        return true;
    }
#endif
    static bool first = true;
    if (!std::getline(std::cin, out)) return false;
    if (first) {
        first = false;
        if (out.size() >= 3 && static_cast<unsigned char>(out[0]) == 0xEF &&
            static_cast<unsigned char>(out[1]) == 0xBB && static_cast<unsigned char>(out[2]) == 0xBF)
            out.erase(0, 3);  // bỏ BOM UTF-8
    }
    while (!out.empty() && (out.back() == '\r' || out.back() == '\n')) out.pop_back();
    // Khi input đến từ file/pipe, in lại câu trả lời để log dễ đọc.
    if (!stdinIsConsole()) println(out);
    return true;
}

std::string ask(const std::string& prompt, const std::string& def) {
    print(prompt);
    if (!def.empty()) print(" [" + def + "]");
    print(": ");
    std::string line;
    if (!readLine(line)) throw InputClosed();
    line = trim(line);
    return line.empty() ? def : line;
}

ll askInt(const std::string& prompt, ll def, ll lo, ll hi, bool hasDefault) {
    for (;;) {
        std::string s = ask(prompt, hasDefault ? std::to_string(def) : "");
        ll v = 0;
        if (!parseLL(s, v)) {
            println("  ! Giá trị không hợp lệ. Hãy nhập một số nguyên (vd: 100, -20, 1e9).");
            continue;
        }
        if (v < lo || v > hi) {
            println("  ! Giá trị phải nằm trong đoạn [" + std::to_string(lo) + ", " + std::to_string(hi) + "].");
            continue;
        }
        return v;
    }
}

bool askYesNo(const std::string& prompt, bool def) {
    for (;;) {
        print(prompt + (def ? " (Y/n): " : " (y/N): "));
        std::string line;
        if (!readLine(line)) throw InputClosed();
        std::string s = toLower(trim(line));
        if (s.empty()) return def;
        if (s == "y" || s == "yes" || s == "c" || s == "co" || s == "có") return true;
        if (s == "n" || s == "no" || s == "k" || s == "khong" || s == "không") return false;
        println("  ! Hãy nhập y (có) hoặc n (không).");
    }
}

std::string openFileDialog() {
#ifdef _WIN32
    wchar_t file[4096] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner = GetConsoleWindow();
    ofn.lpstrFilter =
        L"Mã nguồn C++ (*.cpp;*.cc;*.cxx)\0*.cpp;*.cc;*.cxx\0"
        L"Chương trình (*.exe)\0*.exe\0Tất cả (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = 4096;
    ofn.lpstrTitle = L"Chọn file lời giải chuẩn";
    // OFN_NOCHANGEDIR: không để hộp thoại đổi thư mục làm việc của ojgen.
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&ofn)) return platform::narrow(file);
#endif
    return "";
}

std::string askPath(const std::string& prompt, const std::string& def) {
    std::string s = ask(prompt, def);
    if (s == "*") {
#ifdef _WIN32
        std::string picked = openFileDialog();
        if (picked.empty()) println("  (Đã hủy hộp thoại chọn file.)");
        else println("  Đã chọn: " + picked);
        return picked;
#else
        println("  (Hộp thoại chọn file chỉ hỗ trợ trên Windows. Hãy nhập đường dẫn.)");
        return askPath(prompt, def);
#endif
    }
    // Bỏ dấu ngoặc kép/đơn bao quanh (kéo-thả file vào cửa sổ console).
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')))
        s = s.substr(1, s.size() - 2);
    if (!s.empty() && s[0] == '&') {  // PowerShell: & 'C:\a b\x.cpp'
        std::string t = trim(s.substr(1));
        if (t.size() >= 2 && (t.front() == '\'' || t.front() == '"') && t.back() == t.front())
            s = t.substr(1, t.size() - 2);
    }
    return s;
}

void pause(const std::string& msg) {
    print(msg);
    std::string dummy;
    if (!readLine(dummy)) throw InputClosed();
}

}  // namespace con
}  // namespace ojgen
