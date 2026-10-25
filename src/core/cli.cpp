// cli.cpp - Xử lý tham số dòng lệnh.
#include "cli.h"

#include <cctype>
#include <map>
#include <set>

#include "console.h"
#include "generator.h"
#include "problem.h"
#include "settings.h"

namespace ojgen {

std::string usageText() {
    std::string s;
    s += "ojgen " + std::string(kVersion) + " - Công cụ sinh test cho bài lập trình thi đấu\n\n";
    s += "CÁCH DÙNG\n";
    s += "  ojgen                              Mở menu tương tác\n";
    s += "  ojgen --list                       Liệt kê các dạng bài (module)\n";
    s += "  ojgen --info <dạng>                Xem định dạng input và tham số của một dạng bài\n";
    s += "  ojgen --problem <dạng> [tùy chọn]  Sinh bộ test không cần menu\n";
    s += "  ojgen --regen-out <thư mục> --solution <file>   Tạo lại .out cho thư mục test có sẵn\n";
    s += "  ojgen --zip-dir <thư mục>          Đóng gói ZIP thư mục test có sẵn\n";
    s += "  ojgen --selftest                   Tự kiểm tra các module và bộ tạo ZIP\n";
    s += "  ojgen --help | --version\n\n";
    s += "TÙY CHỌN KHI SINH TEST\n";
    s += "  --tests N            Số lượng test (1.." + std::to_string(kMaxTests) + ", mặc định 10)\n";
    s += "  --seed S             Seed ngẫu nhiên (số nguyên không âm). Cùng seed + cùng cấu hình = cùng bộ test\n";
    s += "  --solution FILE      Lời giải chuẩn (.cpp hoặc .exe) để tạo file .out. Bỏ trống = chỉ tạo .inp\n";
    s += "  --out DIR            Thư mục chứa test (mặc định output/<dạng>)\n";
    s += "  --force              Ghi đè test cũ trong thư mục (mặc định: tự tạo thư mục mới <tên>_2, _3...)\n";
    s += "  --no-zip             Không tạo file ZIP (mặc định có tạo khi bộ test hoàn chỉnh hoặc chỉ có input)\n";
    s += "  --zip-incomplete     Vẫn tạo ZIP khi lời giải lỗi ở một số test\n";
    s += "  --rebuild            Biên dịch lại lời giải dù mã nguồn không đổi\n";
    s += "  --timeout MS         Thời gian tối đa mỗi test (ms)\n";
    s += "  --max-output-mb M    Dung lượng output tối đa mỗi test (MB)\n";
    s += "  --memory-mb M        Giới hạn bộ nhớ lời giải (MB, 0 = không giới hạn)\n";
    s += "  --compiler PATH      Trình biên dịch C++ (mặc định g++)\n";
    s += "  --cxxflags \"...\"     Cờ biên dịch\n";
    s += "  --prefix P           Tiền tố tên file test (mặc định test)\n";
    s += "  --no-compress        ZIP chỉ lưu, không nén\n";
    s += "  --<tham số> GIÁ_TRỊ  Tham số riêng của dạng bài, xem bằng --info <dạng>\n\n";
    s += "VÍ DỤ\n";
    s += "  ojgen --list\n";
    s += "  ojgen --problem array --tests 20 --seed 2024\n";
    s += "  ojgen --problem custom --tests 100 --seed 2024\n";
    s += "  ojgen --problem custom --tests 100 --n 20 --min -1000 --max 1000 --solution sol.cpp\n";
    s += "  ojgen --problem custom --tests 50 --bounds \"1:10,50:100,-20:20\" --solution \"D:\\OJ\\bai 1\\sol.cpp\"\n";
    s += "  ojgen --problem graph --tests 30 --minN 5 --maxN 1000 --minM 4 --maxM 5000 --connected 1 --weighted 1\n\n";
    s += "MÃ THOÁT: 0 = thành công (hoàn chỉnh hoặc chỉ input), 1 = lời giải lỗi ở một số test, 2 = lỗi tham số/cấu hình.\n";
    return s;
}

static std::string listText() {
    std::string s = "Các dạng bài có sẵn:\n";
    for (const auto& p : allProblems()) {
        std::string id = p->id();
        s += "  " + id + std::string(id.size() < 10 ? 10 - id.size() : 1, ' ') + p->title() + "\n";
    }
    s += "\nXem chi tiết: ojgen --info <dạng>\n";
    return s;
}

static std::string infoText(const Problem& p) {
    std::string s = p.id() + " - " + p.title() + "\n\nĐỊNH DẠNG INPUT\n";
    for (const auto& line : split(p.formatDescription(), '\n'))
        if (!line.empty()) s += "  " + line + "\n";
    s += "\nTHAM SỐ (--tên giá_trị)\n";
    Config d = p.defaultConfig();
    for (const auto& ps : p.params()) {
        s += "  --" + ps.key + "  : " + ps.label;
        if (ps.type == ParamType::Int)
            s += " [mặc định " + std::to_string(d.i(ps.key)) + ", miền " + std::to_string(ps.lo) + ".." + std::to_string(ps.hi) + "]";
        else if (ps.type == ParamType::Bool)
            s += " [0/1, mặc định " + std::to_string(d.i(ps.key)) + "]";
        else
            s += " [mặc định \"" + d.t(ps.key) + "\"]";
        s += "\n";
        if (!ps.help.empty()) s += "        " + ps.help + "\n";
    }
    return s;
}

namespace {
struct Args {
    std::map<std::string, std::string> kv;
    std::set<std::string> flags;
    std::vector<std::string> order;  // các khóa tham số theo thứ tự xuất hiện
};

const std::set<std::string> kFlagOnly = {"help",  "h",          "list",           "version",  "selftest", "force",
                                         "no-zip", "zip",        "zip-incomplete", "rebuild",  "no-compress"};
}  // namespace

static bool parseArgs(const std::vector<std::string>& in, Args& a, std::string& err) {
    for (size_t i = 0; i < in.size(); ++i) {
        std::string s = in[i];
        if (s.rfind("--", 0) == 0) s = s.substr(2);
        else if (s == "-h" || s == "/?") s = "help";
        else {
            err = "Đối số không hợp lệ: '" + s + "' (các tùy chọn bắt đầu bằng --).";
            return false;
        }
        std::string value;
        bool hasValue = false;
        size_t eq = s.find('=');
        if (eq != std::string::npos) {
            value = s.substr(eq + 1);
            s = s.substr(0, eq);
            hasValue = true;
        }
        if (kFlagOnly.count(s)) {
            if (hasValue) {
                err = "--" + s + " không nhận giá trị.";
                return false;
            }
            a.flags.insert(s);
            continue;
        }
        if (!hasValue) {
            // Giá trị là đối số kế tiếp, kể cả khi bắt đầu bằng '-' (vd --min -20).
            if (i + 1 >= in.size()) {
                err = "Thiếu giá trị cho --" + s + ".";
                return false;
            }
            value = in[++i];
        }
        if (a.kv.count(s)) {
            err = "Tùy chọn --" + s + " bị lặp lại.";
            return false;
        }
        a.kv[s] = value;
        a.order.push_back(s);
    }
    return true;
}

static bool applySettingsOverrides(const Args& a, Settings& st, std::string& err) {
    auto num = [&](const std::string& k, ll lo, ll hi, ll& out) -> bool {
        if (!parseLL(a.kv.at(k), out) || out < lo || out > hi) {
            err = "--" + k + " phải là số nguyên trong [" + std::to_string(lo) + ", " + std::to_string(hi) + "].";
            return false;
        }
        return true;
    };
    ll v;
    if (a.kv.count("timeout")) {
        if (!num("timeout", 1, 3600000, v)) return false;
        st.timeoutMs = static_cast<int>(v);
    }
    if (a.kv.count("max-output-mb")) {
        if (!num("max-output-mb", 1, 4096, v)) return false;
        st.maxOutputMB = v;
    }
    if (a.kv.count("memory-mb")) {
        if (!num("memory-mb", 0, 1048576, v)) return false;
        st.memoryMB = v;
    }
    if (a.kv.count("compiler")) st.compiler = a.kv.at("compiler");
    if (a.kv.count("cxxflags")) st.cxxflags = a.kv.at("cxxflags");
    if (a.kv.count("prefix")) {
        std::string p = a.kv.at("prefix");
        for (char c : p)
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
                err = "--prefix chỉ được chứa chữ, số, '_' và '-'.";
                return false;
            }
        if (p.empty()) {
            err = "--prefix không được rỗng.";
            return false;
        }
        st.prefix = p;
    }
    if (a.flags.count("no-compress")) st.zipCompress = false;
    return true;
}

