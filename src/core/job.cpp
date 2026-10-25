// job.cpp - Điều phối quy trình: sinh input -> chạy lời giải -> ghi thông tin -> ZIP.
#include <ctime>
#include <sstream>

#include "console.h"
#include "generator.h"
#include "runner.h"
#include "zip_manager.h"

namespace ojgen {

fs::path workDirFor(const Settings& st) { return u8path(st.outputRoot) / ".work"; }

static std::string nowStr() {
    std::time_t t = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tmv);
    return buf;
}

// Ghi nhớ lời giải gần nhất vào file cấu hình (không lưu các tùy chọn CLI tạm thời).
static void rememberSolution(const std::string& path, const std::string& problemId) {
    Settings disk = Settings::load();
    if (disk.solution == path && (problemId.empty() || disk.solutionFor(problemId) == path)) return;
    disk.solution = path;
    if (!problemId.empty()) disk.solutionByProblem[problemId] = path;
    disk.save();
}

enum class SetStatus { Complete, InputOnly, Incomplete };

static std::string statusLine(SetStatus s) {
    switch (s) {
        case SetStatus::Complete: return "HOÀN CHỈNH (đủ .inp và .out)";
        case SetStatus::InputOnly: return "CHỈ CÓ INPUT (chưa tạo .out vì không dùng lời giải chuẩn)";
        case SetStatus::Incomplete: return "CHƯA HOÀN CHỈNH (lời giải chuẩn lỗi, thiếu .out)";
    }
    return "";
}

static void printFailures(const OutputReport& rep) {
    con::println("  Các test bị lỗi (" + std::to_string(rep.failures.size()) + "/" + std::to_string(rep.total) +
                 ") - KHÔNG có file .out cho các test này:");
    size_t show = rep.failures.size() > 30 ? 30 : rep.failures.size();
    for (size_t i = 0; i < show; ++i)
        con::println("    - " + rep.failures[i].name + ": " + rep.failures[i].reason);
    if (rep.failures.size() > show) con::println("    ... và " + std::to_string(rep.failures.size() - show) + " test khác.");
}

static std::string failureSummary(const OutputReport& rep) {
    std::string s;
    for (const auto& f : rep.failures) s += "    - " + f.name + ": " + split(f.reason, '\n')[0] + "\n";
    return s;
}

static bool decideZip(ZipMode mode, SetStatus status, bool zipIncomplete) {
    if (mode == ZipMode::No) return false;
    if (status == SetStatus::Incomplete) {
        if (mode == ZipMode::Ask)
            return con::askYesNo("Bộ test CHƯA hoàn chỉnh. Vẫn tạo ZIP chỉ gồm các file đã có?", false);
        if (!zipIncomplete) {
            con::println("Không tạo ZIP vì bộ test chưa hoàn chỉnh (dùng --zip-incomplete nếu vẫn muốn nén).");
            return false;
        }
        return true;
    }
    if (mode == ZipMode::Ask) {
        std::string q = status == SetStatus::InputOnly ? "Tạo file ZIP (chỉ chứa các file .inp)?" : "Tạo file ZIP chứa toàn bộ test?";
        return con::askYesNo(q, true);
    }
    return true;
}

bool zipTestFolder(const fs::path& dir, const Settings& st, bool verbose) {
    auto inputs = listFilesWithExt(dir, ".inp");
    if (inputs.empty()) {
        con::println("Thư mục " + pathStr(dir) + " không có file .inp nào để nén.");
        return false;
    }
    std::vector<ZipEntry> entries;
    std::vector<std::string> names, missing;
    size_t outs = 0;
    for (const auto& in : inputs) {
        entries.push_back({in.filename().u8string(), in});
        fs::path o = in;
        o.replace_extension(".out");
        std::error_code ec;
        if (fs::exists(o, ec)) {
            entries.push_back({o.filename().u8string(), o});
            ++outs;
        } else {
            missing.push_back(in.stem().u8string());
        }
    }
    for (auto& e : entries) names.push_back(e.name);
    fs::path zip = dir;
    zip += ".zip";
    ZipStats zs;
    std::string err;
    if (!createZip(zip, entries, st.zipCompress, &zs, err)) {
        con::println("LỖI tạo ZIP: " + err);
        return false;
    }
    if (!verifyZip(zip, names, err)) {
        con::println("LỖI: file ZIP vừa tạo không vượt qua kiểm tra (" + err + "). Đã xóa file ZIP.");
        std::error_code ec;
        fs::remove(zip, ec);
        return false;
    }
    if (verbose) {
        con::println("Đã tạo ZIP: " + pathStr(zip));
        con::println("  Gồm " + std::to_string(inputs.size()) + " file .inp và " + std::to_string(outs) + " file .out (dữ liệu " +
                     fmtBytes(zs.rawBytes) + ", file ZIP " + fmtBytes(zs.zipBytes) + "). Đã kiểm tra: mở được, đủ file, đúng CRC.");
        if (outs == 0) con::println("  Lưu ý: ZIP CHỈ chứa input, chưa có output.");
        else if (!missing.empty())
            con::println("  Lưu ý: " + std::to_string(missing.size()) + " test thiếu .out (vd " + missing[0] + ").");
    }
    return true;
}

