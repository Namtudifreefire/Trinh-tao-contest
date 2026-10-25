// custom.cpp - Module: dãy n số nguyên với giới hạn do người dùng cấu hình.
//
// Hai chế độ giới hạn:
//   mode = 1 : mọi số nằm trong [min, max] (giới hạn chung)
//   mode = 2 : số thứ i nằm trong [lo_i, hi_i] (giới hạn riêng từng vị trí),
//              lưu trong tham số bounds dạng "lo1:hi1,lo2:hi2,..."
//
// Định dạng input (mặc định header = 0, layout = 1):
//   [n]                (chỉ khi header = 1)
//   x1 x2 ... xn       (layout = 1: cùng một dòng)
//   hoặc mỗi số một dòng (layout = 2)
#include <algorithm>
#include <climits>

#include "console.h"
#include "problem.h"
#include "problem_list.h"

namespace ojgen {
namespace {

constexpr ll kMaxCount = 1000000;

Error parseBounds(const std::string& text, std::vector<std::pair<ll, ll>>& out) {
    out.clear();
    std::string t = trim(text);
    if (t.empty()) return "Chưa có giới hạn riêng (bounds) cho từng vị trí.";
    auto parts = split(t, ',');
    for (size_t i = 0; i < parts.size(); ++i) {
        std::string p = trim(parts[i]);
        // Dấu ':' phân cách lo và hi (lo có thể âm, vd -20:20 hoặc -9:-1).
        size_t colon = p.find(':');
        const std::string where = "bounds vị trí " + std::to_string(i + 1);
        if (colon == std::string::npos) return where + ": cần dạng lo:hi, nhận được '" + p + "'.";
        ll lo, hi;
        if (!parseLL(p.substr(0, colon), lo) || !parseLL(p.substr(colon + 1), hi))
            return where + ": giá trị không hợp lệ hoặc vượt phạm vi long long ('" + p + "').";
        if (lo > hi) return where + ": giá trị nhỏ nhất " + std::to_string(lo) + " > lớn nhất " + std::to_string(hi) + ".";
        out.emplace_back(lo, hi);
    }
    return "";
}

std::string boundsToText(const std::vector<std::pair<ll, ll>>& b) {
    std::string s;
    for (size_t i = 0; i < b.size(); ++i) {
        if (i) s += ',';
        appendLL(s, b[i].first);
        s += ':';
        appendLL(s, b[i].second);
    }
    return s;
}

class CustomProblem : public Problem {
public:
    std::string id() const override { return "custom"; }
    std::string title() const override { return "Tùy chỉnh: dãy số nguyên, giới hạn chung hoặc riêng từng vị trí"; }
    std::string formatDescription() const override {
        return "Mỗi test gồm n số nguyên (n cố định cho mọi test).\n"
               "  mode = 1: mọi số trong [min, max].\n"
               "  mode = 2: số thứ i trong [lo_i, hi_i]; CLI: --bounds \"1:10,50:100,-20:20\".\n"
               "  header = 1: ghi n ở dòng đầu.  layout = 1: các số trên một dòng; layout = 2: mỗi số một dòng.\n"
               "Ví dụ (n = 3, mode = 2, header = 0, layout = 1):\n  7 93 -15\n";
    }
    std::vector<ParamSpec> params() const override {
        return {
            {"n", "Số lượng số nguyên trong mỗi test (n)", ParamType::Int, 10, 1, kMaxCount, "", ""},
            {"mode", "Chế độ giới hạn (1 = chung, 2 = riêng từng vị trí)", ParamType::Int, 1, 1, 2, "", ""},
            {"min", "Giá trị nhỏ nhất (chế độ 1)", ParamType::Int, 1, LLONG_MIN, LLONG_MAX, "", ""},
            {"max", "Giá trị lớn nhất (chế độ 1)", ParamType::Int, 100, LLONG_MIN, LLONG_MAX, "", ""},
            {"bounds", "Giới hạn riêng (chế độ 2), dạng lo1:hi1,lo2:hi2,...", ParamType::Text, 0, 0, 0, "", ""},
            {"header", "Ghi n ở dòng đầu?", ParamType::Bool, 0, 0, 1, "", ""},
            {"layout", "Bố cục (1 = cùng một dòng, 2 = mỗi số một dòng)", ParamType::Int, 1, 1, 2, "", ""},
        };
    }

