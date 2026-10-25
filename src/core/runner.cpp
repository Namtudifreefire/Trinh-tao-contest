// runner.cpp - Biên dịch và chạy lời giải chuẩn.
#include "runner.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <set>
#include <sstream>
#include <thread>

#include "console.h"
#include "platform.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace ojgen {

using Clock = std::chrono::steady_clock;

std::string statusName(RunStatus s) {
    switch (s) {
        case RunStatus::Ok: return "OK";
        case RunStatus::RuntimeError: return "Lỗi khi chạy (RE)";
        case RunStatus::Timeout: return "Quá thời gian (TLE)";
        case RunStatus::OutputLimit: return "Output quá lớn (OLE)";
        case RunStatus::StartFailed: return "Không khởi chạy được";
    }
    return "?";
}

// Tách chuỗi cờ biên dịch thành danh sách đối số, hỗ trợ "..." cho giá trị có dấu cách.
static std::vector<std::string> splitArgs(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    bool inQ = false, has = false;
    for (char c : s) {
        if (c == '"') {
            inQ = !inQ;
            has = true;
        } else if (!inQ && (c == ' ' || c == '\t')) {
            if (has) out.push_back(cur), cur.clear(), has = false;
        } else {
            cur += c;
            has = true;
        }
    }
    if (has) out.push_back(cur);
    return out;
}

#ifdef _WIN32
// Trích dẫn đối số theo quy tắc của CommandLineToArgvW / CRT của MSVC & MinGW.
static std::wstring quoteArg(const std::wstring& a) {
    if (!a.empty() && a.find_first_of(L" \t\n\v\"") == std::wstring::npos) return a;
    std::wstring r = L"\"";
    for (size_t i = 0;; ++i) {
        size_t bs = 0;
        while (i < a.size() && a[i] == L'\\') ++i, ++bs;
        if (i == a.size()) {
            r.append(bs * 2, L'\\');
            break;
        }
        if (a[i] == L'"') {
            r.append(bs * 2 + 1, L'\\');
            r += L'"';
        } else {
            r.append(bs, L'\\');
            r += a[i];
        }
    }
    r += L'"';
    return r;
}

static std::string ntStatusText(DWORD code) {
    switch (code) {
        case 0xC0000005: return "truy cập bộ nhớ không hợp lệ (ACCESS_VIOLATION) - thường do vượt chỉ số mảng";
        case 0xC00000FD: return "tràn stack (STACK_OVERFLOW) - đệ quy quá sâu";
        case 0xC0000094: return "chia cho 0 (INTEGER_DIVIDE_BY_ZERO)";
        case 0xC0000095: return "tràn số nguyên (INTEGER_OVERFLOW)";
        case 0xC000008E: return "chia số thực cho 0";
        case 0xC0000017: return "không đủ bộ nhớ (NO_MEMORY)";
        case 0xC0000409: return "lỗi kiểm tra bộ đệm / abort (STACK_BUFFER_OVERRUN)";
        case 0xC0000135: return "thiếu DLL (DLL_NOT_FOUND) - hãy biên dịch với -static";
        case 0xC000013A: return "bị ngắt bằng Ctrl+C";
        case 0xC0000139: return "không tìm thấy hàm trong DLL (ENTRYPOINT_NOT_FOUND)";
        case 3: return "thường do abort()/assert thất bại (hoặc chương trình tự trả về 3)";
        default: return "";
    }
}

