#pragma once
#include <mathvm.h>
#include <ast.h>
#include <vector>

class MyCode: public mathvm::Code {
    std::vector<uint16_t> functionIds;
public:
    MyCode();
    virtual mathvm::Status* execute(std::vector<mathvm::Var*> vars);
    void addFunctionId(uint16_t id) { functionIds.push_back(id); }
    uint16_t funcIdByIndex(size_t idx) { return functionIds[idx]; }
    size_t funcCount() { return functionIds.size(); }
    void dump() const;
};