static const std::set<std::string> kGlobalKeys = {"problem", "tests",     "seed",     "solution", "out",
                                                  "timeout", "max-output-mb", "memory-mb", "compiler", "cxxflags",
                                                  "prefix",  "info",      "regen-out", "zip-dir"};

int runCli(const std::vector<std::string>& argv) {
    Args a;
    std::string err;
    if (!parseArgs(argv, a, err)) {
        con::printErr("LỖI: " + err + "\nXem hướng dẫn: ojgen --help");
        return 2;
    }
    if (a.flags.count("help") || a.flags.count("h")) {
        con::print(usageText());
        return 0;
    }
    if (a.flags.count("version")) {
        con::println(std::string("ojgen ") + kVersion);
        return 0;
    }
    if (a.flags.count("list")) {
        con::print(listText());
        return 0;
    }
    if (a.flags.count("selftest")) return runSelfTest();
    if (a.kv.count("info")) {
        const Problem* p = findProblem(a.kv["info"]);
        if (!p) {
            con::printErr("LỖI: không có dạng bài '" + a.kv["info"] + "'.\n" + listText());
            return 2;
        }
        con::print(infoText(*p));
        return 0;
    }

    std::string warn;
    Settings st = Settings::load(&warn);
    if (!warn.empty()) con::println(warn);
    if (!applySettingsOverrides(a, st, err)) {
        con::printErr("LỖI: " + err);
        return 2;
    }
    ZipMode zipMode = a.flags.count("no-zip") ? ZipMode::No : ZipMode::Yes;

    if (a.kv.count("zip-dir")) {
        fs::path dir = u8path(a.kv["zip-dir"]);
        return zipTestFolder(dir, st, true) ? 0 : 2;
    }
    if (a.kv.count("regen-out")) {
        if (!a.kv.count("solution")) {
            con::printErr("LỖI: --regen-out cần --solution <file lời giải>.");
            return 2;
        }
        return regenerateOutputs(u8path(a.kv["regen-out"]), a.kv["solution"], st, zipMode, a.flags.count("rebuild") > 0,
                                 a.flags.count("zip-incomplete") > 0);
    }
    if (!a.kv.count("problem")) {
        con::printErr("LỖI: thiếu --problem <dạng>. Dùng --list để xem các dạng bài, hoặc chạy không tham số để mở menu.");
        return 2;
    }
    const Problem* p = findProblem(a.kv["problem"]);
    if (!p) {
        con::printErr("LỖI: không có dạng bài '" + a.kv["problem"] + "'.\n" + listText());
        return 2;
    }

    JobSpec job;
    job.problem = p;
    job.cfg = p->defaultConfig();
    std::set<std::string> explicitKeys;
    std::map<std::string, ParamSpec> specs;
    for (const auto& ps : p->params()) specs[ps.key] = ps;
    for (const auto& k : a.order) {
        if (kGlobalKeys.count(k)) continue;
        auto it = specs.find(k);
        if (it == specs.end()) {
            std::string names;
            for (const auto& ps : p->params()) names += " --" + ps.key;
            con::printErr("LỖI: dạng bài '" + p->id() + "' không có tham số --" + k + ".\n  Các tham số hợp lệ:" + names +
                          "\n  Xem chi tiết: ojgen --info " + p->id());
            return 2;
        }
        const ParamSpec& ps = it->second;
        const std::string& v = a.kv[k];
        if (ps.type == ParamType::Text) {
            job.cfg.setText(k, v);
        } else if (ps.type == ParamType::Bool) {
            std::string b = toLower(v);
            if (b == "1" || b == "true" || b == "yes" || b == "y") job.cfg.set(k, 1);
            else if (b == "0" || b == "false" || b == "no" || b == "n") job.cfg.set(k, 0);
            else {
                con::printErr("LỖI: --" + k + " chỉ nhận 0 hoặc 1.");
                return 2;
            }
        } else {
            ll x;
            if (!parseLL(v, x)) {
                con::printErr("LỖI: --" + k + " = '" + v + "' không phải số nguyên hợp lệ (hoặc vượt phạm vi long long).");
                return 2;
            }
            job.cfg.set(k, x);
        }
        explicitKeys.insert(k);
    }
    p->finalizeCliConfig(job.cfg, explicitKeys);
    if (auto e = validateFullConfig(*p, job.cfg); !e.empty()) {
        con::printErr("LỖI cấu hình: " + e);
        return 2;
    }

    ll tests = 10;
    if (a.kv.count("tests") && (!parseLL(a.kv["tests"], tests) || tests < 1 || tests > kMaxTests)) {
        con::printErr("LỖI: --tests phải là số nguyên trong [1, " + std::to_string(kMaxTests) + "].");
        return 2;
    }
    job.testCount = static_cast<int>(tests);
    if (a.kv.count("seed")) {
        ull s;
        if (!parseULL(a.kv["seed"], s)) {
            con::printErr("LỖI: --seed phải là số nguyên không âm.");
            return 2;
        }
        job.seed = s;
    }
    if (a.kv.count("solution")) {
        job.solutionPath = a.kv["solution"];
        if (trim(job.solutionPath).empty()) {
            con::printErr("LỖI: --solution rỗng.");
            return 2;
        }
    }
    job.outDir = a.kv.count("out") ? u8path(a.kv["out"]) : u8path(st.outputRoot) / p->id();
    if (!prepareOutDir(job.outDir, a.flags.count("force") ? ExistingPolicy::Overwrite : ExistingPolicy::NewDir, err)) {
        con::printErr("LỖI: " + err);
        return 2;
    }
    job.zip = zipMode;
    job.zipIncomplete = a.flags.count("zip-incomplete") > 0;
    job.forceRebuild = a.flags.count("rebuild") > 0;
    return runJob(job, st);
}

}  // namespace ojgen