RunResult runProcess(const std::vector<std::string>& argv, const fs::path& inFile, const fs::path& outFile,
                     const fs::path& errFile, const fs::path& workDir, const RunLimits& limits) {
    RunResult res;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof sa;
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    HANDLE hIn = inFile.empty()
                     ? CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr)
                     : CreateFileW(inFile.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hIn == INVALID_HANDLE_VALUE) {
        res.message = "Không mở được file input: " + pathStr(inFile);
        return res;
    }
    HANDLE hOut = CreateFileW(outFile.wstring().c_str(), GENERIC_WRITE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ, &sa, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hOut == INVALID_HANDLE_VALUE) {
        CloseHandle(hIn);
        res.message = "Không tạo được file output tạm: " + pathStr(outFile);
        return res;
    }
    HANDLE hErr = hOut;
    bool sameErr = (errFile == outFile);
    if (!sameErr) {
        hErr = CreateFileW(errFile.wstring().c_str(), GENERIC_WRITE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ, &sa, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hErr == INVALID_HANDLE_VALUE) {
            CloseHandle(hIn);
            CloseHandle(hOut);
            res.message = "Không tạo được file stderr tạm.";
            return res;
        }
    }
    auto closeIo = [&] {
        CloseHandle(hIn);
        CloseHandle(hOut);
        if (!sameErr) CloseHandle(hErr);
    };

    std::wstring cmd;
    for (size_t i = 0; i < argv.size(); ++i) {
        if (i) cmd += L' ';
        cmd += quoteArg(platform::widen(argv[i]));
    }
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(L'\0');

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hIn;
    si.hStdOutput = hOut;
    si.hStdError = hErr;
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof pi);

    // Job object: giết toàn bộ cây tiến trình khi hết giờ, tắt hộp thoại báo lỗi
    // của Windows khi chương trình crash, và (tùy chọn) giới hạn bộ nhớ.
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION li;
        ZeroMemory(&li, sizeof li);
        li.BasicLimitInformation.LimitFlags =
            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
        if (limits.memoryBytes > 0) {
            li.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
            li.ProcessMemoryLimit = static_cast<SIZE_T>(limits.memoryBytes);
        }
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &li, sizeof li);
    }

    std::wstring wdir = workDir.wstring();
    BOOL created = CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE,
                                  CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr,
                                  wdir.empty() ? nullptr : wdir.c_str(), &si, &pi);
    if (!created) {
        DWORD e = GetLastError();
        closeIo();
        if (job) CloseHandle(job);
        res.status = RunStatus::StartFailed;
        if (e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND)
            res.message = "Không tìm thấy chương trình '" + argv[0] + "'. Kiểm tra PATH (vd C:\\msys64\\ucrt64\\bin).";
        else
            res.message = "CreateProcess thất bại (mã lỗi Windows " + std::to_string(e) + ").";
        return res;
    }
    if (job) AssignProcessToJobObject(job, pi.hProcess);  // có thể thất bại trên Windows cũ: vẫn tiếp tục
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);

    auto start = Clock::now();
    res.status = RunStatus::Ok;
    for (;;) {
        DWORD w = WaitForSingleObject(pi.hProcess, 10);
        LARGE_INTEGER sz;
        if (GetFileSizeEx(hOut, &sz) && static_cast<ull>(sz.QuadPart) > limits.maxOutputBytes) {
            res.status = RunStatus::OutputLimit;
            break;
        }
        if (w == WAIT_OBJECT_0) break;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
        if (ms > limits.timeoutMs) {
            res.status = RunStatus::Timeout;
            break;
        }
    }
    if (res.status != RunStatus::Ok) {
        if (job) TerminateJobObject(job, 1);
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 5000);
    }
    res.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    res.exitCode = static_cast<long long>(code);
    CloseHandle(pi.hProcess);
    if (job) CloseHandle(job);  // KILL_ON_JOB_CLOSE: dọn các tiến trình con còn sót
    closeIo();

    if (res.status == RunStatus::Timeout)
        res.message = "Chạy quá " + std::to_string(limits.timeoutMs) + " ms nên đã bị dừng.";
    else if (res.status == RunStatus::OutputLimit)
        res.message = "Output vượt " + fmtBytes(limits.maxOutputBytes) + " nên đã bị dừng.";
    else if (code != 0) {
        res.status = RunStatus::RuntimeError;
        char hex[16];
        std::snprintf(hex, sizeof hex, "0x%08lX", static_cast<unsigned long>(code));
        std::string t = ntStatusText(code);
        res.message = "Mã thoát " + std::to_string(code) + " (" + hex + ")" + (t.empty() ? "" : ": " + t);
        if (limits.memoryBytes > 0 && (code == 0xC0000017 || code == 0xC0000005 || code == 3))
            res.message += ". Có thể đã vượt giới hạn bộ nhớ " + fmtBytes(limits.memoryBytes) + ".";
    }
    return res;
}

#else  // ---------------------------- POSIX ----------------------------------

