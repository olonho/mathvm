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

    void set(mathvm::Var& v) {
        using namespace mathvm;
        switch (v.type()) {
            case VT_INT: i = v.getIntValue(); break;
            case VT_DOUBLE: d = v.getDoubleValue(); break;
            case VT_STRING: s = v.getStringValue(); break;
            default: assert(false);
        }
    }

    void propagate(mathvm::Var& v) {
        using namespace mathvm;
        switch (v.type()) {
            case VT_INT: v.setIntValue(i); break;
            case VT_DOUBLE: v.setDoubleValue(d); break;
            case VT_STRING: v.setStringValue(s); break;
            default: assert(false);
        }
    }

    struct VarID {
        uint16_t id;
        int gen;

        VarID(uint16_t id, int gen): id(id), gen(gen) {}

        bool operator<(VarID rhs) const noexcept {
            return id < rhs.id || (id == rhs.id && gen < rhs.gen);
        }
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

    std::map<LVar::VarID, LVar> vars;
    std::map<std::string, uint16_t> varNames;

public:
    void disassemble(std::ostream& out, mathvm::FunctionFilter* filter) override;
    mathvm::Status* execute(std::vector<mathvm::Var*>& vars) override;
};

#endif // BYTECODE_IMPL_HPP_