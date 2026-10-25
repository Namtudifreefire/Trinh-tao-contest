// graph.cpp - Module: đồ thị.
//
// Định dạng input:
//   n m
//   m dòng: u v      (hoặc u v w nếu weighted = 1), 1 <= u, v <= n
//
// Tùy chọn:
//   directed  = 1 : có hướng (u -> v)
//   simple    = 1 : đồ thị đơn - không khuyên (u = v), không cạnh lặp
//                   (vô hướng: {u,v} không lặp; có hướng: (u,v) không lặp)
//   connected = 1 : liên thông (có hướng: liên thông yếu), cần m >= n - 1
//   weighted  = 1 : mỗi cạnh có trọng số w trong [minW, maxW]
#include <algorithm>
#include <climits>
#include <numeric>
#include <unordered_set>

#include "problem.h"
#include "problem_list.h"

namespace ojgen {
namespace {

constexpr ll kMaxV = 200000;
constexpr ll kMaxE = 1000000;
constexpr ull kEnumLimit = 2000000;  // liệt kê toàn bộ cặp đỉnh khi số cặp <= ngưỡng này

struct Dsu {
    std::vector<int> p;
    explicit Dsu(int n) : p(static_cast<size_t>(n) + 1) { std::iota(p.begin(), p.end(), 0); }
    int f(int x) {
        while (p[static_cast<size_t>(x)] != x) x = p[static_cast<size_t>(x)] = p[static_cast<size_t>(p[static_cast<size_t>(x)])];
        return x;
    }
    bool unite(int a, int b) {
        a = f(a), b = f(b);
        if (a == b) return false;
        p[static_cast<size_t>(a)] = b;
        return true;
    }
};

class GraphProblem : public Problem {
public:
    std::string id() const override { return "graph"; }
    std::string title() const override { return "Đồ thị"; }
    std::string formatDescription() const override {
        return "Dòng 1: n m (số đỉnh, số cạnh).\n"
               "m dòng tiếp: u v (hoặc u v w khi weighted = 1), đỉnh đánh số 1..n.\n"
               "  directed = 1: có hướng.  simple = 1: không khuyên, không cạnh lặp.\n"
               "  connected = 1: liên thông (có hướng: liên thông yếu), cần m >= n - 1.\n"
               "  Đồ thị đơn vô hướng n đỉnh có tối đa n(n-1)/2 cạnh; có hướng tối đa n(n-1).\n"
               "Ví dụ (vô hướng, có trọng số):\n  4 3\n  1 2 7\n  2 4 3\n  3 1 10\n";
    }
    std::vector<ParamSpec> params() const override {
        return {
            {"minN", "Số đỉnh nhỏ nhất (minN)", ParamType::Int, 2, 1, kMaxV, "", ""},
            {"maxN", "Số đỉnh lớn nhất (maxN)", ParamType::Int, 8, 1, kMaxV, "", ""},
            {"minM", "Số cạnh nhỏ nhất (minM)", ParamType::Int, 1, 0, kMaxE, "", ""},
            {"maxM", "Số cạnh lớn nhất (maxM)", ParamType::Int, 10, 0, kMaxE, "", ""},
            {"directed", "Đồ thị có hướng?", ParamType::Bool, 0, 0, 1, "", ""},
            {"simple", "Đồ thị đơn (không khuyên, không cạnh lặp)?", ParamType::Bool, 1, 0, 1, "", ""},
            {"connected", "Bắt buộc liên thông?", ParamType::Bool, 0, 0, 1, "", ""},
            {"weighted", "Có trọng số cạnh?", ParamType::Bool, 0, 0, 1, "", ""},
            {"minW", "Trọng số nhỏ nhất", ParamType::Int, 1, LLONG_MIN, LLONG_MAX, "", "chỉ dùng khi có trọng số"},
            {"maxW", "Trọng số lớn nhất", ParamType::Int, 1000000000, LLONG_MIN, LLONG_MAX, "", ""},
        };
    }

    static ull maxEdgesSimple(ll n, bool directed) {
        ull un = static_cast<ull>(n);
        return directed ? un * (un - 1) : un * (un - 1) / 2;
    }