static std::string signalText(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV - truy cập bộ nhớ không hợp lệ (vượt chỉ số mảng, tràn stack)";
        case SIGFPE: return "SIGFPE - lỗi số học (chia cho 0)";
        case SIGABRT: return "SIGABRT - chương trình gọi abort() (assert thất bại, bad_alloc...)";
        case SIGBUS: return "SIGBUS - lỗi bus";
        case SIGKILL: return "SIGKILL - bị hệ thống dừng (có thể hết bộ nhớ)";
        case SIGXFSZ: return "SIGXFSZ - output vượt giới hạn dung lượng";
        default: return "tín hiệu " + std::to_string(sig);
    }
}

RunResult runProcess(const std::vector<std::string>& argv, const fs::path& inFile, const fs::path& outFile,
                     const fs::path& errFile, const fs::path& workDir, const RunLimits& limits) {
    RunResult res;
    int ep[2];
    if (pipe(ep) != 0) {
        res.message = "pipe() thất bại.";
        return res;
    }
    fcntl(ep[0], F_SETFD, FD_CLOEXEC);
    fcntl(ep[1], F_SETFD, FD_CLOEXEC);

    std::vector<std::string> args = argv;
    std::vector<char*> cargv;
    for (auto& a : args) cargv.push_back(&a[0]);
    cargv.push_back(nullptr);
    const std::string inS = inFile.empty() ? "/dev/null" : inFile.string();
    const std::string outS = outFile.string(), errS = errFile.string(), wdS = workDir.string();

    pid_t pid = fork();
    if (pid < 0) {
        close(ep[0]);
        close(ep[1]);
        res.message = "fork() thất bại.";
        return res;
    }
    if (pid == 0) {
        setpgid(0, 0);
        auto fail = [&](int stage) {
            int pack[2] = {stage, errno};
            ssize_t ignored = write(ep[1], pack, sizeof pack);
            (void)ignored;
            _exit(127);
        };
        int fi = open(inS.c_str(), O_RDONLY);
        if (fi < 0) fail(1);
        int fo = open(outS.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fo < 0) fail(2);
        dup2(fi, 0);
        dup2(fo, 1);
        if (errS == outS) dup2(fo, 2);
        else {
            int fe = open(errS.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fe < 0) fail(3);
            dup2(fe, 2);
        }
        if (!wdS.empty() && chdir(wdS.c_str()) != 0) fail(4);
        struct rlimit rl;
        rl.rlim_cur = rl.rlim_max = static_cast<rlim_t>(limits.maxOutputBytes + 4096);
        setrlimit(RLIMIT_FSIZE, &rl);
        if (limits.memoryBytes > 0) {
            rl.rlim_cur = rl.rlim_max = static_cast<rlim_t>(limits.memoryBytes);
            setrlimit(RLIMIT_AS, &rl);
        }
        if (getrlimit(RLIMIT_STACK, &rl) == 0) {  // tăng stack lên 256 MB nếu được phép
            rlim_t want = 256ULL << 20;
            if (rl.rlim_max == RLIM_INFINITY || rl.rlim_max >= want) rl.rlim_cur = want;
            else rl.rlim_cur = rl.rlim_max;
            setrlimit(RLIMIT_STACK, &rl);
        }
        signal(SIGPIPE, SIG_DFL);
        execvp(cargv[0], cargv.data());
        fail(5);
    }
    setpgid(pid, pid);
    close(ep[1]);
    int pack[2] = {0, 0};
    ssize_t got = read(ep[0], pack, sizeof pack);
    close(ep[0]);
    if (got == static_cast<ssize_t>(sizeof pack)) {
        int st;
        waitpid(pid, &st, 0);
        res.status = RunStatus::StartFailed;
        const char* what[] = {"", "mở file input", "tạo file output", "tạo file stderr", "chuyển thư mục làm việc",
                              "khởi chạy chương trình"};
        if (pack[0] == 5 && pack[1] == ENOENT)
            res.message = "Không tìm thấy chương trình '" + argv[0] + "' (kiểm tra PATH).";
        else
            res.message = std::string("Không thể ") + what[pack[0] >= 1 && pack[0] <= 5 ? pack[0] : 0] + ": " +
                          std::strerror(pack[1]);
        return res;
    }

    auto start = Clock::now();
    int status = 0;
    res.status = RunStatus::Ok;
    bool exited = false;
    for (;;) {
        pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid) {
            exited = true;
            break;
        }
        struct stat sb;
        if (stat(outS.c_str(), &sb) == 0 && static_cast<ull>(sb.st_size) > limits.maxOutputBytes) {
            res.status = RunStatus::OutputLimit;
            break;
        }
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
        if (ms > limits.timeoutMs) {
            res.status = RunStatus::Timeout;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    kill(-pid, SIGKILL);  // dọn cả các tiến trình con (nếu có)
    if (!exited) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    }
    res.elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();

    struct stat sb;
    if (res.status == RunStatus::Ok && stat(outS.c_str(), &sb) == 0 &&
        static_cast<ull>(sb.st_size) > limits.maxOutputBytes)
        res.status = RunStatus::OutputLimit;

    if (res.status == RunStatus::Timeout) {
        res.message = "Chạy quá " + std::to_string(limits.timeoutMs) + " ms nên đã bị dừng.";
    } else if (res.status == RunStatus::OutputLimit) {
        res.message = "Output vượt " + fmtBytes(limits.maxOutputBytes) + " nên đã bị dừng.";
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        res.exitCode = 128 + sig;
        if (sig == SIGXFSZ) {
            res.status = RunStatus::OutputLimit;
            res.message = "Output vượt " + fmtBytes(limits.maxOutputBytes) + " nên đã bị dừng.";
        } else {
            res.status = RunStatus::RuntimeError;
            res.message = "Bị dừng bởi " + signalText(sig);
            if (limits.memoryBytes > 0 && (sig == SIGSEGV || sig == SIGABRT || sig == SIGKILL))
                res.message += ". Có thể đã vượt giới hạn bộ nhớ " + fmtBytes(limits.memoryBytes) + ".";
        }
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        res.exitCode = WEXITSTATUS(status);
        res.status = RunStatus::RuntimeError;
        res.message = "Mã thoát " + std::to_string(res.exitCode) + " (khác 0)";
    }
    return res;
}
#endif

