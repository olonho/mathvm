#ifndef BYTECODE_IMPL_HPP_
#define BYTECODE_IMPL_HPP_

#include <map>
#include <string>
#include <iostream>
#include <stack>
#include <vector>
#include <memory>

#include "ast.h"
#include "mathvm.h"
#include "visitors.h"

struct LVar {
    union {
        double d;
        int64_t i;
        const char* s;
    };

    LVar() = default;
    LVar(const LVar&) = default;

    LVar(double val) : d(val) {};
    LVar(int64_t val) : i(val) {};
    LVar(const char* val) : s(val) {};
};

class mathvm::InterpreterCodeImpl : public mathvm::Code
{
    friend struct Executer;
    friend class mathvm::BytecodeTranslatorImpl;

    struct VarID {
        uint16_t id;
        int gen;

        VarID(uint16_t id, int gen): id(id), gen(gen) {}

        bool operator<(VarID rhs) const noexcept {
            return id < rhs.id || (id == rhs.id && gen < rhs.gen);
        }
    };

    std::map<VarID, LVar> vars;

//    std::vector<std::map<uint16_t, LVar>> vars;
    std::map<std::string, uint16_t> varNames;

public:
    void disassemble(std::ostream& out, mathvm::FunctionFilter* filter) override;
    mathvm::Status* execute(std::vector<mathvm::Var*>& vars) override;
};

#endif // BYTECODE_IMPL_HPP_