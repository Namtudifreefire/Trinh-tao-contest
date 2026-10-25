// cli.h - Giao diện dòng lệnh, menu tương tác và tự kiểm thử.
#pragma once

#include <string>
#include <vector>

namespace ojgen {

int runCli(const std::vector<std::string>& args);  // khi có tham số dòng lệnh
int runMenu();                                       // khi chạy không tham số
int runSelfTest();                                   // ojgen --selftest
std::string usageText();
std::string guideText();

}  // namespace ojgen