    int interactiveSteps() const override { return 2; }

    void configureInteractive(Config& c) const override {
        con::println("");
        con::println("Bước 2/6: Số lượng số nguyên trong mỗi test");
        c.set("n", con::askInt("Mỗi test case có bao nhiêu số nguyên?", c.i("n"), 1, kMaxCount));
        const ll n = c.i("n");

        con::println("");
        con::println("Bước 3/6: Chọn chế độ giới hạn:");
        con::println("1. Dùng chung giới hạn cho tất cả số");
        con::println("2. Đặt giới hạn riêng cho từng vị trí");
        c.set("mode", con::askInt("Lựa chọn", c.i("mode"), 1, 2));

        if (c.i("mode") == 1) {
            for (;;) {
                ll lo = con::askInt("Giá trị nhỏ nhất", c.i("min"), LLONG_MIN, LLONG_MAX);
                ll hi = con::askInt("Giá trị lớn nhất", std::max(c.i("max"), lo), LLONG_MIN, LLONG_MAX);
                if (lo <= hi) {
                    c.set("min", lo);
                    c.set("max", hi);
                    break;
                }
                con::println("  ! Giá trị nhỏ nhất phải <= giá trị lớn nhất. Nhập lại.");
            }
            c.setText("bounds", "");
        } else {
            if (n > 50)
                con::println("  (n = " + std::to_string(n) +
                             " khá lớn. Mẹo: nhấn Enter để dùng lại giới hạn của số ngay trước.)");
            std::vector<std::pair<ll, ll>> b;
            ll prevLo = c.i("min"), prevHi = c.i("max");
            for (ll i = 1; i <= n; ++i) {
                con::println("");
                con::println("Số thứ " + std::to_string(i) + ":");
                for (;;) {
                    ll lo = con::askInt("  Giá trị nhỏ nhất", prevLo, LLONG_MIN, LLONG_MAX);
                    ll hi = con::askInt("  Giá trị lớn nhất", std::max(prevHi, lo), LLONG_MIN, LLONG_MAX);
                    if (lo <= hi) {
                        b.emplace_back(lo, hi);
                        prevLo = lo;
                        prevHi = hi;
                        break;
                    }
                    con::println("  ! Giá trị nhỏ nhất phải <= giá trị lớn nhất. Nhập lại số thứ " +
                                 std::to_string(i) + ".");
                }
            }
            c.setText("bounds", boundsToText(b));
        }

        con::println("");
        con::println("Định dạng file input hiện tại: " + std::string(c.b("header") ? "dòng đầu ghi n, " : "") +
                     (c.i("layout") == 1 ? "n số trên cùng một dòng." : "mỗi số một dòng."));
        if (con::askYesNo("Thay đổi định dạng file input?", false)) {
            c.set("header", con::askYesNo("  Ghi n ở dòng đầu?", c.b("header")) ? 1 : 0);
            c.set("layout", con::askInt("  Bố cục (1 = cùng một dòng, 2 = mỗi số một dòng)", c.i("layout"), 1, 2));
        }
    }

    void finalizeCliConfig(Config& c, const std::set<std::string>& keys) const override {
        if (keys.count("bounds")) {
            // Có --bounds thì mặc định là chế độ 2; lỗi cú pháp sẽ được validateConfig báo.
            if (!keys.count("mode")) c.set("mode", 2);
            std::vector<std::pair<ll, ll>> b;
            if (parseBounds(c.t("bounds"), b).empty() && !keys.count("n")) c.set("n", static_cast<ll>(b.size()));
        }
    }

    Error validateConfig(const Config& c) const override {
        if (c.i("mode") == 1) {
            if (!trim(c.t("bounds")).empty())
                return "bounds (giới hạn riêng) chỉ dùng với mode = 2; chế độ 1 dùng --min/--max.";
            if (c.i("min") > c.i("max")) return "Giá trị nhỏ nhất (min) phải <= giá trị lớn nhất (max).";
            return "";
        }
        std::vector<std::pair<ll, ll>> b;
        if (auto e = parseBounds(c.t("bounds"), b); !e.empty()) return e;
        if (static_cast<ll>(b.size()) != c.i("n"))
            return "Số vị trí trong bounds (" + std::to_string(b.size()) + ") khác n (" + std::to_string(c.i("n")) + ").";
        return "";
    }

