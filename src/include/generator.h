// generator.h - Sinh các file .inp và điều phối toàn bộ quy trình tạo bộ test.
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "file_manager.h"
#include "problem.h"
#include "settings.h"

namespace ojgen {

struct GenerateRequest {
    const Problem* problem = nullptr;
    Config cfg;
    int testCount = 0;
    ull seed = 0;
    fs::path outDir;
    std::string prefix = "test";
    bool verbose = true;
};

struct GenerateResult {
    bool ok = false;
    std::string error;
    std::vector<fs::path> inputs;
    std::vector<std::string> warnings;
};

constexpr int kMaxTests = 10000;

// Kiểm tra cấu hình (miền từng tham số + logic của module).
Error validateFullConfig(const Problem& p, const Config& cfg);

// Sinh testCount file input. Mỗi file được module xác thực lại ngay sau khi sinh;
// sinh lại (tối đa vài lần) nếu trùng một test trước đó.
GenerateResult generateInputs(const GenerateRequest& req);

ull randomSeed();

// ---- Quy trình hoàn chỉnh --------------------------------------------------
enum class ZipMode { Ask, Yes, No };

struct JobSpec {
    const Problem* problem = nullptr;
    Config cfg;
    int testCount = 10;
    std::optional<ull> seed;
    std::string solutionPath;  // rỗng = không tạo output
    fs::path outDir;           // thư mục chứa test (đã xử lý trùng tên)
    ZipMode zip = ZipMode::Ask;
    bool zipIncomplete = false;  // vẫn nén khi bộ test chưa hoàn chỉnh
    bool forceRebuild = false;
    bool verbose = true;
    std::string genStepTitle;     // tiêu đề bước (menu), rỗng = không in
    std::string exportStepTitle;
};

// Trả về mã thoát: 0 = hoàn chỉnh hoặc chỉ input (đúng yêu cầu), 1 = chưa hoàn
// chỉnh (lời giải lỗi), 2 = lỗi cấu hình/ghi file.
int runJob(const JobSpec& job, Settings& st);

// Chạy lời giải cho thư mục test có sẵn (tạo lại toàn bộ .out).
int regenerateOutputs(const fs::path& dir, const std::string& solution, Settings& st, ZipMode zip,
                      bool forceRebuild, bool zipIncomplete);

// Đóng gói ZIP một thư mục test có sẵn (.inp và .out). Trả về true nếu thành công.
bool zipTestFolder(const fs::path& dir, const Settings& st, bool verbose);

// Chuẩn bị thư mục chứa test khi thư mục (hoặc file .zip cùng tên) đã có dữ liệu.
enum class ExistingPolicy { Ask, Overwrite, NewDir };
// Có thể đổi dir (NewDir). Trả về false nếu người dùng hủy hoặc lỗi.
bool prepareOutDir(fs::path& dir, ExistingPolicy policy, std::string& err);

// Thư mục làm việc tạm của ojgen (chứa chương trình đã biên dịch, file tạm).
fs::path workDirFor(const Settings& st);

}  // namespace ojgen
