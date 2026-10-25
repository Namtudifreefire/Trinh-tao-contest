// zip_manager.h - Tạo và kiểm tra file ZIP (không cần thư viện ngoài).
//
// Cài đặt định dạng ZIP chuẩn (PKWARE APPNOTE): nén Deflate (RFC 1951, mã
// Huffman cố định + LZ77) hoặc chỉ lưu (stored). Tên file mã hóa UTF-8.
// Giới hạn: tổng dung lượng < 4 GB và < 65535 file (không hỗ trợ ZIP64).
#pragma once

#include <string>
#include <vector>

#include "file_manager.h"

namespace ojgen {

struct ZipEntry {
    std::string name;  // tên trong ZIP (vd "test01.inp")
    fs::path file;     // file nguồn trên đĩa
};

struct ZipStats {
    size_t files = 0;
    ull rawBytes = 0;
    ull zipBytes = 0;
};

// Ghi ra file tạm rồi đổi tên, nên ZIP cũ chỉ bị thay khi ZIP mới hoàn tất.
bool createZip(const fs::path& zipPath, const std::vector<ZipEntry>& entries, bool compress, ZipStats* stats,
               std::string& err);

// Đọc lại ZIP: kiểm tra cấu trúc, danh sách tên khớp expectedNames, giải nén
// từng file và so CRC-32 + dung lượng.
bool verifyZip(const fs::path& zipPath, const std::vector<std::string>& expectedNames, std::string& err);

// Dùng cho kiểm thử: nén/giải nén một vùng nhớ.
std::string deflateRaw(const std::string& data);
bool inflateRaw(const std::string& comp, size_t expectedSize, std::string& out);
unsigned crc32(const std::string& data);

}  // namespace ojgen
