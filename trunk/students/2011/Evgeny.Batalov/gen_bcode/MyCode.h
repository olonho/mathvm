#pragma once
#include <mathvm.h>
#include <vector>

class MyCode: public mathvm::Code {
    mathvm::Bytecode bytecode;
    std::vector<uint16_t> functionIds;
public:
    virtual mathvm::Status* execute(std::vector<mathvm::Var*> vars);
    const mathvm::Bytecode& getBytecode() const { return bytecode; }
    mathvm::Bytecode& getBytecode() { return bytecode; }
    void addFunctionId(uint16_t id) { functionIds.push_back(id); }
    void dump() const;
};
