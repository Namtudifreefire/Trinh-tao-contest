// problem_list.h - Khai báo hàm tạo của từng module.
// Thêm module mới: khai báo hàm make...() ở đây và thêm 1 dòng vào
// src/problems/registry.cpp (xem templates/new_problem.cpp.txt).
#pragma once

#include <memory>

#include "problem.h"

namespace ojgen {

std::unique_ptr<Problem> makeSingleN();
std::unique_ptr<Problem> makeTwoInts();
std::unique_ptr<Problem> makeArray();
std::unique_ptr<Problem> makeQueries();
std::unique_ptr<Problem> makeString();
std::unique_ptr<Problem> makeCustom();
std::unique_ptr<Problem> makeGraph();

}  // namespace ojgen
