#ifndef BCTRANSLATOR_H
#define BCTRANSLATOR_H

#include "mathvm.h"
#include "ast.h"
#include "interpretationerror.hpp"
#include "logger.hpp"

namespace mathvm {

typedef std::vector<Var> stack_t;
typedef std::vector<std::vector<Var> > vars_t;

class CodeImpl : public Code {
public:
    CodeImpl() {
        m_bc = new Bytecode;
    }

    ~CodeImpl() {
//        if (m_bc)
//            delete m_bc;
//        m_bc = 0;
    }

    Bytecode* bytecode() {
        return m_bc;
    }

    void setBytecode(Bytecode* bc) {
        if (m_bc)
            delete m_bc;
        m_bc = bc;
    }

    virtual Status* execute(std::vector<Var*>& vars);

    virtual void disassemble(std::ostream& out = std::cout, FunctionFilter* f = 0);
private:
    void run(stack_t& stack, vars_t& vars);

    //helper functions
    template<class F>
    void binary_op_d(std::vector<Var>& stack) {
        Var left = stack.back();
        stack.pop_back();
        Var right = stack.back();
        stack.pop_back();
        Var var(VT_DOUBLE, "");
        var.setDoubleValue(F().operator()(left.getDoubleValue(), right.getDoubleValue()));
        stack.push_back(var);
    }

    template<class F>
    void binary_op_i(std::vector<Var>& stack) {
        Var left = stack.back();
        stack.pop_back();
        Var right = stack.back();
        stack.pop_back();
        Var var(VT_INT, "");
        var.setIntValue(F().operator()(left.getIntValue(), right.getIntValue()));
        stack.push_back(var);
    }

    template<class F>
    void unary_op_i(std::vector<Var>& stack) {
        Var v = stack.back();
        stack.pop_back();
        Var var(VT_INT, "");
        var.setIntValue(F().operator()(v.getIntValue()));
        stack.push_back(var);
    }

    template<class F>
    void unary_op_d(std::vector<Var>& stack) {
        Var v = stack.back();
        stack.pop_back();
        Var var(VT_DOUBLE, "");
        var.setDoubleValue(F().operator()(v.getDoubleValue()));
        stack.push_back(var);
    }

    template<class F>
    bool run_if(stack_t& stack, uint32_t ip) {
        Var vl = stack.back();
        stack.pop_back();
        Var vr = stack.back();
        stack.pop_back();
        return F().operator()(vr.getIntValue(), vl.getIntValue());
    }


    void storevar(stack_t& stack, vars_t& vars, uint16_t id) {
        while(vars.size() <= id)
            vars.push_back(vector<Var>());
        vector<Var>& varvec = vars[id];
        if(varvec.empty()) {
            varvec.push_back(stack.back());
        } else
            varvec.back() = stack.back();
        stack.pop_back();
    }

    Bytecode* m_bc;
};

struct PlusD {
    double operator() (double a, double b) {
        return a+b;
    }
};

struct PlusI {
    int64_t operator() (int64_t a, int64_t b) {
        return a+b;
    }
};

struct SubD {
    double operator() (double a, double b) {
        return a-b;
    }
};

struct SubI {
    int64_t operator() (int64_t a, int64_t b) {
        return a-b;
    }
};

struct MulD {
    double operator() (double a, double b) {
        return a*b;
    }
};

struct MulI {
    int64_t operator() (int64_t a, int64_t b) {
        return a*b;
    }
};

struct OrI {
    int64_t operator() (int64_t a, int64_t b) {
        return a | b;
    }
};

struct AndI {
    int64_t operator() (int64_t a, int64_t b) {
        return a & b;
    }
};

struct XorI {
    int64_t operator() (int64_t a, int64_t b) {
        return a ^ b;
    }
};

struct DivD {
    double operator() (double a, double b) {
        return a / b;
    }
};

struct DivI {
    int64_t operator() (int64_t a, int64_t b) {
        return a / b;
    }
};

struct NegI {
    int64_t operator() (int64_t a) {
        if (a == 0)
            return 1;
        return -a;
    }
};

struct NegD {
    double operator() (double a) {
        return -a;
    }
};

struct EqB {
    bool operator() (int64_t a, int64_t b) {
        return a == b;
    }
};

struct NeqB {
    bool operator() (int64_t a, int64_t b) {
        return a != b;
    }
};

struct GtB {
    bool operator() (int64_t a, int64_t b) {
        return a > b;
    }
};

struct GeB {
    bool operator() (int64_t a, int64_t b) {
        return a >= b;
    }
};

struct LtB {
    bool operator() (int64_t a, int64_t b) {
        return a < b;
    }
};

struct LeB {
    bool operator() (int64_t a, int64_t b) {
        return a <= b;
    }
};


} //namespace

#endif
