// queries.cpp - Module: mảng kết hợp nhiều truy vấn.
//
// Định dạng input:
//   n q
//   a1 a2 ... an
//   q dòng, mỗi dòng là một truy vấn:
//     1 i x   -> cập nhật: gán a[i] = x        (1 <= i <= n, minVal <= x <= maxVal)
//     2 l r   -> hỏi trên đoạn [l, r]           (1 <= l <= r <= n)
#include <climits>

#include "problem.h"
#include "problem_list.h"

namespace ojgen {
namespace {

class QueriesProblem : public Problem {
public:
    std::string id() const override { return "queries"; }
    std::string title() const override { return "Mảng và truy vấn"; }
    std::string formatDescription() const override {
        return "Dòng 1: n q.\n"
               "Dòng 2: n số nguyên a1 ... an (minVal <= ai <= maxVal).\n"
               "q dòng tiếp theo, mỗi dòng một truy vấn:\n"
               "  1 i x : gán a[i] = x   (1 <= i <= n, minVal <= x <= maxVal)\n"
               "  2 l r : truy vấn đoạn [l, r]   (1 <= l <= r <= n)\n"
               "Tỷ lệ truy vấn loại 1 do updatePct quy định (0..100). Nếu updatePct < 100,\n"
               "mỗi test có ít nhất một truy vấn loại 2.\n"
               "Ví dụ:\n  5 3\n  1 2 3 4 5\n  2 1 3\n  1 2 10\n  2 2 5\n";
    }
    std::vector<ParamSpec> params() const override {
        return {
            {"minN", "Số phần tử nhỏ nhất (minN)", ParamType::Int, 1, 1, 1000000, "", ""},
            {"maxN", "Số phần tử lớn nhất (maxN)", ParamType::Int, 10, 1, 1000000, "", ""},
            {"minVal", "Giá trị phần tử nhỏ nhất", ParamType::Int, 1, LLONG_MIN, LLONG_MAX, "", ""},
            {"maxVal", "Giá trị phần tử lớn nhất", ParamType::Int, 100, LLONG_MIN, LLONG_MAX, "", ""},
            {"minQ", "Số truy vấn nhỏ nhất (minQ)", ParamType::Int, 1, 1, 1000000, "", ""},
            {"maxQ", "Số truy vấn lớn nhất (maxQ)", ParamType::Int, 10, 1, 1000000, "", ""},
            {"updatePct", "Tỷ lệ % truy vấn cập nhật (loại 1)", ParamType::Int, 30, 0, 100, "",
             "0 = chỉ có truy vấn hỏi đoạn, 100 = chỉ có cập nhật"},
        };
    }
    Error validateConfig(const Config& c) const override {
        if (c.i("minN") > c.i("maxN")) return "minN phải <= maxN.";
        if (c.i("minVal") > c.i("maxVal")) return "minVal phải <= maxVal.";
        if (c.i("minQ") > c.i("maxQ")) return "minQ phải <= maxQ.";
        return "";
    }
    std::string generate(Rng& rng, const Config& c) const override {
        ll n = rng.range(c.i("minN"), c.i("maxN"));
        ll q = rng.range(c.i("minQ"), c.i("maxQ"));
        ll lo = c.i("minVal"), hi = c.i("maxVal");
        int pct = static_cast<int>(c.i("updatePct"));
        std::string s;
        s.reserve(static_cast<size_t>(n) * (maxDigits(lo, hi) + 1) + static_cast<size_t>(q) * 30 + 32);
        appendLL(s, n);
        s += ' ';
        appendLL(s, q);
        s += '\n';
        for (ll i = 0; i < n; ++i) {
            if (i) s += ' ';
            appendLL(s, rng.range(lo, hi));
        }
        s += '\n';
        bool hasRangeQuery = false;
        for (ll k = 0; k < q; ++k) {
            bool update = rng.chance(pct);
            // Bảo đảm có ít nhất một truy vấn loại 2 khi updatePct < 100.
            if (k == q - 1 && !hasRangeQuery && pct < 100) update = false;
            if (update) {
                s += "1 ";
                appendLL(s, rng.range(1, n));
                s += ' ';
                appendLL(s, rng.range(lo, hi));
            } else {
                hasRangeQuery = true;
                ll l = rng.range(1, n), r = rng.range(1, n);
                if (l > r) std::swap(l, r);
                s += "2 ";
                appendLL(s, l);
                s += ' ';
                appendLL(s, r);
            }
            s += '\n';
        }
        return s;
    }
    Error validateInput(const std::string& in, const Config& c) const override {
        std::vector<std::vector<std::string>> L;
        if (auto e = splitInputLines(in, L); !e.empty()) return e;
        if (L.size() < 2 || L[0].size() != 2) return "Dòng 1 phải chứa n q.";
        ll n, q;
        if (auto e = readIntToken(L[0][0], c.i("minN"), c.i("maxN"), "n", n); !e.empty()) return e;
        if (auto e = readIntToken(L[0][1], c.i("minQ"), c.i("maxQ"), "q", q); !e.empty()) return e;
        if (static_cast<ll>(L.size()) != 2 + q)
            return "Số dòng là " + std::to_string(L.size()) + ", cần " + std::to_string(2 + q) + ".";
        if (static_cast<ll>(L[1].size()) != n) return "Dòng 2 phải có đúng n số.";
        ll v;
        for (size_t i = 0; i < L[1].size(); ++i)
            if (auto e = readIntToken(L[1][i], c.i("minVal"), c.i("maxVal"), "a[" + std::to_string(i + 1) + "]", v);
                !e.empty())
                return e;
        bool hasRange = false;
        for (ll k = 0; k < q; ++k) {
            const auto& t = L[static_cast<size_t>(2 + k)];
            std::string where = "Truy vấn " + std::to_string(k + 1);
            if (t.size() != 3) return where + ": phải có đúng 3 số.";
            ll type, x, y;
            if (auto e = readIntToken(t[0], 1, 2, where + " (loại)", type); !e.empty()) return e;
            if (type == 1) {
                if (auto e = readIntToken(t[1], 1, n, where + " (i)", x); !e.empty()) return e;
                if (auto e = readIntToken(t[2], c.i("minVal"), c.i("maxVal"), where + " (x)", y); !e.empty()) return e;
            } else {
                hasRange = true;
                if (auto e = readIntToken(t[1], 1, n, where + " (l)", x); !e.empty()) return e;
                if (auto e = readIntToken(t[2], 1, n, where + " (r)", y); !e.empty()) return e;
                if (x > y) return where + ": l > r.";
            }
        }
        if (c.i("updatePct") < 100 && !hasRange) return "Không có truy vấn loại 2 nào.";
        if (c.i("updatePct") == 0) {
            for (ll k = 0; k < q; ++k)
                if (L[static_cast<size_t>(2 + k)][0] == "1") return "updatePct = 0 nhưng có truy vấn loại 1.";
        }
        if (c.i("updatePct") == 100 && hasRange) return "updatePct = 100 nhưng có truy vấn loại 2.";
        return "";
    }
    ull estimateMaxBytes(const Config& c) const override {
        ull d = maxDigits(c.i("minVal"), c.i("maxVal"));
        ull dn = std::to_string(c.i("maxN")).size();
        return static_cast<ull>(c.i("maxN")) * (d + 1) + static_cast<ull>(c.i("maxQ")) * (4 + dn + (d > dn ? d : dn)) + 32;
    }
};

}  // namespace

std::unique_ptr<Problem> makeQueries() { return std::make_unique<QueriesProblem>(); }

}  // namespace ojgen
