#pragma once
#include <mathvm.h>
#include <ast.h>
#include <vector>

class Executable {
    typedef std::map<uint16_t, BytecodeFunction*> IdToFunction;
    typedef std::map<uint16_t, BytecodeFunction*> FunctionToId;
    IdToFunction idToFunction;
    FunctionToId functionToId;
public:
    Executable();
    virtual mathvm::Status* execute(std::vector<mathvm::Var*, std::allocator<mathvm::Var*> >& vars);
    void addFunc(uint16_t id, mathvm::BytecodeFunction* func) { idToFunction[id] = func; functionToId[func] = id;  }
    mathvm::BytecodeFunction* funcById(uint16_t id) { return idToFunction[id]; }
    mathvm::BytecodeFunction* idByFunc(mathvm::TranslatedFunction* func) { return functionToId[func]; }
    virtual void disassemble(std::ostream& out = std::cout, mathvm::FunctionFilter *f = 0);
};
