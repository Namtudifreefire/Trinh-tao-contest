// menu.cpp - Menu tương tác.
#include <algorithm>
#include <map>
#include <optional>

#include "cli.h"
#include "console.h"
#include "generator.h"
#include "problem.h"
#include "runner.h"
#include "settings.h"

namespace ojgen {

namespace {

// Cấu hình dùng gần nhất cho từng dạng bài trong phiên làm việc (làm giá trị mặc định).
std::map<std::string, Config> gLastConfig;
std::map<std::string, int> gLastTests;

bool gClear = true;  // theo Settings::clearScreen

// Xóa màn hình rồi in tiêu đề màn hình mới.
void newScreen() {
    if (gClear) con::clearScreen();
    else con::println("");
}

void printHeader(const std::string& title) {
    con::println("");
    con::println("==================================================================");
    con::println("  " + title);
    con::println("==================================================================");
}

std::string askSolution(const std::string& def) {
    for (;;) {
        std::string path = con::askPath("Đường dẫn file lời giải (.cpp/.exe; nhập * để mở hộp thoại chọn file)", def);
        if (path.empty()) {
            con::println("  ! Chưa nhập đường dẫn.");
        } else {
            std::error_code ec;
            fs::path p = u8path(path);
            if (fs::is_regular_file(p, ec)) return pathStr(fs::absolute(p, ec));
            con::println("  ! Không tìm thấy file: " + path);
        }
        if (!con::askYesNo("Nhập lại đường dẫn? (chọn n = chỉ tạo input, không tạo output)", true)) return "";
    }
}

// Chọn một thư mục test có sẵn trong output/ hoặc nhập đường dẫn.
bool pickTestFolder(const Settings& st, fs::path& out) {
    std::vector<fs::path> dirs;
    std::error_code ec;
    fs::path root = u8path(st.outputRoot);
    if (fs::is_directory(root, ec))
        for (const auto& e : fs::directory_iterator(root, ec))
            if (e.is_directory(ec) && e.path().filename() != ".work" && !listFilesWithExt(e.path(), ".inp").empty())
                dirs.push_back(e.path());
    std::sort(dirs.begin(), dirs.end());
    con::println("Các thư mục test trong " + st.outputRoot + ":");
    for (size_t i = 0; i < dirs.size(); ++i)
        con::println("  " + std::to_string(i + 1) + ". " + dirs[i].filename().u8string() + "  (" +
                     std::to_string(listFilesWithExt(dirs[i], ".inp").size()) + " .inp, " +
                     std::to_string(listFilesWithExt(dirs[i], ".out").size()) + " .out)");
    if (dirs.empty()) con::println("  (chưa có)");
    std::string s = con::askPath("Nhập số thứ tự hoặc đường dẫn thư mục (Enter để hủy)", "");
    if (s.empty()) return false;
    ll k;
    if (parseLL(s, k) && k >= 1 && k <= static_cast<ll>(dirs.size())) {
        out = dirs[static_cast<size_t>(k - 1)];
        return true;
    }
    out = u8path(s);
    if (!fs::is_directory(out, ec)) {
        con::println("  ! Không tìm thấy thư mục: " + s);
        return false;
    }
    return true;
}

void generateFlow(const Problem& p, Settings& st) {
    const int cfgSteps = p.interactiveSteps();
    const int total = 1 + cfgSteps + 3;
    auto step = [&](int k, const std::string& t) { return "Bước " + std::to_string(k) + "/" + std::to_string(total) + ": " + t; };

    printHeader(p.title() + "  [" + p.id() + "]");
    con::println("Định dạng input:");
    for (const auto& line : split(p.formatDescription(), '\n'))
        if (!line.empty()) con::println("  " + line);
    con::println("(Nhấn Enter để giữ giá trị mặc định trong [ ].)");

    con::println("");
    con::println(step(1, "Số lượng test case"));
    int lastTests = gLastTests.count(p.id()) ? gLastTests[p.id()] : 10;
    int tests = static_cast<int>(con::askInt("Bạn muốn tạo bao nhiêu test case?", lastTests, 1, kMaxTests));

    Config cfg = gLastConfig.count(p.id()) ? gLastConfig[p.id()] : p.defaultConfig();
    for (;;) {
        if (cfgSteps == 1) {
            con::println("");
            con::println(step(2, "Cấu hình dữ liệu"));
        }
        p.configureInteractive(cfg);
        Error e = validateFullConfig(p, cfg);
        if (e.empty()) break;
        con::println("  ! Cấu hình không hợp lệ: " + e);
        if (!con::askYesNo("Nhập lại cấu hình?", true)) return;
    }
    gLastConfig[p.id()] = cfg;
    gLastTests[p.id()] = tests;

    con::println("");
    con::println(step(2 + cfgSteps, "Lời giải chuẩn"));
    std::string solution;
    const std::string defSol = st.solutionFor(p.id());
    if (!defSol.empty()) con::println("  (Lời giải dùng lần trước cho dạng bài này: " + defSol + ")");
    if (con::askYesNo("Bạn có muốn tạo file output bằng lời giải chuẩn?", true)) solution = askSolution(defSol);
    if (solution.empty()) con::println("  -> Chỉ tạo file .inp; file .out sẽ KHÔNG được tạo.");

    con::println("");
    con::println(step(3 + cfgSteps, "Sinh test (seed và thư mục lưu)"));
    std::optional<ull> seed;
    for (;;) {
        std::string s = con::ask("Seed (Enter = ngẫu nhiên; nhập lại seed cũ để tái tạo bộ test)", "");
        if (s.empty()) break;
        ull v;
        if (parseULL(s, v)) {
            seed = v;
            break;
        }
        con::println("  ! Seed phải là số nguyên không âm.");
    }
    std::string name = con::ask("Tên thư mục chứa test (trong " + st.outputRoot + ")", p.id());
    fs::path dir = u8path(name);
    if (!dir.is_absolute()) dir = u8path(st.outputRoot) / dir;
    std::string err;
    if (!prepareOutDir(dir, ExistingPolicy::Ask, err)) {
        con::println(err);
        return;
    }

    JobSpec job;
    job.problem = &p;
    job.cfg = cfg;
    job.testCount = tests;
    job.seed = seed;
    job.solutionPath = solution;
    job.outDir = dir;
    job.zip = ZipMode::Ask;
    job.exportStepTitle = step(4 + cfgSteps, "Xuất file");
    runJob(job, st);
}

void manualRun(Settings& st) {
    if (st.solution.empty()) {
        con::println("Chưa đặt lời giải mặc định (mục 2).");
        return;
    }
    PreparedSolution ps = prepareSolution(st.solution, st, workDirFor(st), false, true);
    if (!ps.ok) {
        con::println(ps.error);
        return;
    }
    con::println("Nhập dữ liệu input, kết thúc bằng một dòng chỉ có dấu chấm (.):");
    std::string data, line;
    for (;;) {
        if (!con::readLine(line)) throw con::InputClosed();
        if (trim(line) == ".") break;
        data += line + "\n";
    }
    fs::path wd = workDirFor(st);
    std::error_code ec;
    fs::create_directories(wd, ec);
    fs::path in = fs::absolute(wd / "manual.inp", ec), out = fs::absolute(wd / "manual.out", ec),
             er = fs::absolute(wd / "manual.err", ec);
    writeFile(in, data);
    RunLimits lim;
    lim.timeoutMs = st.timeoutMs;
    lim.maxOutputBytes = static_cast<ull>(st.maxOutputMB) << 20;
    lim.memoryBytes = static_cast<ull>(st.memoryMB) << 20;
    RunResult rr = runProcess({pathStr(ps.exe)}, in, out, er, wd, lim);
    std::string o, e;
    readFile(out, o);
    readFile(er, e);
    con::println("--- Kết quả: " + statusName(rr.status) + (rr.message.empty() ? "" : " - " + rr.message) + " (" +
                 std::to_string(rr.elapsedMs) + " ms) ---");
    con::print(o.size() > 4000 ? o.substr(0, 4000) + "\n... (đã cắt bớt)\n" : o);
    if (!trim(e).empty()) con::println("--- stderr ---\n" + e.substr(0, 2000));
    for (const auto& f : {in, out, er}) fs::remove(f, ec);
}

void solutionMenu(Settings& st) {
    static const char* kTitles[] = {"",
                                    "ĐẶT / ĐỔI LỜI GIẢI MẶC ĐỊNH",
                                    "BIÊN DỊCH THỬ LỜI GIẢI MẶC ĐỊNH",
                                    "CHẠY THỬ LỜI GIẢI VỚI INPUT NHẬP TAY",
                                    "TẠO LẠI FILE .OUT CHO THƯ MỤC TEST CÓ SẴN",
                                    "ĐÓNG GÓI ZIP THƯ MỤC TEST CÓ SẴN",
                                    "ĐỔI TRÌNH BIÊN DỊCH / CỜ BIÊN DỊCH",
                                    "ĐỔI GIỚI HẠN CHẠY",
                                    "TÙY CHỌN ZIP, XUỐNG DÒNG VÀ MÀN HÌNH"};
    for (;;) {
        newScreen();
        printHeader("QUẢN LÝ LỜI GIẢI CHUẨN VÀ CẤU HÌNH CHẠY");
        con::print(st.describe());
        con::println("");
        con::println("  1. Đặt / đổi lời giải mặc định");
        con::println("  2. Biên dịch thử lời giải mặc định");
        con::println("  3. Chạy thử lời giải với input nhập tay");
        con::println("  4. Tạo lại file .out cho một thư mục test có sẵn");
        con::println("  5. Đóng gói ZIP một thư mục test có sẵn");
        con::println("  6. Đổi trình biên dịch / cờ biên dịch");
        con::println("  7. Đổi giới hạn chạy (timeout, dung lượng output, bộ nhớ)");
        con::println("  8. Tùy chọn: nén ZIP, chuẩn hóa CRLF -> LF, xóa màn hình");
        con::println("  0. Quay lại menu chính");
        ll c = con::askInt("Chọn", 0, 0, 8);
        if (c != 0) {
            newScreen();
            printHeader(kTitles[c]);
        }
        std::string err;
        switch (c) {
            case 0: return;
            case 1: {
                std::string s = askSolution(st.solution);
                if (!s.empty()) {
                    st.solution = s;
                    con::println(st.save(&err) ? "Đã lưu lời giải mặc định." : "LỖI: " + err);
                }
                break;
            }
            case 2: {
                if (st.solution.empty()) {
                    con::println("Chưa đặt lời giải mặc định.");
                    break;
                }
                PreparedSolution ps = prepareSolution(st.solution, st, workDirFor(st), true, true);
                if (!ps.ok) con::println(ps.error);
                break;
            }
            case 3: manualRun(st); break;
            case 4: {
                fs::path dir;
                if (!pickTestFolder(st, dir)) break;
                std::string s = askSolution(st.solution);
                if (s.empty()) break;
                regenerateOutputs(dir, s, st, ZipMode::Ask, false, false);
                break;
            }
            case 5: {
                fs::path dir;
                if (pickTestFolder(st, dir)) zipTestFolder(dir, st, true);
                break;
            }
            case 6: {
                st.compiler = con::ask("Trình biên dịch (vd g++ hoặc C:\\msys64\\ucrt64\\bin\\g++.exe)", st.compiler);
                con::println("Cờ mặc định: " + Settings::defaultFlags());
                std::string f = con::ask("Cờ biên dịch (nhập 'mac dinh' để dùng mặc định)", st.effectiveFlags());
                st.cxxflags = (toLower(f) == "mac dinh" || toLower(f) == "mặc định" || f == Settings::defaultFlags()) ? "" : f;
                con::println(st.save(&err) ? "Đã lưu." : "LỖI: " + err);
                break;
            }
            case 7: {
                st.timeoutMs = static_cast<int>(con::askInt("Timeout mỗi test (ms)", st.timeoutMs, 1, 3600000));
                st.maxOutputMB = con::askInt("Dung lượng output tối đa mỗi test (MB)", st.maxOutputMB, 1, 4096);
                st.memoryMB = con::askInt("Giới hạn bộ nhớ (MB, 0 = không giới hạn)", st.memoryMB, 0, 1048576);
                con::println(st.save(&err) ? "Đã lưu." : "LỖI: " + err);
                break;
            }
            case 8: {
                st.zipCompress = con::askYesNo("Nén file ZIP (Deflate)?", st.zipCompress);
                st.normalizeEol = con::askYesNo("Chuyển CRLF -> LF trong file .out?", st.normalizeEol);
                st.clearScreen = con::askYesNo("Xóa màn hình khi chuyển giữa các mục menu?", st.clearScreen);
                gClear = st.clearScreen;
                con::println(st.save(&err) ? "Đã lưu." : "LỖI: " + err);
                break;
            }
        }
        con::pause();
    }
}

const char* kMenuIds[] = {"", "single_n", "two_ints", "array", "queries", "string", "custom", "graph"};

}  // namespace

std::string guideText() {
    std::string s;
    s += "HƯỚNG DẪN SỬ DỤNG OJGEN\n\n";
    s += "1) Quy trình: chọn dạng bài -> nhập số test và giới hạn -> (tùy chọn) chọn lời giải chuẩn\n";
    s += "   -> ojgen sinh file .inp, biên dịch và chạy lời giải để tạo .out -> đóng gói ZIP.\n";
    s += "   Kết quả nằm trong thư mục output/<tên>/ : test01.inp, test01.out, ... và output/<tên>.zip\n\n";
    s += "2) Lời giải chuẩn phải đọc từ stdin (cin/scanf) và ghi ra stdout (cout/printf).\n";
    s += "   Nếu code có freopen(\"bai.inp\", ...), hãy bọc lại:  #ifndef OJGEN  freopen(...);  #endif\n";
    s += "   (ojgen tự thêm -DOJGEN khi biên dịch). Test nào lỗi (RE/TLE/OLE) sẽ không có .out.\n\n";
    s += "3) Seed: mỗi lần sinh, ojgen in ra seed. Dùng lại cùng seed + cùng cấu hình sẽ tạo lại đúng bộ test.\n\n";
    s += "4) Mỗi file input được kiểm tra lại (định dạng, giới hạn, ràng buộc như đồ thị đơn/liên thông).\n";
    s += "   ojgen cố gắng để các test không trùng nhau; nếu miền dữ liệu quá nhỏ sẽ có cảnh báo.\n\n";
    s += "5) An toàn: lời giải chạy trực tiếp trên máy với quyền của bạn. ojgen chỉ giới hạn thời gian,\n";
    s += "   dung lượng output và (tùy chọn) bộ nhớ - đây KHÔNG phải môi trường cách ly. Chỉ chạy code tin cậy.\n\n";
    s += "6) Dùng dòng lệnh không cần menu: ojgen --help\n\n";
    s += "CÁC DẠNG BÀI\n";
    for (const auto& p : allProblems()) {
        s += "\n[" + p->id() + "] " + p->title() + "\n";
        for (const auto& line : split(p->formatDescription(), '\n'))
            if (!line.empty()) s += "  " + line + "\n";
    }
    return s;
}

int runMenu() {
    std::string warn;
    Settings st = Settings::load(&warn);
    gClear = st.clearScreen;
    try {
        for (;;) {
            newScreen();
            if (!warn.empty()) {  // cảnh báo đọc cấu hình: hiện ở màn hình đầu tiên
                con::println(warn);
                warn.clear();
            }
            printHeader("OJGEN " + std::string(kVersion) + " - SINH TEST CHO BÀI LẬP TRÌNH THI ĐẤU");
            con::println("  1. Sinh test: một số nguyên n");
            con::println("  2. Sinh test: hai số nguyên");
            con::println("  3. Sinh test: mảng một chiều");
            con::println("  4. Sinh test: mảng và truy vấn");
            con::println("  5. Sinh test: xâu ký tự");
            con::println("  6. Sinh test tùy chỉnh: nhiều số nguyên, giới hạn chung hoặc riêng từng vị trí");
            con::println("  7. Sinh test: đồ thị");
            con::println("  8. Quản lý lời giải chuẩn và cấu hình chạy");
            con::println("  9. Hướng dẫn sử dụng");
            con::println("  0. Thoát");
            ll c = con::askInt("Chọn chức năng", 6, 0, 9);
            if (c == 0) break;
            if (c != 8) newScreen();  // mục 8 tự xóa màn hình khi mở menu con
            if (c >= 1 && c <= 7) {
                const Problem* p = findProblem(kMenuIds[c]);
                generateFlow(*p, st);
                st = Settings::load();  // nạp lại (lời giải vừa dùng được ghi nhớ)
                con::pause();
            } else if (c == 8) {
                solutionMenu(st);
            } else if (c == 9) {
                con::print(guideText());
                con::pause();
            }
        }
    } catch (const con::InputClosed&) {
        con::println("");
        con::println("(Hết dữ liệu nhập - thoát.)");
        return 0;
    }
    con::println("Tạm biệt!");
    return 0;
}

}  // namespace ojgen
