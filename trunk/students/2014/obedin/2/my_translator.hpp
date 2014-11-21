#ifndef MY_TRANSLATOR_HPP
#define MY_TRANSLATOR_HPP

#include <iostream>
#include <stack>
#include <vector>
#include <ast.h>
#include <visitors.h>
#include <mathvm.h>
#include <parser.h>
#include <dlfcn.h>

#include "macros.hpp"

using namespace mathvm;


typedef uint16_t Id;

struct TVar {
    Id id;
    Id contextId;
    TVar(Id id, Id contextId)
        : id(id), contextId(contextId)
        {}
};

class TScope {
public:
    BytecodeFunction *fn;
    TScope *parent;
    std::map<const AstVar *, Id> vars;

    TScope(BytecodeFunction *fn, TScope *parent = NULL)
        : fn(fn), parent(parent)
        {}

    Id id()
        { return fn->id(); }

    void addVar(const AstVar *var);
    TVar findVar(const AstVar *var);
};

class TVisitor: public AstVisitor {
public:
    TVisitor();
    Status *visit(Code *code, AstFunction *top);

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    Code *m_code;
    std::vector<VarType> m_stack;
    TScope *m_curScope;
    std::stack<uint32_t> m_sourcePos;

    Bytecode *bc()
        {
            return m_curScope->fn->bytecode();
        }
    void swapTos()
        {
            bc()->addInsn(BC_SWAP);
            std::swap(m_stack.back(), m_stack[m_stack.size()-2]);
        }
    VarType tosType()
        {
            return m_stack.empty() ? VT_INVALID : m_stack.back();
        }
    VarType stackPop()
        {
            VarType r = tosType();
            m_stack.pop_back();
            return r;
        }
    void stackPush(VarType t)
        {
            m_stack.push_back(t);
        }

private:
    inline void castTos(VarType to);
    inline void booleanizeTos();

    void genBooleanOp(TokenKind op);
    void genBitwiseOp(TokenKind op);
    void genComparisonOp(TokenKind op);
    void genNumericOp(TokenKind op);
    void castTosAndPrevToSameNumType();

    void loadVar(const AstVar *astVar);
    void storeVar(const AstVar *astVar, bool checkTos = true);

    void initVars(Scope *scope);
    void initFunctions(Scope *scope);
    void genBlock(BlockNode *node);
};

class TCode: public Code {
public:
    Status *execute(std::vector<Var *> &)
    {
        disassemble();
        return Status::Ok();
    }
};

class MyTranslator: public Translator {
public:
    virtual Status* translate(const string& program, Code** code) {
        Parser parser;
        Status* status = parser.parseProgram(program);
        if (status->isError())
            return status;

        TVisitor visitor;
        TCode c;
        status = visitor.visit(&c, parser.top());
        if (status->isError())
            return status;

        std::vector<Var*> empty;
        return c.execute(empty);
    }
};


#endif /* end of include guard: MY_TRANSLATOR_HPP */