// ---------------------------------------------------------------------------

// Tìm trình biên dịch: nếu chỉ là tên (vd "g++") mà không có trong PATH, trên Windows
// thử các thư mục cài đặt MSYS2 mặc định.
static std::string resolveCompiler(const std::string& name, bool verbose) {
#ifdef _WIN32
    if (name.find_first_of("\\/:") != std::string::npos) return name;
    std::wstring w = platform::widen(name);
    wchar_t buf[MAX_PATH];
    if (SearchPathW(nullptr, w.c_str(), L".exe", MAX_PATH, buf, nullptr) > 0) return name;
    const char* dirs[] = {"C:\\msys64\\ucrt64\\bin", "C:\\msys64\\mingw64\\bin", "C:\\msys64\\clang64\\bin",
                          "D:\\msys64\\ucrt64\\bin", "D:\\msys64\\mingw64\\bin"};
    std::string exe = name;
    if (toLower(exe).size() < 4 || toLower(exe).substr(exe.size() - 4) != ".exe") exe += ".exe";
    for (const char* d : dirs) {
        std::error_code ec;
        fs::path cand = u8path(std::string(d) + "\\" + exe);
        if (fs::exists(cand, ec)) {
            if (verbose) con::println("  (Không thấy '" + name + "' trong PATH, dùng " + pathStr(cand) + ")");
            return pathStr(cand);
        }
    }
#else
    (void)verbose;
#endif
    return name;
}

static bool isSourceExt(const std::string& ext) {
    return ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c++" || ext == ".cp";
}

static std::string firstLines(const std::string& text, size_t maxLines) {
    std::istringstream is(text);
    std::string line, out;
    size_t n = 0;
    while (std::getline(is, line)) {
        if (n == maxLines) {
            out += "  ... (còn nữa)\n";
            break;
        }
        out += "  " + line + "\n";
        ++n;
    }
    return out;
}

