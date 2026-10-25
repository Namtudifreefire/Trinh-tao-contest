// runner.h - Biên dịch và chạy lời giải chuẩn để tạo file output.
//
// Tiến trình con được tạo trực tiếp bằng API hệ điều hành (CreateProcessW trên
// Windows, fork/execvp trên Linux) với danh sách đối số, KHÔNG qua shell, nên
// đường dẫn có dấu cách/ký tự đặc biệt được truyền nguyên vẹn.
//
// LƯU Ý AN TOÀN: lời giải là code do người dùng cung cấp và chạy trực tiếp trên
// máy với quyền của người dùng. ojgen chỉ giới hạn thời gian, dung lượng output
// (và tùy chọn bộ nhớ); đây KHÔNG phải sandbox. Chỉ chạy code bạn tin tưởng.
#pragma once

#include <string>
#include <vector>

#include "common.h"
#include "file_manager.h"
#include "settings.h"

namespace ojgen {

enum class RunStatus { Ok, RuntimeError, Timeout, OutputLimit, StartFailed };

struct RunLimits {
    int timeoutMs = 2000;
    ull maxOutputBytes = 64ULL << 20;
    ull memoryBytes = 0;  // 0 = không giới hạn
};

struct RunResult {
    RunStatus status = RunStatus::StartFailed;
    long long exitCode = 0;
    long long elapsedMs = 0;
    std::string message;  // mô tả lỗi dễ hiểu
};

// Chạy argv với stdin = inFile (rỗng = thiết bị null), stdout -> outFile,
// stderr -> errFile (có thể trùng outFile). workDir: thư mục làm việc của tiến trình.
RunResult runProcess(const std::vector<std::string>& argv, const fs::path& inFile, const fs::path& outFile,
                     const fs::path& errFile, const fs::path& workDir, const RunLimits& limits);

struct PreparedSolution {
    bool ok = false;
    fs::path exe;         // chương trình sẽ chạy
    bool compiled = false;
    bool cached = false;  // dùng lại bản biên dịch trước (mã nguồn không đổi)
    std::string error;    // lỗi (kèm log biên dịch nếu có)
};

// Kiểm tra file nguồn, biên dịch (nếu là .cpp/.cc/.cxx) vào workDir.
// File .exe (hoặc file thực thi trên Linux) được dùng trực tiếp.
PreparedSolution prepareSolution(const std::string& path, const Settings& st, const fs::path& workDir,
                                 bool forceRebuild, bool verbose);

struct TestFailure {
    int index;            // số thứ tự test (bắt đầu từ 1)
    std::string name;     // vd test07
    std::string reason;
};

struct OutputReport {
    int total = 0;
    int ok = 0;
    std::vector<TestFailure> failures;
    std::vector<std::string> warnings;
    long long maxMs = 0;
};

// Chạy lời giải với từng file .inp, ghi .out tương ứng (cùng tên). File .out chỉ
// được tạo khi tiến trình kết thúc bình thường (mã thoát 0, không quá thời gian,
// không quá dung lượng). Test lỗi sẽ KHÔNG có file .out.
OutputReport produceOutputs(const fs::path& exe, const std::vector<fs::path>& inputs, const Settings& st,
                            const fs::path& workDir, bool verbose);

std::string statusName(RunStatus s);

}  // namespace ojgen
