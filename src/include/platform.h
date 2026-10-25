// platform.h - Chuyển đổi UTF-8 <-> UTF-16 cho Windows API.
#pragma once

#include <string>

#ifdef _WIN32
namespace ojgen::platform {
std::wstring widen(const std::string& utf8);
std::string narrow(const std::wstring& wide);
}  // namespace ojgen::platform
#endif