PreparedSolution prepareSolution(const std::string& path, const Settings& st, const fs::path& workDir,
                                 bool forceRebuild, bool verbose) {
    PreparedSolution ps;
    std::error_code ec;
    fs::path src = u8path(path);
    if (path.empty()) {
        ps.error = "Chưa nhập đường dẫn file lời giải.";
        return ps;
    }
    if (!fs::exists(src, ec)) {
        ps.error = "Không tìm thấy file lời giải: " + path;
        return ps;
    }
    if (!fs::is_regular_file(src, ec)) {
        ps.error = "Đường dẫn không phải là file: " + path;
        return ps;
    }
    src = fs::absolute(src, ec);
    std::string ext = toLower(src.extension().u8string());

    if (!isSourceExt(ext)) {
#ifdef _WIN32
        bool runnable = (ext == ".exe");
#else
        bool runnable = (access(src.string().c_str(), X_OK) == 0) && ext != ".c" && ext != ".h" && ext != ".py";
#endif
        if (!runnable) {
            ps.error = "File lời giải phải là mã nguồn C++ (.cpp, .cc, .cxx) hoặc chương trình đã biên dịch"
#ifdef _WIN32
                       " (.exe)"
#endif
                       ". Nhận được: " + src.filename().u8string();
            return ps;
        }
        ps.ok = true;
        ps.exe = src;
        if (verbose) con::println("Dùng trực tiếp chương trình đã biên dịch: " + pathStr(src));
        return ps;
    }

    std::string code;
    if (!readFile(src, code, &ps.error)) return ps;
    if (verbose && code.find("freopen") != std::string::npos)
        con::println("  ! Cảnh báo: lời giải có gọi freopen(). ojgen cấp input qua stdin và đọc output từ stdout.\n"
                     "    Hãy bọc freopen trong  #ifndef OJGEN ... #endif  (ojgen tự định nghĩa macro OJGEN khi biên dịch).");

    fs::create_directories(workDir, ec);
    const std::string flags = st.effectiveFlags();
    ull h = fnv1a(code);
    h = fnv1a("\x1f" + st.compiler + "\x1f" + flags + "\x1f" + pathStr(src), h);
#ifdef _WIN32
    fs::path exe = workDir / ("sol_" + toHex(h).substr(0, 12) + ".exe");
#else
    fs::path exe = workDir / ("sol_" + toHex(h).substr(0, 12));
#endif
    exe = fs::absolute(exe, ec);
    if (!forceRebuild && fs::exists(exe, ec)) {
        ps.ok = true;
        ps.exe = exe;
        ps.compiled = true;
        ps.cached = true;
        if (verbose) con::println("Mã nguồn không đổi so với lần biên dịch trước: dùng lại " + exe.filename().u8string());
        return ps;
    }

    // Biên dịch trong thư mục làm việc, dùng bản sao mã nguồn có tên ASCII và đường dẫn
    // tương đối: tránh lỗi với đường dẫn có dấu tiếng Việt ở các bản g++ MinGW cũ.
    const std::string tag = toHex(h).substr(0, 12);
    const std::string srcName = "src_" + tag + ext, exeName = exe.filename().u8string();
    if (!writeFile(workDir / srcName, code, &ps.error)) return ps;
    std::string compiler = resolveCompiler(st.compiler, verbose);
    if (compiler.find_first_of("/\\") != std::string::npos && fs::exists(u8path(compiler), ec))
        compiler = pathStr(fs::absolute(u8path(compiler), ec));
    std::vector<std::string> args = {compiler, srcName};
    for (auto& f : splitArgs(flags)) args.push_back(f);
    args.push_back("-DOJGEN");
    // Cho phép #include "file.h" nằm cạnh mã nguồn gốc.
    if (code.find("#include \"") != std::string::npos) args.push_back("-I" + pathStr(src.parent_path()));
    args.push_back("-o");
    args.push_back(exeName);

    if (verbose) {
        std::string shown;
        for (size_t k = 0; k < args.size(); ++k) {
            std::string a = k == 1 ? pathStr(src) : args[k];
            shown += (a.find(' ') != std::string::npos ? "\"" + a + "\"" : a) + " ";
        }
        con::println("Đang biên dịch: " + shown);
    }
    fs::path log = workDir / "compile.log";
    RunLimits lim;
    lim.timeoutMs = st.compileTimeoutSec * 1000;
    lim.maxOutputBytes = 32ULL << 20;
    fs::remove(exe, ec);
    RunResult rr = runProcess(args, fs::path(), log, log, fs::absolute(workDir, ec), lim);
    fs::remove(workDir / srcName, ec);
    std::string logText;
    readFile(log, logText);
    // Đổi tên bản sao trong thông báo lỗi về lại tên file gốc của người dùng.
    for (size_t pos = 0; (pos = logText.find(srcName, pos)) != std::string::npos;) {
        logText.replace(pos, srcName.size(), pathStr(src));
        pos += pathStr(src).size();
    }
    if (rr.status != RunStatus::Ok || !fs::exists(exe, ec)) {
        ps.error = "BIÊN DỊCH THẤT BẠI";
        if (rr.status == RunStatus::StartFailed) ps.error += ": " + rr.message;
        else if (rr.status == RunStatus::Timeout) ps.error += ": quá " + std::to_string(st.compileTimeoutSec) + " giây.";
        if (!logText.empty()) ps.error += "\nThông báo của trình biên dịch:\n" + firstLines(logText, 40);
        fs::remove(exe, ec);
        return ps;
    }
    if (verbose) {
        con::println("Biên dịch thành công (" + std::to_string(rr.elapsedMs) + " ms).");
        if (!trim(logText).empty()) con::println("Trình biên dịch có cảnh báo:\n" + firstLines(logText, 10));
    }
    ps.ok = true;
    ps.exe = exe;
    ps.compiled = true;
    return ps;
}

