#ifndef MACHCODE_H
#define MACHCODE_H

#include "mathvm.h"
#include "AsmJit/AsmJit.h"

#include "codeanalyzer.h"

#include <stack>
#include <stdexcept>
#include <cstdio>

struct AsmJitVar {
    AsmJitVar() : gp(0), type(mathvm::VT_INVALID) {}
    AsmJitVar(mathvm::VarType t, AsmJit::GPVar *g = 0) : gp(g), type(t) {}
    AsmJitVar(mathvm::VarType t, AsmJit::XMMVar *x = 0) : xmm(x), type(t) {}

    union {
        AsmJit::GPVar *gp;
        AsmJit::XMMVar *xmm;
    };

    mathvm::VarType type;
};

typedef std::stack<AsmJitVar> ASJStack;

//--------------------------------------------------------------------------------

namespace mathvm {

class AsmJitFunction : public TranslatedFunction {
    ASJStack _localStack;
    AsmJit::Compiler _c;
    AsmJit::FileLogger _logger;
    AsmJit::Label _retLabel;
    AsmJitVar _retVar;
    AsmJitVar _intPrintBuffer, _intVarBuffer, _intLiteralBuffer;
    uint32_t _varsToStore;

public:
    AsmJitFunction(AstFunction *f, bool dbg = false) : TranslatedFunction(f), _logger(stdout), _retLabel(_c.newLabel()), _varsToStore(0) {
        if(dbg) _c.setLogger(&_logger);
    }

    AsmJitVar &retVar() {
        return _retVar;
    }

    AsmJit::Label &retLabel() {
        return _retLabel;
    }

    AsmJitVar &intPrintBuffer() {
        return _intPrintBuffer;
    }

    AsmJitVar &intVarBuffer() {
        return _intVarBuffer;
    }

    AsmJitVar &intLiteralBuffer() {
        return _intLiteralBuffer;
    }

    AsmJit::Compiler &code() {
        return _c;
    }

    ASJStack &stack() {
        return _localStack;
    }

    void setVarsToStore(uint32_t vts) {
        _varsToStore = vts;
    }

    uint32_t varsToStore() const {
        return _varsToStore;
    }

    virtual void disassemble(ostream& out) const {}
};

class AsmJitCodeImpl : public Code {

    typedef void (*VoidFunc)(void*, void*);
    typedef int64_t (*IntFunc)(void*, void*);
    typedef double (*DoubleFunc)(void*, void*);

    std::vector<void*> funcPtrs;

public:
    AsmJitCodeImpl() { }
    virtual ~AsmJitCodeImpl() {}

    virtual Status* execute(vector<Var*>& vars);

    AsmJitFunction* functionByName(const string& name) {
        return dynamic_cast<AsmJitFunction*>(Code::functionByName(name));
    }

    AsmJitFunction* functionById(uint16_t id) {
        return dynamic_cast<AsmJitFunction*>(Code::functionById(id));
    }

    std::vector<void*> &getFuncPtrs() {
        return funcPtrs;
    }

    void *getFunctionAddress(uint16_t id) {
        if(id >= funcPtrs.size()) throw std::logic_error("compiled function not found");
        return funcPtrs[id];
    }

    void setCompiled(uint16_t id, void *code) {
        funcPtrs[id] = code;
    }
};

}

#endif // MACHCODE_H
