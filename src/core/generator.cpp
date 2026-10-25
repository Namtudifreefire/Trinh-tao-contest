// generator.cpp - Sinh file input cho một module.
#include "generator.h"

#include <chrono>
#include <random>
#include <unordered_map>

#include "console.h"

namespace ojgen {

Error validateFullConfig(const Problem& p, const Config& cfg) {
    if (auto e = checkParamRanges(p, cfg); !e.empty()) return e;
    return p.validateConfig(cfg);
}

ull randomSeed() {
    std::random_device rd;
    ull a = (static_cast<ull>(rd()) << 32) ^ rd();
    ull t = static_cast<ull>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    ull s = a ^ (t * 0x9E3779B97F4A7C15ULL);
    return s % 1000000000000ULL;  // seed ngắn (tối đa 12 chữ số) cho dễ ghi lại
}

GenerateResult generateInputs(const GenerateRequest& req) {
    GenerateResult res;
    const Problem& p = *req.problem;
    if (req.testCount < 1 || req.testCount > kMaxTests) {
        res.error = "Số lượng test phải trong [1, " + std::to_string(kMaxTests) + "].";
        return res;
    }
    if (auto e = validateFullConfig(p, req.cfg); !e.empty()) {
        res.error = "Cấu hình không hợp lệ: " + e;
        return res;
    }
    std::error_code ec;
    fs::create_directories(req.outDir, ec);
    if (ec || !fs::is_directory(req.outDir, ec)) {
        res.error = "Không tạo được thư mục " + pathStr(req.outDir);
        return res;
    }

    Rng rng(req.seed);
    const int width = testNumberWidth(req.testCount);
    const bool live = req.verbose && con::stdoutIsConsole();
    std::unordered_map<ull, int> seen;  // hash nội dung -> số thứ tự test
    int duplicates = 0;
    std::string dupList;
    for (int t = 1; t <= req.testCount; ++t) {
        std::string data;
        int sameAs = 0;
        for (int attempt = 0; attempt < 50; ++attempt) {
            data = p.generate(rng, req.cfg);
            auto it = seen.find(fnv1a(data));
            if (it == seen.end()) {
                sameAs = 0;
                break;
            }
            sameAs = it->second;
        }
        if (auto e = p.validateInput(data, req.cfg); !e.empty()) {
            // Không bao giờ xảy ra nếu module đúng; nếu có là lỗi của module.
            res.error = "Test " + std::to_string(t) + " sinh ra không đúng định dạng/giới hạn (lỗi module '" +
                        p.id() + "'): " + e;
            return res;
        }
        if (sameAs) {
            ++duplicates;
            if (duplicates <= 10)
                dupList += " " + testName(req.prefix, t, width) + "=" + testName(req.prefix, sameAs, width);
        } else {
            seen.emplace(fnv1a(data), t);
        }
        fs::path f = req.outDir / (testName(req.prefix, t, width) + ".inp");
        std::string err;
        if (!writeFile(f, data, &err)) {
            res.error = err;
            return res;
        }
        res.inputs.push_back(f);
        if (live && (t == req.testCount || t % 10 == 0 || data.size() > (1u << 20)))
            con::print("\r  Đã sinh " + std::to_string(t) + "/" + std::to_string(req.testCount) + " file input");
    }
    if (live) con::println("");
    if (duplicates > 0)
        res.warnings.push_back(std::to_string(duplicates) +
                               " test trùng với test trước đó vì miền dữ liệu quá nhỏ so với số test (" +
                               trim(dupList) + (duplicates > 10 ? " ..." : "") + "). Hãy nới giới hạn hoặc giảm số test.");
    res.ok = true;
    return res;
}

}  // namespace ojgen
