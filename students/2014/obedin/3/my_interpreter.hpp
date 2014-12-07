#ifndef MY_INTERPRETER_HPP
#define MY_INTERPRETER_HPP

#include <iostream>
#include <stack>
#include <exception>
#include <ast.h>
#include <visitors.h>
#include <mathvm.h>
#include <parser.h>

#include "macros.hpp"

using namespace mathvm;


typedef uint64_t IP;
typedef uint16_t ID;

class StackItem {
public:
    StackItem(VarType t = VT_INVALID, int64_t i = 0, double d = 0.0, const char *s = "")
        : type(t)
        {
            if (type == VT_INT)
                m_i = i;
            else if (type == VT_DOUBLE)
                m_d = d;
            else if (type == VT_STRING)
                m_s = s;
        }

    template<typename T>
    T as();

    VarType type;
    union {
        int64_t m_i;
        double  m_d;
        const char *m_s;
    };
};

class IScope {
public:
    BytecodeFunction *fn;
    IScope *parent;
    IP ip;
    std::vector<StackItem> vars;

    IScope(BytecodeFunction *fn, IScope *parent = NULL)
        : fn(fn), parent(parent), ip(0), vars(fn->localsNumber())
        {}

    inline ID id()
        { return fn->id(); }

    inline StackItem &findVar(ID id)
        {
            if (id >= vars.size())
                throw std::runtime_error(MSG_VAR_NOT_FOUND);
            return vars[id];
        }

    inline StackItem &findCtxVar(ID ctx, ID id)
        {
            IScope *scope = this;
            while (scope && scope->id() != ctx)
                scope = scope->parent;
            if (scope == NULL)
                throw std::runtime_error(MSG_CTX_NOT_FOUND);
            return scope->findVar(id);
        }
};

class ICode: public Code {
public:
    Status *execute(std::vector<Var *> &vars);

private:
    IScope *m_curScope;
    std::stack<StackItem, std::vector<StackItem> > m_stack;

    Bytecode *bc()
        { return m_curScope->fn->bytecode(); }
    IP &ip()
        { return m_curScope->ip; }
    StackItem &localVar(ID id)
        { return m_curScope->findVar(id); }
    StackItem &ctxVar(ID ctx, ID id)
        { return m_curScope->findCtxVar(ctx, id); }

    void stackPush(const StackItem &x)
        { m_stack.push(x); }

    StackItem stackPop()
        {
            StackItem val = m_stack.top();
            m_stack.pop();
            return val;
        }

    template<typename T>
    T next();

    template<typename T>
    void doNumeric(TokenKind op);

    template<typename T>
    void doComparison();

    void doBitwise(TokenKind op);
    void doCmpAndGo(TokenKind op);

    void doCallFunction(ID id);
    void doCallNativeFunction(ID id);
};

#endif /* end of include guard: MY_INTERPRETER_HPP */
