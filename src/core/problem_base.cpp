// problem_base.cpp - Cài đặt mặc định của lớp Problem và hàm hỗ trợ xác thực.
#include <sstream>

#include "console.h"
#include "problem.h"

namespace ojgen {

ll Config::i(const std::string& k) const {
    auto it = ints.find(k);
    return it == ints.end() ? 0 : it->second;
}

std::string Config::t(const std::string& k) const {
    auto it = texts.find(k);
    return it == texts.end() ? std::string() : it->second;
}

Config Problem::defaultConfig() const {
    Config c;
    for (const auto& p : params()) {
        if (p.type == ParamType::Text) c.setText(p.key, p.defText);
        else c.set(p.key, p.def);
    }
    return c;
}

void askParam(const ParamSpec& ps, Config& cfg) {
    if (!ps.help.empty()) con::println("  (" + ps.help + ")");
    switch (ps.type) {
        case ParamType::Int:
            cfg.set(ps.key, con::askInt("  " + ps.label, cfg.i(ps.key), ps.lo, ps.hi));
            break;
        case ParamType::Bool:
            cfg.set(ps.key, con::askYesNo("  " + ps.label, cfg.b(ps.key)) ? 1 : 0);
            break;
        case ParamType::Text:
            cfg.setText(ps.key, con::ask("  " + ps.label, cfg.t(ps.key)));
            break;
    }
}

void Problem::configureInteractive(Config& cfg) const {
    for (const auto& ps : params()) askParam(ps, cfg);
}

void Problem::finalizeCliConfig(Config&, const std::set<std::string>&) const {}

std::string Problem::describeConfig(const Config& cfg) const {
    std::ostringstream os;
    for (const auto& p : params()) {
        os << "  - " << p.label;
        if (p.label.find("(" + p.key + ")") == std::string::npos) os << " (" << p.key << ")";
        os << ": ";
        if (p.type == ParamType::Text) os << '"' << cfg.t(p.key) << '"';
        else if (p.type == ParamType::Bool) os << (cfg.b(p.key) ? "có" : "không");
        else os << cfg.i(p.key);
        os << "\n";
    }
    return os.str();
}

Error checkParamRanges(const Problem& p, const Config& cfg) {
    for (const auto& ps : p.params()) {
        if (ps.type == ParamType::Int) {
            ll v = cfg.i(ps.key);
            if (v < ps.lo || v > ps.hi)
                return "Tham số '" + ps.key + "' (" + ps.label + ") = " + std::to_string(v) +
                       " nằm ngoài miền cho phép [" + std::to_string(ps.lo) + ", " + std::to_string(ps.hi) + "].";
        } else if (ps.type == ParamType::Bool) {
            ll v = cfg.i(ps.key);
            if (v != 0 && v != 1) return "Tham số '" + ps.key + "' chỉ nhận 0 hoặc 1.";
        }
    }
    return "";
}

Error splitInputLines(const std::string& input, std::vector<std::vector<std::string>>& lines) {
    lines.clear();
    if (input.empty()) return "File input rỗng.";
    if (input.back() != '\n') return "File input không kết thúc bằng ký tự xuống dòng.";
    size_t start = 0;
    size_t lineNo = 0;
    while (start < input.size()) {
        size_t end = input.find('\n', start);
        std::string line = input.substr(start, end - start);
        ++lineNo;
        const std::string where = "Dòng " + std::to_string(lineNo) + ": ";
        if (line.empty()) return where + "dòng trống không hợp lệ.";
        if (line.find('\r') != std::string::npos) return where + "chứa ký tự \\r (CRLF).";
        if (line.front() == ' ' || line.back() == ' ') return where + "có khoảng trắng thừa ở đầu/cuối dòng.";
        if (line.find("  ") != std::string::npos) return where + "có hai dấu cách liên tiếp.";
        if (line.find('\t') != std::string::npos) return where + "chứa ký tự tab.";
        lines.push_back(split(line, ' '));
        start = end + 1;
    }
    return "";
}

Error readIntToken(const std::string& tok, ll lo, ll hi, const std::string& what, ll& out) {
    if (!parseStrictToken(tok, out)) return what + ": '" + tok + "' không phải số nguyên hợp lệ.";
    if (out < lo || out > hi)
        return what + " = " + tok + " nằm ngoài [" + std::to_string(lo) + ", " + std::to_string(hi) + "].";
    return "";
}

}  // namespace ojgen
