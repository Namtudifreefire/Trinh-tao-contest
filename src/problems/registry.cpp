// registry.cpp - Danh sách module. Thứ tự ở đây = thứ tự hiển thị trong --list.
#include "problem_list.h"

namespace ojgen {

const std::vector<std::unique_ptr<Problem>>& allProblems() {
    static const std::vector<std::unique_ptr<Problem>> list = [] {
        std::vector<std::unique_ptr<Problem>> v;
        v.push_back(makeSingleN());
        v.push_back(makeTwoInts());
        v.push_back(makeArray());
        v.push_back(makeQueries());
        v.push_back(makeString());
        v.push_back(makeCustom());
        v.push_back(makeGraph());
        // v.push_back(makeMyProblem());   // <- thêm module mới tại đây
        return v;
    }();
    return list;
}

const Problem* findProblem(const std::string& id) {
    for (const auto& p : allProblems())
        if (p->id() == id) return p.get();
    return nullptr;
}

}  // namespace ojgen