bool prepareOutDir(fs::path& dir, ExistingPolicy policy, std::string& err) {
    std::error_code ec;
    fs::path zip = dir;
    zip += ".zip";
    size_t n = countTestFiles(dir);
    bool zipExists = fs::exists(zip, ec);
    if (n == 0 && !zipExists) return true;
    if (policy == ExistingPolicy::Ask) {
        con::println("");
        con::println("Thư mục " + pathStr(dir) + " đã có " + std::to_string(n) + " file test" +
                     (zipExists ? " (và file " + zip.filename().u8string() + ")" : "") + ".");
        con::println("1. Ghi đè (xóa các file .inp/.out cũ trong thư mục đó, các file khác giữ nguyên)");
        con::println("2. Tạo thư mục mới: " + uniqueDir(dir).filename().u8string());
        con::println("3. Hủy");
        ll c = con::askInt("Lựa chọn", 2, 1, 3);
        if (c == 3) {
            err = "Đã hủy theo yêu cầu.";
            return false;
        }
        policy = c == 1 ? ExistingPolicy::Overwrite : ExistingPolicy::NewDir;
    }
    if (policy == ExistingPolicy::NewDir) {
        dir = uniqueDir(dir);
        con::println("Dùng thư mục mới: " + pathStr(dir));
        return true;
    }
    long removed = clearTestFiles(dir, &err);
    if (removed < 0) return false;
    if (zipExists && !fs::remove(zip, ec)) {
        err = "Không xóa được file ZIP cũ " + pathStr(zip);
        return false;
    }
    con::println("Ghi đè: đã xóa " + std::to_string(removed) + " file test cũ" + (zipExists ? " và file ZIP cũ" : "") + ".");
    return true;
}

