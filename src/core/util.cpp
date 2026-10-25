// util.cpp - Hàm tiện ích chung.
#include "common.h"

#include <cctype>
#include <charconv>
#include <climits>
#include <cstdio>

namespace ojgen {

std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::vector<std::string> splitWS(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) out.push_back(cur), cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) out.push_back(cur), cur.clear();
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

static bool parsePlainLL(const std::string& s, ll& out) {
    if (s.empty()) return false;
    const char* b = s.data();
    const char* e = b + s.size();
    if (*b == '+') ++b;
    if (b == e) return false;
    auto r = std::from_chars(b, e, out);
    return r.ec == std::errc() && r.ptr == e;
}

bool parseLL(const std::string& raw, ll& out) {
    std::string s = trim(raw);
    // Bỏ dấu phân cách hàng nghìn kiểu "1_000_000" hoặc "1'000'000".
    std::string t;
    for (char c : s)
        if (c != '_' && c != '\'') t += c;
    s = t;
    if (s.empty()) return false;
    size_t epos = s.find_first_of("eE");
    if (epos == std::string::npos) return parsePlainLL(s, out);
    ll mant = 0, ex = 0;
    if (!parsePlainLL(s.substr(0, epos), mant)) return false;
    if (!parsePlainLL(s.substr(epos + 1), ex)) return false;
    if (ex < 0 || ex > 18) return false;
    __int128 v = mant;
    for (ll i = 0; i < ex; ++i) {
        v *= 10;
        if (v > LLONG_MAX || v < LLONG_MIN) return false;
    }
    out = static_cast<ll>(v);
    return true;
}

bool parseULL(const std::string& raw, ull& out) {
    std::string s = trim(raw);
    if (s.empty() || s[0] == '-') return false;
    const char* b = s.data();
    const char* e = b + s.size();
    if (*b == '+') ++b;
    auto r = std::from_chars(b, e, out);
    return r.ec == std::errc() && r.ptr == e && b != e;
}

bool parseStrictToken(const std::string& s, ll& out) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-') {
        if (s.size() == 1) return false;
        i = 1;
    }
    for (size_t j = i; j < s.size(); ++j)
        if (s[j] < '0' || s[j] > '9') return false;
    // Không cho phép số 0 thừa ở đầu ("007", "-0").
    if (s.size() - i > 1 && s[i] == '0') return false;
    if (s == "-0") return false;
    auto r = std::from_chars(s.data(), s.data() + s.size(), out);
    return r.ec == std::errc() && r.ptr == s.data() + s.size();
}

std::string fmtBytes(ull bytes) {
    char buf[64];
    if (bytes < 1024ULL) std::snprintf(buf, sizeof buf, "%llu B", bytes);
    else if (bytes < 1024ULL * 1024) std::snprintf(buf, sizeof buf, "%.1f KB", bytes / 1024.0);
    else if (bytes < 1024ULL * 1024 * 1024) std::snprintf(buf, sizeof buf, "%.1f MB", bytes / 1048576.0);
    else std::snprintf(buf, sizeof buf, "%.2f GB", bytes / 1073741824.0);
    return buf;
}

ull fnv1a(const std::string& data, ull h) {
    for (unsigned char c : data) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

void appendLL(std::string& out, ll v) {
    char buf[24];
    auto r = std::to_chars(buf, buf + sizeof buf, v);
    out.append(buf, r.ptr);
}

ull maxDigits(ll lo, ll hi) {
    auto len = [](ll v) { return static_cast<ull>(std::to_string(v).size()); };
    ull a = len(lo), b = len(hi);
    return a > b ? a : b;
}

std::string toHex(ull v) {
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", v);
    return buf;
}

}  // namespace ojgen
