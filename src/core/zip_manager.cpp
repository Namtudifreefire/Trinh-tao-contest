// zip_manager.cpp - Ghi/đọc ZIP với Deflate (Huffman cố định + LZ77) tự cài đặt.
#include "zip_manager.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <fstream>
#include <map>

namespace ojgen {

// ------------------------------- CRC-32 --------------------------------------
static const std::array<unsigned, 256>& crcTable() {
    static const std::array<unsigned, 256> t = [] {
        std::array<unsigned, 256> a{};
        for (unsigned i = 0; i < 256; ++i) {
            unsigned c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            a[i] = c;
        }
        return a;
    }();
    return t;
}

unsigned crc32(const std::string& data) {
    const auto& t = crcTable();
    unsigned c = 0xFFFFFFFFu;
    for (unsigned char b : data) c = t[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// ------------------------------- Deflate -------------------------------------
static const int kLenBase[29] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const int kLenExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const int kDistBase[30] = {1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
                                  193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
static const int kDistExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

namespace {

class BitWriter {
public:
    explicit BitWriter(std::string& out) : out_(out) {}
    void bits(unsigned value, int n) {  // ghi n bit, bit thấp trước
        acc_ |= static_cast<unsigned long long>(value) << cnt_;
        cnt_ += n;
        while (cnt_ >= 8) {
            out_ += static_cast<char>(acc_ & 0xFF);
            acc_ >>= 8;
            cnt_ -= 8;
        }
    }
    void huff(unsigned code, int len) {  // mã Huffman ghi bit cao trước
        unsigned rev = 0;
        for (int i = 0; i < len; ++i) rev |= ((code >> i) & 1u) << (len - 1 - i);
        bits(rev, len);
    }
    void flush() {
        if (cnt_ > 0) out_ += static_cast<char>(acc_ & 0xFF);
        acc_ = 0;
        cnt_ = 0;
    }

private:
    std::string& out_;
    unsigned long long acc_ = 0;
    int cnt_ = 0;
};

void putLitLen(BitWriter& bw, int sym) {
    if (sym <= 143) bw.huff(0x30 + static_cast<unsigned>(sym), 8);
    else if (sym <= 255) bw.huff(0x190 + static_cast<unsigned>(sym - 144), 9);
    else if (sym <= 279) bw.huff(static_cast<unsigned>(sym - 256), 7);
    else bw.huff(0xC0 + static_cast<unsigned>(sym - 280), 8);
}

void putMatch(BitWriter& bw, int len, int dist) {
    int li = 28;
    while (kLenBase[li] > len) --li;
    putLitLen(bw, 257 + li);
    if (kLenExtra[li]) bw.bits(static_cast<unsigned>(len - kLenBase[li]), kLenExtra[li]);
    int di = 29;
    while (kDistBase[di] > dist) --di;
    bw.huff(static_cast<unsigned>(di), 5);
    if (kDistExtra[di]) bw.bits(static_cast<unsigned>(dist - kDistBase[di]), kDistExtra[di]);
}

}  // namespace

std::string deflateRaw(const std::string& data) {
    std::string out;
    out.reserve(data.size() / 2 + 64);
    BitWriter bw(out);
    bw.bits(1, 1);  // BFINAL
    bw.bits(1, 2);  // BTYPE = 01 (Huffman cố định)

    const size_t n = data.size();
    const unsigned char* p = reinterpret_cast<const unsigned char*>(data.data());
    constexpr int kWin = 32768, kHashBits = 15, kMaxChain = 48, kGood = 128;
    constexpr int kMask = kWin - 1;
    std::vector<int> head(1 << kHashBits, -1), prev(kWin, -1);
    auto hash3 = [&](size_t i) {
        return static_cast<int>(((p[i] << 10) ^ (p[i + 1] << 5) ^ p[i + 2]) & ((1 << kHashBits) - 1));
    };
    auto insert = [&](size_t i) {
        if (i + 2 >= n) return;
        int h = hash3(i);
        prev[i & kMask] = head[static_cast<size_t>(h)];
        head[static_cast<size_t>(h)] = static_cast<int>(i);
    };

    size_t i = 0;
    while (i < n) {
        int bestLen = 0, bestDist = 0;
        if (i + 2 < n) {
            int cand = head[static_cast<size_t>(hash3(i))];
            int chain = kMaxChain;
            size_t maxLen = std::min<size_t>(258, n - i);
            int last = static_cast<int>(i);
            while (cand >= 0 && cand < last && static_cast<int>(i) - cand <= kWin && chain-- > 0) {
                const unsigned char* a = p + cand;
                const unsigned char* b = p + i;
                if (a[bestLen] == b[bestLen] || bestLen == 0) {
                    size_t len = 0;
                    while (len < maxLen && a[len] == b[len]) ++len;
                    if (static_cast<int>(len) > bestLen) {
                        bestLen = static_cast<int>(len);
                        bestDist = static_cast<int>(i) - cand;
                        if (bestLen >= kGood || len == maxLen) break;
                    }
                }
                last = cand;
                cand = prev[static_cast<size_t>(cand) & kMask];
            }
        }
        if (bestLen >= 3) {
            putMatch(bw, bestLen, bestDist);
            for (int k = 0; k < bestLen; ++k) insert(i + static_cast<size_t>(k));
            i += static_cast<size_t>(bestLen);
        } else {
            putLitLen(bw, p[i]);
            insert(i);
            ++i;
        }
    }
    putLitLen(bw, 256);  // kết thúc block
    bw.flush();
    return out;
}

// ------------------------------- Inflate -------------------------------------
// Hỗ trợ block stored và Huffman cố định (đủ để kiểm tra ZIP do ojgen tạo).
namespace {
class BitReader {
public:
    BitReader(const std::string& s) : s_(s) {}
    bool bit(unsigned& b) {
        if (pos_ >= s_.size()) return false;
        b = (static_cast<unsigned char>(s_[pos_]) >> cnt_) & 1u;
        if (++cnt_ == 8) cnt_ = 0, ++pos_;
        return true;
    }
    bool bits(int n, unsigned& v) {
        v = 0;
        for (int i = 0; i < n; ++i) {
            unsigned b;
            if (!bit(b)) return false;
            v |= b << i;
        }
        return true;
    }
    bool huffBits(int n, unsigned& code) {  // đọc thêm n bit vào code (bit cao trước)
        for (int i = 0; i < n; ++i) {
            unsigned b;
            if (!bit(b)) return false;
            code = (code << 1) | b;
        }
        return true;
    }
    void alignByte() {
        if (cnt_) cnt_ = 0, ++pos_;
    }
    size_t pos() const { return pos_; }
    void skip(size_t k) { pos_ += k; }

private:
    const std::string& s_;
    size_t pos_ = 0;
    int cnt_ = 0;
};
}  // namespace

bool inflateRaw(const std::string& comp, size_t expectedSize, std::string& out) {
    out.clear();
    out.reserve(expectedSize);
    BitReader br(comp);
    for (;;) {
        unsigned final_, type;
        if (!br.bits(1, final_) || !br.bits(2, type)) return false;
        if (type == 0) {
            br.alignByte();
            size_t q = br.pos();
            if (q + 4 > comp.size()) return false;
            unsigned len = static_cast<unsigned char>(comp[q]) | (static_cast<unsigned char>(comp[q + 1]) << 8);
            unsigned nlen = static_cast<unsigned char>(comp[q + 2]) | (static_cast<unsigned char>(comp[q + 3]) << 8);
            if ((len ^ 0xFFFFu) != nlen || q + 4 + len > comp.size()) return false;
            out.append(comp, q + 4, len);
            br.skip(4 + len);
        } else if (type == 1) {
            for (;;) {
                unsigned code = 0;
                int sym;
                if (!br.huffBits(7, code)) return false;
                if (code <= 0x17) sym = 256 + static_cast<int>(code);
                else {
                    if (!br.huffBits(1, code)) return false;
                    if (code >= 0x30 && code <= 0xBF) sym = static_cast<int>(code - 0x30);
                    else if (code >= 0xC0 && code <= 0xC7) sym = 280 + static_cast<int>(code - 0xC0);
                    else {
                        if (!br.huffBits(1, code)) return false;
                        if (code < 0x190 || code > 0x1FF) return false;
                        sym = 144 + static_cast<int>(code - 0x190);
                    }
                }
                if (sym < 256) {
                    out += static_cast<char>(sym);
                } else if (sym == 256) {
                    break;
                } else {
                    int li = sym - 257;
                    if (li > 28) return false;
                    unsigned extra;
                    if (!br.bits(kLenExtra[li], extra)) return false;
                    size_t len = static_cast<size_t>(kLenBase[li]) + extra;
                    unsigned dcode = 0;
                    if (!br.huffBits(5, dcode) || dcode > 29) return false;
                    if (!br.bits(kDistExtra[dcode], extra)) return false;
                    size_t dist = static_cast<size_t>(kDistBase[dcode]) + extra;
                    if (dist > out.size()) return false;
                    size_t from = out.size() - dist;
                    for (size_t k = 0; k < len; ++k) out += out[from + k];
                }
                if (out.size() > expectedSize) return false;
            }
        } else {
            return false;  // block Huffman động: ojgen không tạo loại này
        }
        if (final_) break;
    }
    return out.size() == expectedSize;
}

// ------------------------------- ZIP -----------------------------------------
namespace {
void put16(std::string& s, unsigned v) {
    s += static_cast<char>(v & 0xFF);
    s += static_cast<char>((v >> 8) & 0xFF);
}
void put32(std::string& s, unsigned v) {
    put16(s, v & 0xFFFF);
    put16(s, (v >> 16) & 0xFFFF);
}
unsigned get16(const std::string& s, size_t o) {
    return static_cast<unsigned char>(s[o]) | (static_cast<unsigned char>(s[o + 1]) << 8);
}
unsigned get32(const std::string& s, size_t o) { return get16(s, o) | (get16(s, o + 2) << 16); }

void dosDateTime(unsigned& dtime, unsigned& ddate) {
    std::time_t now = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    int year = tmv.tm_year + 1900;
    if (year < 1980) year = 1980;
    dtime = static_cast<unsigned>((tmv.tm_hour << 11) | (tmv.tm_min << 5) | (tmv.tm_sec / 2));
    ddate = static_cast<unsigned>(((year - 1980) << 9) | ((tmv.tm_mon + 1) << 5) | tmv.tm_mday);
}
}  // namespace

bool createZip(const fs::path& zipPath, const std::vector<ZipEntry>& entries, bool compress, ZipStats* stats,
               std::string& err) {
    if (entries.size() >= 65535) {
        err = "Quá nhiều file cho định dạng ZIP thường (tối đa 65534).";
        return false;
    }
    fs::path tmp = zipPath;
    tmp += ".tmp";
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    if (!f) {
        err = "Không tạo được file ZIP: " + pathStr(tmp);
        return false;
    }
    unsigned dtime, ddate;
    dosDateTime(dtime, ddate);
    std::string central;
    ull offset = 0, raw = 0;
    for (const auto& e : entries) {
        std::string data;
        if (!readFile(e.file, data, &err)) {
            f.close();
            std::error_code ec;
            fs::remove(tmp, ec);
            return false;
        }
        unsigned crc = crc32(data);
        std::string body;
        unsigned method = 0;
        if (compress && !data.empty()) {
            body = deflateRaw(data);
            method = 8;
            if (body.size() >= data.size()) body = data, method = 0;
        } else {
            body = data;
        }
        if (offset + body.size() + 30 + e.name.size() >= 0xFFFFFFFFULL || data.size() >= 0xFFFFFFFFULL) {
            f.close();
            std::error_code ec;
            fs::remove(tmp, ec);
            err = "Dữ liệu vượt 4 GB, không hỗ trợ (cần ZIP64).";
            return false;
        }
        std::string lh;
        put32(lh, 0x04034b50);
        put16(lh, 20);       // phiên bản cần để giải nén (2.0)
        put16(lh, 0x0800);   // cờ: tên file mã hóa UTF-8
        put16(lh, method);
        put16(lh, dtime);
        put16(lh, ddate);
        put32(lh, crc);
        put32(lh, static_cast<unsigned>(body.size()));
        put32(lh, static_cast<unsigned>(data.size()));
        put16(lh, static_cast<unsigned>(e.name.size()));
        put16(lh, 0);
        lh += e.name;
        f.write(lh.data(), static_cast<std::streamsize>(lh.size()));
        f.write(body.data(), static_cast<std::streamsize>(body.size()));

        put32(central, 0x02014b50);
        put16(central, 20);  // tạo bởi: MS-DOS/FAT, phiên bản 2.0
        put16(central, 20);
        put16(central, 0x0800);
        put16(central, method);
        put16(central, dtime);
        put16(central, ddate);
        put32(central, crc);
        put32(central, static_cast<unsigned>(body.size()));
        put32(central, static_cast<unsigned>(data.size()));
        put16(central, static_cast<unsigned>(e.name.size()));
        put16(central, 0);  // extra
        put16(central, 0);  // comment
        put16(central, 0);  // disk
        put16(central, 0);  // thuộc tính trong
        put32(central, 0);  // thuộc tính ngoài
        put32(central, static_cast<unsigned>(offset));
        central += e.name;
        offset += lh.size() + body.size();
        raw += data.size();
    }
    std::string eocd;
    put32(eocd, 0x06054b50);
    put16(eocd, 0);
    put16(eocd, 0);
    put16(eocd, static_cast<unsigned>(entries.size()));
    put16(eocd, static_cast<unsigned>(entries.size()));
    put32(eocd, static_cast<unsigned>(central.size()));
    put32(eocd, static_cast<unsigned>(offset));
    put16(eocd, 0);
    f.write(central.data(), static_cast<std::streamsize>(central.size()));
    f.write(eocd.data(), static_cast<std::streamsize>(eocd.size()));
    f.close();
    if (!f) {
        std::error_code ec;
        fs::remove(tmp, ec);
        err = "Lỗi khi ghi file ZIP (ổ đĩa đầy?).";
        return false;
    }
    if (!moveFile(tmp, zipPath, &err)) return false;
    if (stats) {
        stats->files = entries.size();
        stats->rawBytes = raw;
        stats->zipBytes = offset + central.size() + eocd.size();
    }
    return true;
}

bool verifyZip(const fs::path& zipPath, const std::vector<std::string>& expectedNames, std::string& err) {
    std::string z;
    if (!readFile(zipPath, z, &err)) return false;
    if (z.size() < 22) {
        err = "File ZIP quá ngắn.";
        return false;
    }
    size_t eo = std::string::npos;
    for (size_t k = z.size() - 22 + 1; k-- > 0;) {
        if (get32(z, k) == 0x06054b50) {
            eo = k;
            break;
        }
        if (z.size() - k > 65557) break;
    }
    if (eo == std::string::npos) {
        err = "Không tìm thấy End of Central Directory.";
        return false;
    }
    unsigned count = get16(z, eo + 10), cdSize = get32(z, eo + 12), cdOff = get32(z, eo + 16);
    if (static_cast<size_t>(cdOff) + cdSize > eo) {
        err = "Central directory nằm ngoài file.";
        return false;
    }
    std::vector<std::string> names;
    size_t q = cdOff;
    for (unsigned k = 0; k < count; ++k) {
        if (q + 46 > z.size() || get32(z, q) != 0x02014b50) {
            err = "Central directory hỏng tại mục " + std::to_string(k + 1) + ".";
            return false;
        }
        unsigned method = get16(z, q + 10), crc = get32(z, q + 16), csize = get32(z, q + 20), usize = get32(z, q + 24);
        unsigned nlen = get16(z, q + 28), xlen = get16(z, q + 30), clen = get16(z, q + 32), lo = get32(z, q + 42);
        std::string name = z.substr(q + 46, nlen);
        q += 46 + nlen + xlen + clen;
        if (lo + 30ULL > z.size() || get32(z, lo) != 0x04034b50) {
            err = "Local header hỏng: " + name;
            return false;
        }
        unsigned lnlen = get16(z, lo + 26), lxlen = get16(z, lo + 28);
        if (z.substr(lo + 30, lnlen) != name) {
            err = "Tên trong local header không khớp: " + name;
            return false;
        }
        size_t dataOff = lo + 30 + lnlen + lxlen;
        if (dataOff + csize > z.size()) {
            err = "Dữ liệu nén vượt quá file: " + name;
            return false;
        }
        std::string comp = z.substr(dataOff, csize), plain;
        if (method == 0) plain = comp;
        else if (method == 8) {
            if (!inflateRaw(comp, usize, plain)) {
                err = "Giải nén thất bại: " + name;
                return false;
            }
        } else {
            err = "Phương thức nén không hỗ trợ: " + name;
            return false;
        }
        if (plain.size() != usize || crc32(plain) != crc) {
            err = "Sai CRC-32 hoặc dung lượng: " + name;
            return false;
        }
        names.push_back(name);
    }
    std::vector<std::string> a = names, b = expectedNames;
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    if (a != b) {
        err = "Danh sách file trong ZIP (" + std::to_string(a.size()) + ") không khớp với danh sách cần có (" +
              std::to_string(b.size()) + ").";
        return false;
    }
    return true;
}

}  // namespace ojgen