    // Miền số cạnh hợp lệ với n đỉnh. Trả về false nếu rỗng.
    static bool edgeRange(const Config& c, ll n, ll& lo, ll& hi) {
        lo = c.i("minM");
        if (c.b("connected")) lo = std::max(lo, n - 1);
        hi = c.i("maxM");
        if (c.b("simple")) {
            ull me = maxEdgesSimple(n, c.b("directed"));
            if (me < static_cast<ull>(hi)) hi = static_cast<ll>(me);
        }
        return lo <= hi;
    }

    void configureInteractive(Config& c) const override {
        auto ps = params();
        for (const auto& p : ps) {
            if ((p.key == "minW" || p.key == "maxW") && !c.b("weighted")) continue;
            askParam(p, c);
        }
    }

    Error validateConfig(const Config& c) const override {
        if (c.i("minN") > c.i("maxN")) return "minN phải <= maxN.";
        if (c.i("minM") > c.i("maxM")) return "minM phải <= maxM.";
        if (c.b("weighted") && c.i("minW") > c.i("maxW")) return "minW phải <= maxW.";
        ll lo, hi;
        for (ll n = c.i("minN"); n <= c.i("maxN"); ++n)
            if (edgeRange(c, n, lo, hi)) return "";
        std::string why = "Không có số đỉnh n nào trong [minN, maxN] cho phép số cạnh trong [minM, maxM].";
        if (c.b("simple"))
            why += "\n  Đồ thị đơn " + std::string(c.b("directed") ? "có hướng" : "vô hướng") + " với maxN = " +
                   std::to_string(c.i("maxN")) + " đỉnh có tối đa " +
                   std::to_string(maxEdgesSimple(c.i("maxN"), c.b("directed"))) + " cạnh.";
        if (c.b("connected"))
            why += "\n  Đồ thị liên thông với minN = " + std::to_string(c.i("minN")) + " đỉnh cần ít nhất " +
                   std::to_string(c.i("minN") - 1) + " cạnh.";
        return why;
    }

    std::string generate(Rng& rng, const Config& c) const override {
        const bool directed = c.b("directed"), simple = c.b("simple"), connected = c.b("connected");
        // Chọn n sao cho tồn tại số cạnh hợp lệ.
        ll n = 0, lo = 0, hi = -1;
        for (int tries = 0; tries < 64; ++tries) {
            n = rng.range(c.i("minN"), c.i("maxN"));
            if (edgeRange(c, n, lo, hi)) break;
        }
        if (lo > hi) {
            std::vector<ll> ok;
            for (ll x = c.i("minN"); x <= c.i("maxN"); ++x)
                if (edgeRange(c, x, lo, hi)) ok.push_back(x);
            n = ok[static_cast<size_t>(rng.below(ok.size()))];
            edgeRange(c, n, lo, hi);
        }
        const ll m = rng.range(lo, hi);
        const ull un = static_cast<ull>(n);
        auto key = [&](ll u, ll v) -> ull {
            if (!directed && u > v) std::swap(u, v);
            return static_cast<ull>(u - 1) * un + static_cast<ull>(v - 1);
        };

        std::vector<std::pair<ll, ll>> edges;
        edges.reserve(static_cast<size_t>(m));
        std::unordered_set<ull> used;
        if (connected && n > 1) {
            // Cây khung ngẫu nhiên: mỗi đỉnh nối với một đỉnh đứng trước trong hoán vị.
            std::vector<ll> perm(static_cast<size_t>(n));
            std::iota(perm.begin(), perm.end(), 1);
            rng.shuffle(perm);
            for (size_t i = 1; i < perm.size(); ++i) {
                ll u = perm[i], v = perm[static_cast<size_t>(rng.below(i))];
                if (rng.chance(50)) std::swap(u, v);
                edges.emplace_back(u, v);
                if (simple) used.insert(key(u, v));
            }
        }
        size_t need = static_cast<size_t>(m) - edges.size();
        if (!simple) {
            for (size_t k = 0; k < need; ++k) edges.emplace_back(rng.range(1, n), rng.range(1, n));
        } else if (need > 0) {
            ull maxE = maxEdgesSimple(n, directed);
            if (maxE <= kEnumLimit && static_cast<ull>(m) * 2 > maxE) {
                // Đồ thị dày: liệt kê mọi cặp còn trống rồi chọn ngẫu nhiên.
                std::vector<std::pair<ll, ll>> cand;
                cand.reserve(static_cast<size_t>(maxE));
                for (ll u = 1; u <= n; ++u)
                    for (ll v = directed ? 1 : u + 1; v <= n; ++v)
                        if (u != v && !used.count(key(u, v))) cand.emplace_back(u, v);
                for (size_t k = 0; k < need; ++k) {
                    size_t j = k + static_cast<size_t>(rng.below(cand.size() - k));
                    std::swap(cand[k], cand[j]);
                    edges.push_back(cand[k]);
                }
            } else {
                used.reserve(static_cast<size_t>(m) * 2);
                while (edges.size() < static_cast<size_t>(m)) {
                    ll u = rng.range(1, n), v = rng.range(1, n);
                    if (u == v) continue;
                    if (used.insert(key(u, v)).second) edges.emplace_back(u, v);
                }
            }
        }
        rng.shuffle(edges);
        if (!directed)
            for (auto& e : edges)
                if (rng.chance(50)) std::swap(e.first, e.second);

        std::string s;
        s.reserve(static_cast<size_t>(m) * 26 + 24);
        appendLL(s, n);
        s += ' ';
        appendLL(s, m);
        s += '\n';
        for (const auto& e : edges) {
            appendLL(s, e.first);
            s += ' ';
            appendLL(s, e.second);
            if (c.b("weighted")) {
                s += ' ';
                appendLL(s, rng.range(c.i("minW"), c.i("maxW")));
            }
            s += '\n';
        }
        return s;
    }