int runJob(const JobSpec& job, Settings& st) {
    const Problem& p = *job.problem;
    const ull seed = job.seed ? *job.seed : randomSeed();
    if (auto e = validateFullConfig(p, job.cfg); !e.empty()) {
        con::println("LỖI cấu hình: " + e);
        return 2;
    }
    con::println("");
    con::println("=== Sinh test: " + p.title() + " [" + p.id() + "] ===");
    con::println("  Số test : " + std::to_string(job.testCount));
    con::println("  Seed    : " + std::to_string(seed) + "  (dùng lại seed này + cùng cấu hình để tạo lại đúng bộ test)");
    con::println("  Thư mục : " + pathStr(job.outDir));
    con::print(p.describeConfig(job.cfg));
    ull est = p.estimateMaxBytes(job.cfg);
    if (est > 0) {
        long double total = static_cast<long double>(est) * job.testCount;
        if (total > 1024.0L * 1024 * 1024)
            con::println("  ! Cảnh báo: dung lượng input có thể lên tới ~" +
                         fmtBytes(static_cast<ull>(total)) + ". Kiểm tra dung lượng ổ đĩa.");
    }

    if (!job.genStepTitle.empty()) con::println("\n" + job.genStepTitle);
    GenerateRequest gr;
    gr.problem = &p;
    gr.cfg = job.cfg;
    gr.testCount = job.testCount;
    gr.seed = seed;
    gr.outDir = job.outDir;
    gr.prefix = st.prefix;
    gr.verbose = job.verbose;
    GenerateResult g = generateInputs(gr);
    if (!g.ok) {
        con::println("LỖI: " + g.error);
        return 2;
    }
    con::println("Đã tạo " + std::to_string(g.inputs.size()) + " file .inp (mỗi file đã được kiểm tra đúng định dạng và giới hạn).");
    for (const auto& w : g.warnings) con::println("  ! " + w);

    SetStatus status = SetStatus::InputOnly;
    OutputReport rep;
    std::string compileErr;
    std::string solutionShown = job.solutionPath.empty() ? "(không dùng)" : job.solutionPath;
    if (job.solutionPath.empty()) {
        con::println("Không dùng lời giải chuẩn: chỉ có file .inp, CHƯA tạo file .out.");
    } else {
        rememberSolution(job.solutionPath, p.id());
        con::println("");
        con::println("--- Tạo output bằng lời giải chuẩn ---");
        PreparedSolution ps = prepareSolution(job.solutionPath, st, workDirFor(st), job.forceRebuild, job.verbose);
        if (!ps.ok) {
            compileErr = ps.error;
            con::println(ps.error);
            con::println("Dừng tạo output. Các file .inp vẫn được giữ lại; sửa lời giải rồi dùng menu 8 (hoặc --regen-out) để tạo .out.");
            status = SetStatus::Incomplete;
        } else {
            con::println("Chạy lời giải trên " + std::to_string(g.inputs.size()) + " test (timeout " +
                         std::to_string(st.timeoutMs) + " ms/test):");
            rep = produceOutputs(ps.exe, g.inputs, st, workDirFor(st), job.verbose);
            status = rep.failures.empty() ? SetStatus::Complete : SetStatus::Incomplete;
            for (const auto& w : rep.warnings) con::println("  ! " + w);
            if (!rep.failures.empty()) printFailures(rep);
        }
    }

    std::ostringstream info;
    info << "ojgen " << kVersion << " - thông tin bộ test\n";
    info << "Thời gian tạo : " << nowStr() << "\n";
    info << "Dạng bài      : " << p.id() << " - " << p.title() << "\n";
    info << "Số test       : " << job.testCount << "\n";
    info << "Seed          : " << seed << "\n";
    info << "Lời giải chuẩn: " << solutionShown << "\n";
    info << "Cấu hình:\n" << p.describeConfig(job.cfg);
    info << "Định dạng input:\n" << p.formatDescription();
    info << "Trạng thái    : " << statusLine(status) << "\n";
    if (!compileErr.empty()) info << "  Lỗi biên dịch/chuẩn bị lời giải:\n" << compileErr << "\n";
    if (!rep.failures.empty()) info << "  Test lỗi:\n" << failureSummary(rep);
    for (const auto& w : g.warnings) info << "Cảnh báo: " << w << "\n";
    for (const auto& w : rep.warnings) info << "Cảnh báo: " << w << "\n";
    writeFile(job.outDir / kInfoFile, info.str());

    con::println("");
    if (!job.exportStepTitle.empty()) con::println(job.exportStepTitle);
    con::println("KẾT QUẢ: " + statusLine(status));
    if (status == SetStatus::Complete)
        con::println("  " + std::to_string(rep.ok) + "/" + std::to_string(rep.total) + " test có output, thời gian chạy lâu nhất " +
                     std::to_string(rep.maxMs) + " ms.");
    else if (status == SetStatus::Incomplete && rep.total > 0)
        con::println("  " + std::to_string(rep.ok) + "/" + std::to_string(rep.total) + " test có output.");
    con::println("  Thư mục: " + pathStr(job.outDir) + "  (thông tin chi tiết: " + kInfoFile + ")");

    if (decideZip(job.zip, status, job.zipIncomplete)) zipTestFolder(job.outDir, st, true);
    return status == SetStatus::Incomplete ? 1 : 0;
}

int regenerateOutputs(const fs::path& dir, const std::string& solution, Settings& st, ZipMode zip, bool forceRebuild,
                      bool zipIncomplete) {
    auto inputs = listFilesWithExt(dir, ".inp");
    if (inputs.empty()) {
        con::println("LỖI: thư mục " + pathStr(dir) + " không có file .inp nào.");
        return 2;
    }
    con::println("Tạo lại output cho " + std::to_string(inputs.size()) + " file .inp trong " + pathStr(dir));
    PreparedSolution ps = prepareSolution(solution, st, workDirFor(st), forceRebuild, true);
    SetStatus status;
    OutputReport rep;
    std::string note;
    if (!ps.ok) {
        con::println(ps.error);
        status = SetStatus::Incomplete;
        note = ps.error;
    } else {
        rememberSolution(solution, "");
        rep = produceOutputs(ps.exe, inputs, st, workDirFor(st), true);
        for (const auto& w : rep.warnings) con::println("  ! " + w);
        if (!rep.failures.empty()) printFailures(rep);
        status = rep.failures.empty() ? SetStatus::Complete : SetStatus::Incomplete;
    }
    std::string old;
    readFile(dir / kInfoFile, old);
    std::ostringstream info;
    info << old << "\n--- Tạo lại output lúc " << nowStr() << " ---\n";
    info << "Lời giải chuẩn: " << solution << "\n";
    info << "Trạng thái    : " << statusLine(status) << "\n";
    if (!note.empty()) info << note << "\n";
    if (!rep.failures.empty()) info << "  Test lỗi:\n" << failureSummary(rep);
    writeFile(dir / kInfoFile, info.str());
    con::println("KẾT QUẢ: " + statusLine(status));
    if (decideZip(zip, status, zipIncomplete)) zipTestFolder(dir, st, true);
    return status == SetStatus::Incomplete ? 1 : 0;
}

}  // namespace ojgen
