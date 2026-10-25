// string.cpp - Module: xâu ký tự.
//
// Định dạng input:
//   [len]      (chỉ khi printLen = 1)
//   s
#include <set>

#include "problem.h"
#include "problem_list.h"

namespace ojgen {
namespace {

// Phân tích tập ký tự: tên có sẵn hoặc danh sách ký tự cụ thể.
Error resolveCharset(const std::string& spec, std::string& out) {
    std::string k = toLower(trim(spec));
    out.clear();
    auto rangeAdd = [&](char a, char b) {
        for (char ch = a; ch <= b; ++ch) out += ch;
    };
    if (k == "lower") rangeAdd('a', 'z');
    else if (k == "upper") rangeAdd('A', 'Z');
    else if (k == "letters") rangeAdd('A', 'Z'), rangeAdd('a', 'z');
    else if (k == "digits") rangeAdd('0', '9');
    else if (k == "alnum") rangeAdd('0', '9'), rangeAdd('A', 'Z'), rangeAdd('a', 'z');
    else if (k == "binary") out = "01";
    else if (k == "dna") out = "ACGT";
    else {
        std::string lit = spec;
        if (lit.rfind("set:", 0) == 0) lit = lit.substr(4);
        std::set<char> seen;
        for (char ch : lit) {
            unsigned char u = static_cast<unsigned char>(ch);
            if (u < 33 || u > 126)
                return "Tập ký tự chỉ được chứa ký tự ASCII in được, không gồm dấu cách/tiếng Việt có dấu.";
            if (seen.insert(ch).second) out += ch;
        }
        if (out.empty()) return "Tập ký tự rỗng.";
    }
    return "";
}

class StringProblem : public Problem {
public:
    std::string id() const override { return "string"; }
    std::string title() const override { return "Xâu ký tự"; }
    std::string formatDescription() const override {
        return "Nếu printLen = 1: dòng 1 là độ dài L của xâu.\n"
               "Dòng tiếp theo: xâu s gồm L ký tự (minLen <= L <= maxLen) lấy từ tập charset.\n"
               "charset: lower (a-z), upper (A-Z), letters, digits, alnum, binary (01), dna (ACGT),\n"
               "  hoặc tự liệt kê ký tự, vd: abc  hoặc  set:xyz# (tiền tố set: để ép hiểu là danh sách).\n"
               "Ví dụ (printLen = 1, charset = lower):\n  5\n  hello\n";
    }
    std::vector<ParamSpec> params() const override {
        return {
            {"minLen", "Độ dài nhỏ nhất", ParamType::Int, 1, 1, 10000000, "", ""},
            {"maxLen", "Độ dài lớn nhất", ParamType::Int, 20, 1, 10000000, "", ""},
            {"charset", "Tập ký tự", ParamType::Text, 0, 0, 0, "lower",
             "lower | upper | letters | digits | alnum | binary | dna | hoặc danh sách ký tự, vd abc"},
            {"printLen", "Ghi độ dài xâu ở dòng đầu?", ParamType::Bool, 0, 0, 1, "", ""},
        };
    }
    Error validateConfig(const Config& c) const override {
        if (c.i("minLen") > c.i("maxLen")) return "minLen phải <= maxLen.";
        std::string cs;
        return resolveCharset(c.t("charset"), cs);
    }
    std::string generate(Rng& rng, const Config& c) const override {
        std::string cs;
        resolveCharset(c.t("charset"), cs);
        ll len = rng.range(c.i("minLen"), c.i("maxLen"));
        std::string s;
        s.reserve(static_cast<size_t>(len) + 24);
        if (c.b("printLen")) {
            appendLL(s, len);
            s += '\n';
        }
        for (ll i = 0; i < len; ++i) s += cs[static_cast<size_t>(rng.below(cs.size()))];
        s += '\n';
        return s;
    }
    Error validateInput(const std::string& in, const Config& c) const override {
        std::vector<std::vector<std::string>> L;
        if (auto e = splitInputLines(in, L); !e.empty()) return e;
        size_t expectLines = c.b("printLen") ? 2 : 1;
        if (L.size() != expectLines) return "Số dòng phải là " + std::to_string(expectLines) + ".";
        const auto& sl = L.back();
        if (sl.size() != 1) return "Xâu không được chứa dấu cách.";
        const std::string& s = sl[0];
        ll len = static_cast<ll>(s.size());
        if (len < c.i("minLen") || len > c.i("maxLen")) return "Độ dài xâu nằm ngoài [minLen, maxLen].";
        if (c.b("printLen")) {
            ll L0;
            if (L[0].size() != 1) return "Dòng 1 phải chứa đúng độ dài xâu.";
            if (auto e = readIntToken(L[0][0], c.i("minLen"), c.i("maxLen"), "Độ dài", L0); !e.empty()) return e;
            if (L0 != len) return "Độ dài ghi ở dòng 1 khác độ dài thực của xâu.";
        }
        std::string cs;
        resolveCharset(c.t("charset"), cs);
        for (char ch : s)
            if (cs.find(ch) == std::string::npos) return std::string("Ký tự '") + ch + "' không thuộc charset.";
        return "";
    }
    ull estimateMaxBytes(const Config& c) const override { return static_cast<ull>(c.i("maxLen")) + 12; }
};

}  // namespace

std::unique_ptr<Problem> makeString() { return std::make_unique<StringProblem>(); }

}  // namespace ojgen
