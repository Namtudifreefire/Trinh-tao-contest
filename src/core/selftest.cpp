// selftest.cpp - Tự kiểm tra: mọi module sinh dữ liệu hợp lệ, tái tạo được bằng seed,
// bộ xác thực bắt được input sai, cấu hình sai bị từ chối, ZIP nén/giải nén đúng.
// Chạy: ojgen --selftest
#include <climits>

#include "cli.h"
#include "console.h"
#include "generator.h"
#include "problem.h"
#include "settings.h"
#include "zip_manager.h"

namespace ojgen {

namespace {

struct Case {
    std::string problem;
    std::vector<std::pair<std::string, ll>> ints;
    std::vector<std::pair<std::string, std::string>> texts;
    int tests;
    std::string note;
};

int gPass = 0, gFail = 0;

void check(bool ok, const std::string& what, const std::string& detail = "") {
    if (ok) ++gPass;
    else {
        ++gFail;
        con::println("  [FAIL] " + what + (detail.empty() ? "" : ": " + detail));
    }
}

Config makeConfig(const Problem& p, const Case& c) {
    Config cfg = p.defaultConfig();
    for (auto& kv : c.ints) cfg.set(kv.first, kv.second);
    for (auto& kv : c.texts) cfg.setText(kv.first, kv.second);
    return cfg;
}

}  // namespace

int runSelfTest() {
    con::println("ojgen --selftest: kiểm tra module, xác thực, seed và ZIP");
    const ll MN = LLONG_MIN, MX = LLONG_MAX;
    std::vector<Case> valid = {
        {"single_n", {}, {}, 50, "mặc định"},
        {"single_n", {{"minVal", MN}, {"maxVal", MX}}, {}, 200, "toàn bộ phạm vi long long"},
        {"single_n", {{"minVal", -5}, {"maxVal", -5}}, {}, 3, "miền một giá trị"},
        {"two_ints", {}, {}, 50, "mặc định"},
        {"two_ints", {{"minA", -10}, {"maxA", 10}, {"minB", 0}, {"maxB", 5}, {"order", 1}}, {}, 100, "a <= b"},
        {"array", {}, {}, 50, "mặc định"},
        {"array", {{"minN", 50}, {"maxN", 50}, {"minVal", 1}, {"maxVal", 50}, {"distinct", 1}}, {}, 20, "hoán vị 1..50"},
        {"array", {{"minN", 1}, {"maxN", 300}, {"minVal", -1000000000000LL}, {"maxVal", -1}, {"sorted", 1}}, {}, 20, "âm, tăng dần"},
        {"array", {{"minN", 5}, {"maxN", 100}, {"minVal", MN}, {"maxVal", MX}, {"distinct", 1}, {"sorted", 2}}, {}, 20, "khác nhau, giảm dần"},
        {"array", {{"minN", 200000}, {"maxN", 200000}, {"minVal", 1}, {"maxVal", 1000000000}}, {}, 2, "n lớn"},
        {"queries", {}, {}, 50, "mặc định"},
        {"queries", {{"updatePct", 0}, {"minN", 1}, {"maxN", 1}}, {}, 20, "n = 1, chỉ hỏi đoạn"},
        {"queries", {{"updatePct", 100}, {"minQ", 5}, {"maxQ", 20}}, {}, 20, "chỉ cập nhật"},
        {"string", {}, {}, 50, "mặc định"},
        {"string", {{"printLen", 1}}, {{"charset", "digits"}}, 30, "chữ số, ghi độ dài"},
        {"string", {{"minLen", 1}, {"maxLen", 3}}, {{"charset", "abc"}}, 30, "tập ký tự tự liệt kê"},
        {"string", {{"minLen", 100000}, {"maxLen", 100000}}, {{"charset", "binary"}}, 3, "xâu dài"},
        {"custom", {}, {}, 100, "mặc định (100 test x 10 số)"},
        {"custom", {{"n", 20}, {"min", -1000}, {"max", 1000}}, {}, 100, "100 test x 20 số, có số âm"},
        {"custom", {{"n", 3}, {"mode", 2}}, {{"bounds", "1:10,50:100,-20:20"}}, 50, "giới hạn riêng từng vị trí"},
        {"custom", {{"n", 2}, {"mode", 2}, {"header", 1}, {"layout", 2}},
         {{"bounds", "-9223372036854775808:9223372036854775807,-9:-1"}}, 30, "giới hạn cực trị, mỗi số một dòng"},
        {"graph", {}, {}, 50, "mặc định"},
        {"graph", {{"minN", 10}, {"maxN", 10}, {"minM", 45}, {"maxM", 45}, {"connected", 1}}, {}, 5, "đồ thị đầy đủ K10"},
        {"graph", {{"minN", 6}, {"maxN", 6}, {"minM", 30}, {"maxM", 30}, {"directed", 1}}, {}, 5, "có hướng đầy đủ"},
        {"graph", {{"minN", 1}, {"maxN", 3}, {"minM", 0}, {"maxM", 20}, {"simple", 0}, {"weighted", 1}, {"minW", -5}, {"maxW", 5}},
         {}, 40, "đa đồ thị có khuyên"},
        {"graph", {{"minN", 1}, {"maxN", 1}, {"minM", 0}, {"maxM", 0}}, {}, 1, "một đỉnh"},
        {"graph", {{"minN", 2}, {"maxN", 50}, {"minM", 0}, {"maxM", 60}, {"connected", 1}, {"directed", 1}}, {}, 40, "có hướng liên thông yếu"},
        {"graph", {{"minN", 200000}, {"maxN", 200000}, {"minM", 199999}, {"maxM", 300000}, {"connected", 1}, {"weighted", 1}},
         {}, 1, "đồ thị lớn"},
    };
    for (const auto& c : valid) {
        const Problem* p = findProblem(c.problem);
        check(p != nullptr, "tìm module " + c.problem);
        if (!p) continue;
        const std::string tag = c.problem + " (" + c.note + ")";
        Config cfg = makeConfig(*p, c);
        Error e = validateFullConfig(*p, cfg);
        check(e.empty(), tag + ": cấu hình hợp lệ", e);
        if (!e.empty()) continue;
        Rng r1(20241025), r2(20241025);
        bool allValid = true, same = true;
        std::string firstErr, first;
        for (int t = 0; t < c.tests; ++t) {
            std::string a = p->generate(r1, cfg), b = p->generate(r2, cfg);
            if (t == 0) first = a;
            if (a != b) same = false;
            Error ve = p->validateInput(a, cfg);
            if (!ve.empty() && allValid) allValid = false, firstErr = ve;
        }
        check(allValid, tag + ": " + std::to_string(c.tests) + " test đều hợp lệ", firstErr);
        check(same, tag + ": cùng seed cho cùng dữ liệu");
        // Bộ xác thực phải bắt được input bị hỏng.
        check(!p->validateInput(first.substr(0, first.size() - 1), cfg).empty(), tag + ": phát hiện thiếu xuống dòng cuối");
        check(!p->validateInput(first + "\n", cfg).empty(), tag + ": phát hiện dòng trống thừa");
        std::string spaced = first;
        spaced.insert(spaced.size() - 1, " ");
        check(!p->validateInput(spaced, cfg).empty(), tag + ": phát hiện khoảng trắng thừa");
    }
    con::println("  Đã kiểm tra " + std::to_string(valid.size()) + " cấu hình hợp lệ.");

    // Kiểm tra giới hạn: giá trị ngoài miền phải bị bộ xác thực từ chối.
    {
        const Problem* p = findProblem("custom");
        Case c{"custom", {{"n", 3}, {"mode", 2}}, {{"bounds", "1:10,50:100,-20:20"}}, 0, ""};
        Config cfg = makeConfig(*p, c);
        check(p->validateInput("10 100 -20\n", cfg).empty(), "custom: biên đúng được chấp nhận");
        check(!p->validateInput("11 100 -20\n", cfg).empty(), "custom: số thứ 1 vượt biên bị từ chối");
        check(!p->validateInput("1 49 0\n", cfg).empty(), "custom: số thứ 2 dưới biên bị từ chối");
        check(!p->validateInput("1 50 21\n", cfg).empty(), "custom: số thứ 3 vượt biên bị từ chối");
        check(!p->validateInput("1 50\n", cfg).empty(), "custom: thiếu số bị từ chối");
        const Problem* g = findProblem("graph");
        Case gc{"graph", {{"minN", 3}, {"maxN", 3}, {"minM", 2}, {"maxM", 3}, {"connected", 1}}, {}, 0, ""};
        Config gcfg = makeConfig(*g, gc);
        check(g->validateInput("3 2\n1 2\n2 3\n", gcfg).empty(), "graph: đồ thị đúng được chấp nhận");
        check(!g->validateInput("3 2\n1 2\n2 1\n", gcfg).empty(), "graph: cạnh lặp bị từ chối");
        check(!g->validateInput("3 2\n1 1\n2 3\n", gcfg).empty(), "graph: khuyên bị từ chối");
        check(!g->validateInput("3 2\n1 2\n2 4\n", gcfg).empty(), "graph: đỉnh ngoài phạm vi bị từ chối");
        gcfg.set("minM", 1);
        check(!g->validateInput("3 1\n1 2\n", gcfg).empty(), "graph: không liên thông bị từ chối");
    }

    std::vector<Case> invalid = {
        {"single_n", {{"minVal", 5}, {"maxVal", 4}}, {}, 0, "min > max"},
        {"array", {{"minN", 10}, {"maxN", 10}, {"minVal", 1}, {"maxVal", 5}, {"distinct", 1}}, {}, 0, "không đủ giá trị khác nhau"},
        {"array", {{"minN", 0}}, {}, 0, "minN = 0 ngoài miền"},
        {"queries", {{"minQ", 5}, {"maxQ", 4}}, {}, 0, "minQ > maxQ"},
        {"string", {}, {{"charset", "a b"}}, 0, "charset có dấu cách"},
        {"custom", {{"n", 2}, {"mode", 2}}, {{"bounds", "1:2,3:4,5:6"}}, 0, "bounds khác n"},
        {"custom", {{"n", 1}, {"mode", 2}}, {{"bounds", "5:1"}}, 0, "bounds lo > hi"},
        {"custom", {{"n", 1}, {"mode", 2}}, {{"bounds", "1:99999999999999999999"}}, 0, "bounds vượt long long"},
        {"custom", {{"min", 10}, {"max", 1}}, {}, 0, "min > max"},
        {"graph", {{"minN", 4}, {"maxN", 4}, {"minM", 7}, {"maxM", 7}}, {}, 0, "quá số cạnh đồ thị đơn"},
        {"graph", {{"minN", 10}, {"maxN", 10}, {"minM", 3}, {"maxM", 3}, {"connected", 1}}, {}, 0, "không đủ cạnh để liên thông"},
        {"two_ints", {{"minA", 10}, {"maxA", 20}, {"minB", 1}, {"maxB", 5}, {"order", 1}}, {}, 0, "không thể a <= b"},
    };
    for (const auto& c : invalid) {
        const Problem* p = findProblem(c.problem);
        Config cfg = makeConfig(*p, c);
        check(!validateFullConfig(*p, cfg).empty(), c.problem + ": từ chối cấu hình sai (" + c.note + ")");
    }
    con::println("  Đã kiểm tra " + std::to_string(invalid.size()) + " cấu hình sai (phải bị từ chối).");

    // Deflate/Inflate và ZIP.
    {
        Rng r(7);
        std::vector<std::string> samples = {"", "a", "abcabcabcabcabcabcabc", std::string(100000, 'x')};
        std::string rnd, txt;
        for (int i = 0; i < 300000; ++i) rnd += static_cast<char>(r.below(256));
        for (int i = 0; i < 50000; ++i) appendLL(txt, r.range(-1000000, 1000000)), txt += (i % 20 == 19 ? '\n' : ' ');
        samples.push_back(rnd);
        samples.push_back(txt);
        for (size_t i = 0; i < samples.size(); ++i) {
            std::string comp = deflateRaw(samples[i]), back;
            bool ok = inflateRaw(comp, samples[i].size(), back) && back == samples[i];
            check(ok, "deflate/inflate mẫu " + std::to_string(i + 1) + " (" + fmtBytes(samples[i].size()) + ")");
        }
        check(crc32("123456789") == 0xCBF43926u, "CRC-32 chuẩn");
        Settings st = Settings::load();
        fs::path dir = workDirFor(st) / "selftest";
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::create_directories(dir, ec);
        std::vector<ZipEntry> entries;
        std::vector<std::string> names;
        for (size_t i = 0; i < samples.size(); ++i) {
            std::string nm = "tệp_" + std::to_string(i + 1) + ".inp";
            writeFile(dir / u8path(nm), samples[i]);
            entries.push_back({nm, dir / u8path(nm)});
            names.push_back(nm);
        }
        for (bool compress : {true, false}) {
            fs::path z = dir / (compress ? "nen.zip" : "luu.zip");
            std::string err;
            bool ok = createZip(z, entries, compress, nullptr, err) && verifyZip(z, names, err);
            check(ok, std::string("tạo + kiểm tra ZIP ") + (compress ? "(Deflate)" : "(stored)"), err);
            std::vector<std::string> wrong = names;
            wrong.pop_back();
            check(!verifyZip(z, wrong, err), "verifyZip phát hiện thiếu/thừa file");
        }
        fs::remove_all(dir, ec);
    }

    con::println("");
    con::println("Kết quả: " + std::to_string(gPass) + " kiểm tra đạt, " + std::to_string(gFail) + " thất bại.");
    con::println(gFail == 0 ? "SELFTEST: ĐẠT" : "SELFTEST: KHÔNG ĐẠT");
    return gFail == 0 ? 0 : 1;
}

}  // namespace ojgen
