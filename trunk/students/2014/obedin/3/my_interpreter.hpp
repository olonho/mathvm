#ifndef MY_INTERPRETER_HPP
#define MY_INTERPRETER_HPP

#include <iostream>
#include <stack>
#include <ast.h>
#include <visitors.h>
#include <mathvm.h>
#include <parser.h>

#include "macros.hpp"

using namespace mathvm;


typedef uint64_t IP;

class IScope {
public:
    BytecodeFunction *fn;
    IScope *parent;
    IP ip;

    IScope(BytecodeFunction *fn, IScope *parent = NULL)
        : fn(fn), parent(parent), ip(0)
        {}
};

class StackItem {
public:
    StackItem(VarType t, int64_t i = 0, double d = 0.0, const string &s = "")
        : m_i(i), m_d(d), m_s(s), m_t(t)
        {}

    template<typename T>
    T as();

    VarType type()
        { return m_t; }

private:
    int64_t m_i;
    double  m_d;
    string  m_s;
    VarType m_t;
};

class ICode: public Code {
public:
    Status *execute(std::vector<Var *> &vars);

private:
    IScope *m_curScope;
    std::stack<StackItem> m_stack;

    Bytecode *bc()
        { return m_curScope->fn->bytecode(); }
    IP &ip()
        { return m_curScope->ip; }

    void stackPush(const StackItem &x)
        { m_stack.push(x); }

    StackItem stackPop()
        {
            StackItem val = m_stack.top();
            m_stack.pop();
            return val;
        }

    void stackSwap()
        {
            StackItem a = stackPop();
            StackItem b = stackPop();
            stackPush(a);
            stackPush(b);
        }

    template<typename T>
    T next();

    template<typename T>
    void doNumeric(TokenKind op);

    template<typename T>
    void doComaprison();

    void doBitwise(TokenKind op);
    void doCmpAndGo(TokenKind op);
};

#endif /* end of include guard: MY_INTERPRETER_HPP */