OutputReport produceOutputs(const fs::path& exe, const std::vector<fs::path>& inputs, const Settings& st,
                            const fs::path& workDir, bool verbose) {
    OutputReport rep;
    rep.total = static_cast<int>(inputs.size());
    std::error_code ec;
    fs::create_directories(workDir, ec);
    const fs::path tmpOut = fs::absolute(workDir / "run_stdout.tmp", ec);
    const fs::path tmpErr = fs::absolute(workDir / "run_stderr.tmp", ec);
    RunLimits lim;
    lim.timeoutMs = st.timeoutMs;
    lim.maxOutputBytes = static_cast<ull>(st.maxOutputMB) << 20;
    lim.memoryBytes = static_cast<ull>(st.memoryMB) << 20;
    const int width = static_cast<int>(std::to_string(inputs.size()).size());

    std::set<ull> outHashes;
    int emptyOutputs = 0;
    for (size_t i = 0; i < inputs.size(); ++i) {
        const fs::path& in = inputs[i];
        fs::path finalOut = in;
        finalOut.replace_extension(".out");
        fs::remove(finalOut, ec);  // không để lại .out cũ không khớp với input
        const std::string name = in.stem().u8string();
        std::string idx = std::to_string(i + 1);
        idx = std::string(static_cast<size_t>(width) - idx.size(), ' ') + idx;
        if (verbose) con::print("  [" + idx + "/" + std::to_string(inputs.size()) + "] " + name + " ... ");

        RunResult rr = runProcess({pathStr(exe)}, fs::absolute(in, ec), tmpOut, tmpErr, workDir, lim);
        rep.maxMs = std::max(rep.maxMs, rr.elapsedMs);
        std::string reason;
        if (rr.status == RunStatus::Ok) {
            std::string data, err;
            if (!readFile(tmpOut, data, &err)) reason = err;
            else {
                if (st.normalizeEol && data.find('\r') != std::string::npos) {
                    std::string t;
                    t.reserve(data.size());
                    for (size_t k = 0; k < data.size(); ++k)
                        if (!(data[k] == '\r' && k + 1 < data.size() && data[k + 1] == '\n')) t += data[k];
                    data.swap(t);
                }
                if (!writeFile(finalOut, data, &err)) reason = err;
                else {
                    ++rep.ok;
                    outHashes.insert(fnv1a(data));
                    if (data.empty()) ++emptyOutputs;
                    if (verbose) con::println("OK (" + std::to_string(rr.elapsedMs) + " ms)");
                }
            }
        } else {
            reason = statusName(rr.status) + ": " + rr.message;
            std::string errText;
            if (readFile(tmpErr, errText) && !trim(errText).empty())
                reason += "\n      stderr: " + trim(errText.substr(0, 300));
        }
        if (!reason.empty()) {
            rep.failures.push_back({static_cast<int>(i + 1), name, reason});
            if (verbose) con::println("LỖI\n      " + reason);
        }
    }
    fs::remove(tmpOut, ec);
    fs::remove(tmpErr, ec);
    if (rep.ok > 1 && outHashes.size() == 1 && inputs.size() > 1)
        rep.warnings.push_back("Tất cả " + std::to_string(rep.ok) +
                               " file output giống hệt nhau. Hãy kiểm tra lời giải có đọc input từ stdin không.");
    if (emptyOutputs > 0)
        rep.warnings.push_back(std::to_string(emptyOutputs) + " test có output rỗng (lời giải không in gì).");
    return rep;
}

}  // namespace ojgen
