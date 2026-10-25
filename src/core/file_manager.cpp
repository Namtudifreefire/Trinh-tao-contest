// file_manager.cpp - Thao tác file/thư mục.
#include "file_manager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <system_error>

namespace ojgen {

fs::path u8path(const std::string& utf8) { return fs::u8path(utf8); }

std::string pathStr(const fs::path& p) { return p.u8string(); }

bool readFile(const fs::path& p, std::string& out, std::string* err) {
    std::ifstream f(p, std::ios::binary);
    if (!f) {
        if (err) *err = "Không mở được file để đọc: " + pathStr(p);
        return false;
    }
    f.seekg(0, std::ios::end);
    std::streamoff sz = f.tellg();
    f.seekg(0, std::ios::beg);
    out.assign(static_cast<size_t>(sz > 0 ? sz : 0), '\0');
    if (sz > 0) f.read(&out[0], sz);
    if (!f && sz > 0) {
        if (err) *err = "Lỗi khi đọc file: " + pathStr(p);
        return false;
    }
    return true;
}

bool writeFile(const fs::path& p, const std::string& data, std::string* err) {
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) {
        if (err) *err = "Không tạo được file: " + pathStr(p);
        return false;
    }
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
    f.close();
    if (!f) {
        if (err) *err = "Lỗi khi ghi file (ổ đĩa đầy?): " + pathStr(p);
        return false;
    }
    return true;
}

bool moveFile(const fs::path& from, const fs::path& to, std::string* err) {
    std::error_code ec;
    fs::remove(to, ec);
    ec.clear();
    fs::rename(from, to, ec);
    if (!ec) return true;
    ec.clear();
    fs::copy_file(from, to, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        if (err) *err = "Không chuyển được file tới " + pathStr(to) + ": " + ec.message();
        return false;
    }
    fs::remove(from, ec);
    return true;
}

int testNumberWidth(int testCount) {
    int w = static_cast<int>(std::to_string(testCount).size());
    return w < 2 ? 2 : w;
}

std::string testName(const std::string& prefix, int index, int width) {
    std::string num = std::to_string(index);
    if (static_cast<int>(num.size()) < width) num = std::string(static_cast<size_t>(width) - num.size(), '0') + num;
    return prefix + num;
}

// So sánh tự nhiên: tách phần số để test2 < test10.
static bool naturalLess(const std::string& a, const std::string& b) {
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (std::isdigit(static_cast<unsigned char>(a[i])) && std::isdigit(static_cast<unsigned char>(b[j]))) {
            size_t i2 = i, j2 = j;
            while (i2 < a.size() && std::isdigit(static_cast<unsigned char>(a[i2]))) ++i2;
            while (j2 < b.size() && std::isdigit(static_cast<unsigned char>(b[j2]))) ++j2;
            std::string na = a.substr(i, i2 - i), nb = b.substr(j, j2 - j);
            na.erase(0, std::min(na.find_first_not_of('0'), na.size()));
            nb.erase(0, std::min(nb.find_first_not_of('0'), nb.size()));
            if (na.size() != nb.size()) return na.size() < nb.size();
            if (na != nb) return na < nb;
            i = i2, j = j2;
        } else {
            if (a[i] != b[j]) return a[i] < b[j];
            ++i, ++j;
        }
    }
    return a.size() - i < b.size() - j;
}

std::vector<fs::path> listFilesWithExt(const fs::path& dir, const std::string& ext) {
    std::vector<fs::path> res;
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return res;
    for (const auto& e : fs::directory_iterator(dir, ec)) {
        std::error_code ec2;
        if (!e.is_regular_file(ec2)) continue;
        std::string x = e.path().extension().u8string();
        std::transform(x.begin(), x.end(), x.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (x == ext) res.push_back(e.path());
    }
    std::sort(res.begin(), res.end(), [](const fs::path& a, const fs::path& b) {
        return naturalLess(a.filename().u8string(), b.filename().u8string());
    });
    return res;
}

size_t countTestFiles(const fs::path& dir) {
    return listFilesWithExt(dir, ".inp").size() + listFilesWithExt(dir, ".out").size();
}

long clearTestFiles(const fs::path& dir, std::string* err) {
    long removed = 0;
    std::vector<fs::path> files = listFilesWithExt(dir, ".inp");
    auto outs = listFilesWithExt(dir, ".out");
    files.insert(files.end(), outs.begin(), outs.end());
    std::error_code ec;
    if (fs::exists(dir / kInfoFile, ec)) files.push_back(dir / kInfoFile);
    for (const auto& f : files) {
        ec.clear();
        if (!fs::remove(f, ec) || ec) {
            if (err) *err = "Không xóa được " + pathStr(f) + (ec ? ": " + ec.message() : "");
            return -1;
        }
        ++removed;
    }
    return removed;
}

fs::path uniqueDir(const fs::path& base) {
    auto taken = [](const fs::path& d) {
        std::error_code ec;
        fs::path z = d;
        z += ".zip";
        return fs::exists(d, ec) || fs::exists(z, ec);
    };
    if (!taken(base)) return base;
    for (int k = 2;; ++k) {
        fs::path cand = base;
        cand += "_" + std::to_string(k);
        if (!taken(cand)) return cand;
    }
}

}  // namespace ojgen
