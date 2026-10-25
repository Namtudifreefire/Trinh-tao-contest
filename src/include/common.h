// common.h - Kiểu dữ liệu và hàm tiện ích dùng chung cho toàn bộ ojgen.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ojgen {

using ll = long long;
using ull = unsigned long long;

constexpr const char* kVersion = "2.0.0";

// Cắt khoảng trắng hai đầu.
std::string trim(const std::string& s);
std::string toLower(std::string s);

// Tách theo khoảng trắng (bỏ token rỗng).
std::vector<std::string> splitWS(const std::string& s);
// Tách theo ký tự phân cách (giữ token rỗng).
std::vector<std::string> split(const std::string& s, char sep);

// Phân tích số nguyên 64-bit chặt chẽ. Hỗ trợ dạng "123", "-45", "+7",
// "1e9", "2e5" (ký hiệu khoa học với phần mũ nguyên). Trả về false nếu
// chuỗi sai định dạng hoặc vượt phạm vi long long.
bool parseLL(const std::string& s, ll& out);
bool parseULL(const std::string& s, ull& out);

// Phân tích token số nguyên theo đúng định dạng xuất (chỉ chữ số, dấu '-').
// Dùng khi xác thực file input đã sinh.
bool parseStrictToken(const std::string& s, ll& out);

// Độ rộng đoạn [lo, hi] dưới dạng unsigned (hi - lo), an toàn với tràn số.
inline ull spanOf(ll lo, ll hi) { return static_cast<ull>(hi) - static_cast<ull>(lo); }

std::string fmtBytes(ull bytes);

// Hàm băm FNV-1a 64-bit.
ull fnv1a(const std::string& data, ull h = 1469598103934665603ULL);
std::string toHex(ull v);

// Kết quả kiểm tra: rỗng = hợp lệ, ngược lại là thông báo lỗi.
using Error = std::string;

// Nối số nguyên vào chuỗi (nhanh hơn std::to_string khi sinh dữ liệu lớn).
void appendLL(std::string& out, ll v);

// Số chữ số thập phân tối đa của một giá trị trong [lo, hi] (kể cả dấu '-').
ull maxDigits(ll lo, ll hi);

}  // namespace ojgen
