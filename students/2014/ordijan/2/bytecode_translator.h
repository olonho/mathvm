#ifndef _TRANSLATOR_H
#define _TRANSLATOR_H

#include "mathvm.h"
#include "ast.h"
#include "parser.h"
#include "visitors.h"

#include <stack>

namespace mathvm {

class BytecodeTranslatorVisitor: public AstVisitor {

    /* var->setInfo(descriptor); */
    struct VarDescriptor {
        uint16_t functionId, localIndex;
        bool initialized;
    };

    struct Context {
        AstFunction* currentFunction;
        uint16_t localsNumber;
        Scope* currentScope;
        std::stack<VarType>* typeStack; // validation
    } _context;

    struct ContextSwitcher {
        Context saved;
        BytecodeTranslatorVisitor* self;

        ContextSwitcher(BytecodeTranslatorVisitor* me, Context next):
            saved(me->_context), self(me) {
            self->_context = next;
        }

        ~ContextSwitcher() {
            self->_context = saved;
        }
    };

    InterpreterCodeImpl* _code;
    uint32_t _location;

    static const uint16_t VAR_REGISTERS_NUMBER = 0x04;
    static const uint16_t VAR_TYPES_NUMBER = 0x05;


  public:
    BytecodeTranslatorVisitor(InterpreterCodeImpl* code):
        _context({NULL, 0, NULL, NULL}), _code(code), _location(0) {
    }

    virtual void startWith(AstFunction* f);


  private:
#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
    virtual void visitAstFunction(AstFunction* f);

    void onVisitNode(AstNode* node);
    void visitTyped(AstNode* node, VarType type);

    void declareFunction(AstFunction* f);
    uint16_t getFunctionId(AstFunction* f) const;
    VarDescriptor* getDescriptor(const AstVar* x) const;

    void variableInScope(AstVar* x);
    void variableOutOfScope(AstVar* x);

    VarType topType() const;
    void pushType(VarType type);
    void popType(VarType expected);
    VarType popType();
    void tryCast(VarType to);
    void castTopsToCommonType();

    void warningIf(bool bad, const char* msg) const;
    bool isUnused(AstNode* node) const;

    void checkSignature(CallNode* node, AstFunction* f) const;

    void ensureTypeStackEmpty() const;
    void ensureIsParameterType(VarType type) const;
    void ensureTopType(VarType expected) const;
    void ensureTopIsNumeric() const;

    void loadStore(const AstVar* x, bool load = true);
    void loadStoreLocal(uint16_t id, VarType type, bool load);
    void loadStoreGlobal(const VarDescriptor* descriptor, VarType type, bool load);

    void processLazyLogic(BinaryOpNode* node);
    void processComparison(TokenKind kind);
    void processNumericOperation(TokenKind op);
    void beforeProcessBlock();
    void processBlockNode(BlockNode* node);
    void afterProcessBlock();
    void processNativeCallNode(AstFunction* f);

    void falseJump(AstNode* node, Label& label);

  private:
    AstFunction* function() const {
        return _context.currentFunction;
    }

    Scope* scope() const {
        return _context.currentScope;
    }

    uint16_t& locals() {
        return _context.localsNumber;
    }

    std::stack<VarType>* typeStack() const {
        return _context.typeStack;
    }

    InterpreterCodeImpl* code() const {
        return _code;
    }

    uint32_t location() const {
        return _location;
    }

    Bytecode* bytecode() const {
        return bytecodeFunction()->bytecode();
    }

    uint16_t functionId() const {
        return bytecodeFunction()->id();
    }

    BytecodeFunction* bytecodeFunction() const {
        return (BytecodeFunction*)function()->info();
    }
};

}

#endif // _TRANSLATOR_H
