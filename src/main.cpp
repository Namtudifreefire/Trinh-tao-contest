// main.cpp - Điểm vào chương trình ojgen.
//   ojgen            -> menu tương tác
//   ojgen --help ... -> chế độ dòng lệnh
#include <exception>
#include <string>
#include <vector>

#include "cli.h"
#include "console.h"
#include "platform.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#endif

int main(int argc, char** argv) {
    using namespace ojgen;
    con::init();
    std::vector<std::string> args;
#ifdef _WIN32
    // Tắt hộp thoại "chương trình ngừng hoạt động" (thiết lập này được tiến trình con kế thừa).
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    // Lấy đối số dạng UTF-16 để giữ đúng đường dẫn có dấu tiếng Việt.
    int wargc = 0;
    LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (wargv) {
        for (int i = 1; i < wargc; ++i) args.push_back(platform::narrow(wargv[i]));
        LocalFree(wargv);
    }
    (void)argc;
    (void)argv;
#else
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
#endif
    try {
        if (args.empty()) return runMenu();
        return runCli(args);
    } catch (const con::InputClosed&) {
        return 0;
    } catch (const std::exception& e) {
        con::printErr(std::string("LỖI không mong đợi: ") + e.what());
        return 3;
    }
}