    Error validateInput(const std::string& in, const Config& c) const override {
        std::vector<std::vector<std::string>> L;
        if (auto e = splitInputLines(in, L); !e.empty()) return e;
        if (L[0].size() != 2) return "Dòng 1 phải chứa n m.";
        ll n, m, lo, hi;
        if (auto e = readIntToken(L[0][0], c.i("minN"), c.i("maxN"), "n", n); !e.empty()) return e;
        if (!edgeRange(c, n, lo, hi)) return "Không có số cạnh hợp lệ với n = " + std::to_string(n) + ".";
        if (auto e = readIntToken(L[0][1], lo, hi, "m", m); !e.empty()) return e;
        if (static_cast<ll>(L.size()) != m + 1) return "Số dòng cạnh khác m.";
        const bool directed = c.b("directed");
        const size_t width = c.b("weighted") ? 3 : 2;
        std::unordered_set<ull> seen;
        Dsu dsu(static_cast<int>(n));
        ll comps = n;
        for (ll k = 1; k <= m; ++k) {
            const auto& t = L[static_cast<size_t>(k)];
            std::string where = "Cạnh " + std::to_string(k);
            if (t.size() != width) return where + ": cần " + std::to_string(width) + " số.";
            ll u, v, w;
            if (auto e = readIntToken(t[0], 1, n, where + " (u)", u); !e.empty()) return e;
            if (auto e = readIntToken(t[1], 1, n, where + " (v)", v); !e.empty()) return e;
            if (width == 3)
                if (auto e = readIntToken(t[2], c.i("minW"), c.i("maxW"), where + " (w)", w); !e.empty()) return e;
            if (c.b("simple")) {
                if (u == v) return where + ": khuyên (u = v) trong đồ thị đơn.";
                ll a = u, b = v;
                if (!directed && a > b) std::swap(a, b);
                if (!seen.insert(static_cast<ull>(a - 1) * static_cast<ull>(n) + static_cast<ull>(b - 1)).second)
                    return where + ": cạnh lặp trong đồ thị đơn.";
            }
            if (dsu.unite(static_cast<int>(u), static_cast<int>(v))) --comps;
        }
        if (c.b("connected") && comps != 1) return "Đồ thị không liên thông.";
        return "";
    }

    ull estimateMaxBytes(const Config& c) const override {
        ull per = 2 * std::to_string(c.i("maxN")).size() + 2;
        if (c.b("weighted")) per += maxDigits(c.i("minW"), c.i("maxW")) + 1;
        return static_cast<ull>(c.i("maxM")) * per + 24;
    }
};

}  // namespace

std::unique_ptr<Problem> makeGraph() { return std::make_unique<GraphProblem>(); }

}  // namespace ojgen