    std::string generate(Rng& rng, const Config& c) const override {
        const ll n = c.i("n");
        std::vector<std::pair<ll, ll>> b;
        if (c.i("mode") == 2) parseBounds(c.t("bounds"), b);
        const char sep = c.i("layout") == 1 ? ' ' : '\n';
        std::string s;
        s.reserve(static_cast<size_t>(n) * 12 + 16);
        if (c.b("header")) {
            appendLL(s, n);
            s += '\n';
        }
        for (ll i = 0; i < n; ++i) {
            if (i) s += sep;
            if (c.i("mode") == 1) appendLL(s, rng.range(c.i("min"), c.i("max")));
            else appendLL(s, rng.range(b[static_cast<size_t>(i)].first, b[static_cast<size_t>(i)].second));
        }
        s += '\n';
        return s;
    }

    Error validateInput(const std::string& in, const Config& c) const override {
        std::vector<std::vector<std::string>> L;
        if (auto e = splitInputLines(in, L); !e.empty()) return e;
        const ll n = c.i("n");
        size_t idx = 0;
        if (c.b("header")) {
            ll v;
            if (L.empty() || L[0].size() != 1) return "Dòng 1 phải chứa n.";
            if (auto e = readIntToken(L[0][0], n, n, "n", v); !e.empty()) return e;
            idx = 1;
        }
        std::vector<std::string> toks;
        if (c.i("layout") == 1) {
            if (L.size() != idx + 1) return "Các số phải nằm trên đúng một dòng.";
            toks = L[idx];
        } else {
            if (static_cast<ll>(L.size() - idx) != n) return "Phải có đúng n dòng, mỗi dòng một số.";
            for (size_t k = idx; k < L.size(); ++k) {
                if (L[k].size() != 1) return "Dòng " + std::to_string(k + 1) + " phải chứa đúng một số.";
                toks.push_back(L[k][0]);
            }
        }
        if (static_cast<ll>(toks.size()) != n)
            return "Có " + std::to_string(toks.size()) + " số, cần " + std::to_string(n) + ".";
        std::vector<std::pair<ll, ll>> b;
        if (c.i("mode") == 2) parseBounds(c.t("bounds"), b);
        for (size_t k = 0; k < toks.size(); ++k) {
            ll lo = c.i("mode") == 1 ? c.i("min") : b[k].first;
            ll hi = c.i("mode") == 1 ? c.i("max") : b[k].second;
            ll v;
            if (auto e = readIntToken(toks[k], lo, hi, "Số thứ " + std::to_string(k + 1), v); !e.empty()) return e;
        }
        return "";
    }

    ull estimateMaxBytes(const Config& c) const override {
        ull d = 20;
        if (c.i("mode") == 1) d = maxDigits(c.i("min"), c.i("max"));
        return static_cast<ull>(c.i("n")) * (d + 1) + 10;
    }

    std::string describeConfig(const Config& c) const override {
        std::string s = "  - Số lượng số nguyên mỗi test (n): " + std::to_string(c.i("n")) + "\n";
        if (c.i("mode") == 1) {
            s += "  - Chế độ: giới hạn chung [" + std::to_string(c.i("min")) + ", " + std::to_string(c.i("max")) + "]\n";
        } else {
            s += "  - Chế độ: giới hạn riêng từng vị trí\n";
            std::vector<std::pair<ll, ll>> b;
            parseBounds(c.t("bounds"), b);
            size_t show = b.size() > 20 ? 20 : b.size();
            for (size_t i = 0; i < show; ++i)
                s += "      Số thứ " + std::to_string(i + 1) + ": [" + std::to_string(b[i].first) + ", " +
                     std::to_string(b[i].second) + "]\n";
            if (b.size() > show) s += "      ... (" + std::to_string(b.size() - show) + " vị trí nữa)\n";
        }
        s += std::string("  - Ghi n ở dòng đầu: ") + (c.b("header") ? "có" : "không") + "\n";
        s += std::string("  - Bố cục: ") + (c.i("layout") == 1 ? "cùng một dòng" : "mỗi số một dòng") + "\n";
        return s;
    }
};

}  // namespace

std::unique_ptr<Problem> makeCustom() { return std::make_unique<CustomProblem>(); }

}  // namespace ojgen
