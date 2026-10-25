// two_ints.cpp - Module: bài có HAI số nguyên a, b.
//
// Định dạng input:
//   a b
#include <algorithm>
#include <climits>

#include "problem.h"
#include "problem_list.h"

namespace ojgen {
namespace {

class TwoInts : public Problem {
public:
    std::string id() const override { return "two_ints"; }
    std::string title() const override { return "Hai số nguyên a, b"; }
    std::string formatDescription() const override {
        return "Một dòng chứa hai số nguyên a b, cách nhau một dấu cách.\n"
               "  minA <= a <= maxA, minB <= b <= maxB; nếu order = 1 thì luôn có a <= b.\n"
               "Ví dụ:\n  15 -7\n";
    }
    std::vector<ParamSpec> params() const override {
        return {
            {"minA", "Giá trị nhỏ nhất của a", ParamType::Int, 1, LLONG_MIN, LLONG_MAX, "", ""},
            {"maxA", "Giá trị lớn nhất của a", ParamType::Int, 1000000000, LLONG_MIN, LLONG_MAX, "", ""},
            {"minB", "Giá trị nhỏ nhất của b", ParamType::Int, 1, LLONG_MIN, LLONG_MAX, "", ""},
            {"maxB", "Giá trị lớn nhất của b", ParamType::Int, 1000000000, LLONG_MIN, LLONG_MAX, "", ""},
            {"order", "Bắt buộc a <= b?", ParamType::Bool, 0, 0, 1, "", ""},
        };
    }
    Error validateConfig(const Config& c) const override {
        if (c.i("minA") > c.i("maxA")) return "minA phải <= maxA.";
        if (c.i("minB") > c.i("maxB")) return "minB phải <= maxB.";
        if (c.b("order") && c.i("minA") > c.i("maxB"))
            return "Không thể có a <= b vì minA > maxB.";
        return "";
    }
    std::string generate(Rng& rng, const Config& c) const override {
        ll a, b;
        if (c.b("order")) {
            a = rng.range(c.i("minA"), std::min(c.i("maxA"), c.i("maxB")));
            b = rng.range(std::max(c.i("minB"), a), c.i("maxB"));
        } else {
            a = rng.range(c.i("minA"), c.i("maxA"));
            b = rng.range(c.i("minB"), c.i("maxB"));
        }
        std::string s;
        appendLL(s, a);
        s += ' ';
        appendLL(s, b);
        s += '\n';
        return s;
    }
    Error validateInput(const std::string& in, const Config& c) const override {
        std::vector<std::vector<std::string>> L;
        if (auto e = splitInputLines(in, L); !e.empty()) return e;
        if (L.size() != 1 || L[0].size() != 2) return "Phải có đúng 1 dòng chứa 2 số nguyên.";
        ll a, b;
        if (auto e = readIntToken(L[0][0], c.i("minA"), c.i("maxA"), "a", a); !e.empty()) return e;
        if (auto e = readIntToken(L[0][1], c.i("minB"), c.i("maxB"), "b", b); !e.empty()) return e;
        if (c.b("order") && a > b) return "Vi phạm điều kiện a <= b.";
        return "";
    }
    ull estimateMaxBytes(const Config& c) const override {
        return maxDigits(c.i("minA"), c.i("maxA")) + maxDigits(c.i("minB"), c.i("maxB")) + 2;
    }
};

}  // namespace

std::unique_ptr<Problem> makeTwoInts() { return std::make_unique<TwoInts>(); }

}  // namespace ojgen
