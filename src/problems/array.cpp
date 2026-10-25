// array.cpp - Module: mảng một chiều.
//
// Định dạng input:
//   n
//   a1 a2 ... an
#include <algorithm>
#include <climits>
#include <unordered_set>

#include "problem.h"
#include "problem_list.h"

namespace ojgen {

// Sinh k giá trị đôi một khác nhau trong [lo, hi]. Yêu cầu hi - lo + 1 >= k.
std::vector<ll> distinctValues(Rng& rng, ll lo, ll hi, size_t k) {
    std::vector<ll> res;
    res.reserve(k);
    ull span = spanOf(lo, hi);
    if (span != ~0ULL && span + 1 <= 4ULL * k + 16) {
        // Miền nhỏ: xáo trộn một phần (Fisher-Yates) trên toàn bộ miền.
        std::vector<ll> all(static_cast<size_t>(span + 1));
        for (size_t i = 0; i < all.size(); ++i) all[i] = static_cast<ll>(static_cast<ull>(lo) + i);
        for (size_t i = 0; i < k; ++i) {
            size_t j = i + static_cast<size_t>(rng.below(all.size() - i));
            std::swap(all[i], all[j]);
            res.push_back(all[i]);
        }
        return res;
    }
    std::unordered_set<ll> seen;
    seen.reserve(k * 2);
    while (res.size() < k) {
        ll v = rng.range(lo, hi);
        if (seen.insert(v).second) res.push_back(v);
    }
    return res;
}

namespace {

class ArrayProblem : public Problem {
public:
    std::string id() const override { return "array"; }
    std::string title() const override { return "Mảng một chiều"; }
    std::string formatDescription() const override {
        return "Dòng 1: số nguyên n (minN <= n <= maxN).\n"
               "Dòng 2: n số nguyên a1 ... an (minVal <= ai <= maxVal), cách nhau một dấu cách.\n"
               "  distinct = 1: các phần tử đôi một khác nhau.\n"
               "  sorted   = 0: ngẫu nhiên, 1: không giảm, 2: không tăng.\n"
               "Ví dụ:\n  5\n  3 -1 4 1 5\n";
    }
    std::vector<ParamSpec> params() const override {
        return {
            {"minN", "Số phần tử nhỏ nhất (minN)", ParamType::Int, 1, 1, 1000000, "", ""},
            {"maxN", "Số phần tử lớn nhất (maxN)", ParamType::Int, 10, 1, 1000000, "", ""},
            {"minVal", "Giá trị phần tử nhỏ nhất", ParamType::Int, 1, LLONG_MIN, LLONG_MAX, "", ""},
            {"maxVal", "Giá trị phần tử lớn nhất", ParamType::Int, 100, LLONG_MIN, LLONG_MAX, "", ""},
            {"distinct", "Các phần tử đôi một khác nhau?", ParamType::Bool, 0, 0, 1, "", ""},
            {"sorted", "Sắp xếp (0 = ngẫu nhiên, 1 = tăng dần, 2 = giảm dần)", ParamType::Int, 0, 0, 2, "", ""},
        };
    }
    Error validateConfig(const Config& c) const override {
        if (c.i("minN") > c.i("maxN")) return "minN phải <= maxN.";
        if (c.i("minVal") > c.i("maxVal")) return "minVal phải <= maxVal.";
        if (c.b("distinct")) {
            ull span = spanOf(c.i("minVal"), c.i("maxVal"));
            if (span != ~0ULL && span + 1 < static_cast<ull>(c.i("minN")))
                return "Miền [minVal, maxVal] chỉ có " + std::to_string(span + 1) +
                       " giá trị, không đủ cho " + std::to_string(c.i("minN")) + " phần tử khác nhau.";
        }
        return "";
    }
    std::string generate(Rng& rng, const Config& c) const override {
        ll maxN = c.i("maxN");
        if (c.b("distinct")) {
            ull span = spanOf(c.i("minVal"), c.i("maxVal"));
            if (span != ~0ULL && static_cast<ull>(maxN) > span + 1) maxN = static_cast<ll>(span + 1);
        }
        ll n = rng.range(c.i("minN"), maxN);
        std::vector<ll> a;
        if (c.b("distinct")) {
            a = distinctValues(rng, c.i("minVal"), c.i("maxVal"), static_cast<size_t>(n));
        } else {
            a.resize(static_cast<size_t>(n));
            for (auto& x : a) x = rng.range(c.i("minVal"), c.i("maxVal"));
        }
        if (c.i("sorted") == 1) std::sort(a.begin(), a.end());
        if (c.i("sorted") == 2) std::sort(a.rbegin(), a.rend());
        std::string s;
        s.reserve(static_cast<size_t>(n) * (maxDigits(c.i("minVal"), c.i("maxVal")) + 1) + 24);
        appendLL(s, n);
        s += '\n';
        for (size_t i = 0; i < a.size(); ++i) {
            if (i) s += ' ';
            appendLL(s, a[i]);
        }
        s += '\n';
        return s;
    }
    Error validateInput(const std::string& in, const Config& c) const override {
        std::vector<std::vector<std::string>> L;
        if (auto e = splitInputLines(in, L); !e.empty()) return e;
        if (L.size() != 2) return "Phải có đúng 2 dòng (n và mảng).";
        if (L[0].size() != 1) return "Dòng 1 phải chứa đúng 1 số n.";
        ll n;
        if (auto e = readIntToken(L[0][0], c.i("minN"), c.i("maxN"), "n", n); !e.empty()) return e;
        if (static_cast<ll>(L[1].size()) != n)
            return "Dòng 2 có " + std::to_string(L[1].size()) + " số, cần " + std::to_string(n) + ".";
        std::vector<ll> a(static_cast<size_t>(n));
        for (size_t i = 0; i < a.size(); ++i)
            if (auto e = readIntToken(L[1][i], c.i("minVal"), c.i("maxVal"), "a[" + std::to_string(i + 1) + "]", a[i]);
                !e.empty())
                return e;
        if (c.i("sorted") == 1 && !std::is_sorted(a.begin(), a.end())) return "Mảng không tăng dần.";
        if (c.i("sorted") == 2 && !std::is_sorted(a.rbegin(), a.rend())) return "Mảng không giảm dần.";
        if (c.b("distinct")) {
            std::vector<ll> b = a;
            std::sort(b.begin(), b.end());
            if (std::adjacent_find(b.begin(), b.end()) != b.end()) return "Mảng có phần tử trùng nhau.";
        }
        return "";
    }
    ull estimateMaxBytes(const Config& c) const override {
        return static_cast<ull>(c.i("maxN")) * (maxDigits(c.i("minVal"), c.i("maxVal")) + 1) + 10;
    }
};

}  // namespace

std::unique_ptr<Problem> makeArray() { return std::make_unique<ArrayProblem>(); }

}  // namespace ojgen
