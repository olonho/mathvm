#pragma once

#include <mathvm.h>
#include <visitors.h>

#include <asmjit/asmjit.h>

#include "preprocessor.h"
#include "bc_interpreter.h"
#include "bc_jit.h"

#include <map>
#include <stack>

namespace mathvm {

class BytecodeGenerator : public AstVisitor {
public:
    BytecodeGenerator(InterpreterCodeImpl* code, Preprocessor* preprocessor)
        : _code(code)
        , _preprocessor(preprocessor)
    {}
    ~BytecodeGenerator() override {}

    void processFunction(AstFunction* function, bool topFunction = false);

private:
    InterpreterCodeImpl* _code;
    Preprocessor* _preprocessor;

    stack<BytecodeFunction*> _functions;
    map<const AstVar*, uint16_t> _var2scope;
    map<const AstVar*, uint16_t> _var2id;

    Bytecode* bc();
    void loadVar(const AstVar* var);
    void storeVar(const AstVar* var);

    uint16_t getScopeId(const AstVar* var);
    uint16_t getVarId(const AstVar* var);

    void preProcessFunctions(Scope* scope);

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

struct CodeGenError : public ErrorInfoHolder {};

}
