// problem.h - Giao diện thống nhất cho mọi module (dạng bài) sinh test.
//
// Mỗi module cài đặt lớp con của Problem. Core chỉ làm việc qua giao diện này:
//   params()          -> danh sách tham số (dùng cho menu hỏi đáp và CLI --key value)
//   validateConfig()  -> kiểm tra cấu hình trước khi sinh
//   generate()        -> sinh nội dung MỘT file input
//   validateInput()   -> kiểm tra lại file input đúng định dạng + đúng giới hạn
#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "common.h"
#include "rng.h"

namespace ojgen {

enum class ParamType { Int, Bool, Text };

struct ParamSpec {
    std::string key;      // tên dùng trên CLI: --key value
    std::string label;    // câu hỏi trên menu
    ParamType type = ParamType::Int;
    ll def = 0;           // mặc định (Int/Bool)
    ll lo = 0, hi = 0;    // miền cho phép (Int)
    std::string defText;  // mặc định (Text)
    std::string help;     // giải thích thêm
};

struct Config {
    std::map<std::string, ll> ints;
    std::map<std::string, std::string> texts;

    ll i(const std::string& k) const;
    bool b(const std::string& k) const { return i(k) != 0; }
    std::string t(const std::string& k) const;
    void set(const std::string& k, ll v) { ints[k] = v; }
    void setText(const std::string& k, const std::string& v) { texts[k] = v; }
};

class Problem {
public:
    virtual ~Problem() = default;

    virtual std::string id() const = 0;           // vd "array"
    virtual std::string title() const = 0;        // vd "Mảng một chiều"
    virtual std::string formatDescription() const = 0;  // mô tả định dạng input
    virtual std::vector<ParamSpec> params() const = 0;

    // Cấu hình mặc định lấy từ params().
    virtual Config defaultConfig() const;

    // Hỏi cấu hình trên menu. Mặc định: hỏi lần lượt từng tham số trong params().
    // Module có quy trình riêng (vd custom) ghi đè hàm này.
    virtual void configureInteractive(Config& cfg) const;
    // Số "bước" mà configureInteractive chiếm trong quy trình menu (để đánh số bước).
    virtual int interactiveSteps() const { return 1; }

    // Hoàn thiện cấu hình từ CLI (vd suy ra n từ --bounds). explicitKeys là các
    // tham số người dùng đã truyền.
    virtual void finalizeCliConfig(Config& cfg, const std::set<std::string>& explicitKeys) const;

    // Kiểm tra logic giữa các tham số (min <= max, đủ giá trị khác nhau...).
    // Miền [lo, hi] của từng tham số đã được core kiểm tra trước.
    virtual Error validateConfig(const Config& cfg) const = 0;

    // Sinh nội dung một file input (kết thúc bằng '\n', xuống dòng kiểu LF).
    virtual std::string generate(Rng& rng, const Config& cfg) const = 0;

    // Xác thực một file input theo đúng định dạng và giới hạn của cấu hình.
    virtual Error validateInput(const std::string& input, const Config& cfg) const = 0;

    // Ước lượng dung lượng tối đa của một file input (byte), 0 = không rõ.
    virtual ull estimateMaxBytes(const Config&) const { return 0; }

    // Mô tả cấu hình (ghi vào file thông tin và hiển thị cho người dùng).
    virtual std::string describeConfig(const Config& cfg) const;
};

// Kiểm tra miền giá trị của từng tham số theo ParamSpec.
Error checkParamRanges(const Problem& p, const Config& cfg);

// Hỏi một tham số theo ParamSpec (dùng chung cho menu).
void askParam(const ParamSpec& ps, Config& cfg);

// ---- Hỗ trợ xác thực input -------------------------------------------------
// Tách input thành các dòng, mỗi dòng là danh sách token. Bắt buộc:
//  - kết thúc bằng '\n', không có '\r', không có dòng trống,
//  - các token cách nhau đúng MỘT dấu cách, không có khoảng trắng đầu/cuối dòng.
Error splitInputLines(const std::string& input, std::vector<std::vector<std::string>>& lines);
// Đọc token số nguyên và kiểm tra nằm trong [lo, hi].
Error readIntToken(const std::string& tok, ll lo, ll hi, const std::string& what, ll& out);

// Sinh k giá trị đôi một khác nhau trong [lo, hi] (yêu cầu hi - lo + 1 >= k).
std::vector<ll> distinctValues(Rng& rng, ll lo, ll hi, size_t k);

// ---- Danh sách module -------------------------------------------------------
const std::vector<std::unique_ptr<Problem>>& allProblems();
const Problem* findProblem(const std::string& id);

}  // namespace ojgen
