#ifndef _BYTECODE_TRANSLATOR_IMPL_H
#define _BYTECODE_TRANSLATOR_IMPL_H

#include "visitors.h"
#include "parser.h"
#include "mathvm.h"
#include <stack>
#include "code_impl.h"

namespace mathvm {

class QBytecodeTranslatorImpl : public Translator {
  public:
    QBytecodeTranslatorImpl() = default;
    virtual ~QBytecodeTranslatorImpl() = default;

    void processFunctions(Scope *scope, CodeImpl& code);

    virtual Status* translate(const string& program, Code* *code);
};


class BytecodeVisitor : public AstVisitor {
    CodeImpl& _code;
    std::stack<BytecodeFunction*> _stack;
    std::stack<VarType> _type;

  private:
    Bytecode* bytecode();
    VarType returnType();
    uint16_t functionId();

  public:
    BytecodeVisitor(CodeImpl& code);
    virtual ~BytecodeVisitor() = default;

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}


#endif // _BYTECODE_TRANSLATOR_IMPL_H
