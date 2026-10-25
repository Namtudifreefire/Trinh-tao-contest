// rng.h - Bộ sinh số ngẫu nhiên dựa trên std::mt19937_64.
//
// Không dùng std::uniform_int_distribution / std::shuffle vì kết quả của
// chúng phụ thuộc vào thư viện chuẩn (libstdc++, libc++, MSVC khác nhau).
// Các hàm dưới đây tự cài đặt nên CÙNG seed + CÙNG cấu hình luôn cho CÙNG
// bộ test, kể cả khi build bằng trình biên dịch khác.
#pragma once

#include <random>
#include <utility>
#include <vector>

#include "common.h"

namespace ojgen {

class Rng {
public:
    explicit Rng(ull seed) : eng_(seed) {}

    ull next() { return eng_(); }

    // Số nguyên đều trong [0, bound) với bound > 0 (rejection sampling, không lệch).
    ull below(ull bound) {
        if (bound == 0) return next();
        ull limit = ~0ULL - (~0ULL % bound);  // bội lớn nhất của bound
        ull x;
        do x = next(); while (x >= limit);
        return x % bound;
    }

    // Số nguyên đều trong đoạn [lo, hi], an toàn trên toàn bộ phạm vi long long.
    ll range(ll lo, ll hi) {
        if (lo > hi) std::swap(lo, hi);
        ull span = spanOf(lo, hi);
        if (span == ~0ULL) return static_cast<ll>(next());
        ull off = below(span + 1);
        return static_cast<ll>(static_cast<ull>(lo) + off);
    }

    // true với xác suất percent%.
    bool chance(int percent) { return static_cast<int>(below(100)) < percent; }

    template <class T>
    void shuffle(std::vector<T>& v) {
        for (size_t i = v.size(); i > 1; --i) {
            size_t j = static_cast<size_t>(below(i));
            std::swap(v[i - 1], v[j]);
        }
    }

private:
    std::mt19937_64 eng_;
};

}  // namespace ojgen
