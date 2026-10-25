// single_n.cpp - Module: bài có MỘT số nguyên n.
//
// Định dạng input:
//   n
#include <climits>

#include "problem.h"
#include "problem_list.h"

namespace ojgen {
namespace {

class SingleN : public Problem {
public:
    std::string id() const override { return "single_n"; }
    std::string title() const override { return "Một số nguyên n"; }
    std::string formatDescription() const override {
        return "Một dòng duy nhất chứa số nguyên n (minVal <= n <= maxVal).\n"
               "Ví dụ:\n  123456\n";
    }
    std::vector<ParamSpec> params() const override {
        return {
            {"minVal", "Giá trị nhỏ nhất của n", ParamType::Int, 1, LLONG_MIN, LLONG_MAX, "", ""},
            {"maxVal", "Giá trị lớn nhất của n", ParamType::Int, 1000000000, LLONG_MIN, LLONG_MAX, "", ""},
        };
    }
    Error validateConfig(const Config& c) const override {
        if (c.i("minVal") > c.i("maxVal")) return "minVal phải <= maxVal.";
        return "";
    }
    std::string generate(Rng& rng, const Config& c) const override {
        std::string s;
        appendLL(s, rng.range(c.i("minVal"), c.i("maxVal")));
        s += '\n';
        return s;
    }
    Error validateInput(const std::string& in, const Config& c) const override {
        std::vector<std::vector<std::string>> L;
        if (auto e = splitInputLines(in, L); !e.empty()) return e;
        if (L.size() != 1 || L[0].size() != 1) return "Phải có đúng 1 dòng chứa 1 số nguyên.";
        ll v;
        return readIntToken(L[0][0], c.i("minVal"), c.i("maxVal"), "n", v);
    }
    ull estimateMaxBytes(const Config& c) const override { return maxDigits(c.i("minVal"), c.i("maxVal")) + 1; }
};

}  // namespace

std::unique_ptr<Problem> makeSingleN() { return std::make_unique<SingleN>(); }

}  // namespace ojgen
