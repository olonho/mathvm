#ifndef BCINTERPRETER_H
#define BCINTERPRETER_H

#include "mathvm.h"
#include "ast.h"

#include <stack>
#include <vector>
#include <string>

using std::stack;
using std::vector;
using std::string;

namespace mathvm {

class InterpreterCodeImpl: public Code {
public:
    InterpreterCodeImpl(): Code() {}
    Status* execute(vector<Var*> &vars);

private:
    Bytecode* bytecode() { return m_current_scope->function->bytecode(); }

    uint32_t& ip() { return m_current_scope->ip; }

    int16_t get_int16() {
        int16_t result = bytecode()->getTyped<int16_t>(ip());
        return result;
    }

    uint16_t get_uint16() {
        uint16_t result = bytecode()->getTyped<uint16_t>(ip());
        ip() += sizeof(uint16_t);
        return result;
    }

    int64_t get_int64() {
        int64_t result = bytecode()->getTyped<int64_t>(ip());
        ip() += sizeof(int64_t);
        return result;
    }

    double get_double() {
        double result = bytecode()->getTyped<double>(ip());
        ip() += sizeof(double);
        return result;
    }

    void double_bin_op(TokenKind op);
    void int_bin_op(TokenKind op);

    void load_var(uint16_t index);
    void load_var(uint16_t cid, uint16_t index);

    void store_var(uint32_t index);
    void store_var(uint16_t cid, uint16_t index);

    void call_function(uint16_t id) {
        BytecodeFunction *f = (BytecodeFunction*)functionById(id);
        m_current_scope = new Scope(f, m_current_scope);
    }

    struct var_holder {
    public:
        var_holder():
            m_content(0)
        {}

        var_holder(var_holder const & other):
            m_content (other.m_content ? other.m_content->clone() : 0)
        {}

        template<typename T>
        var_holder(T const & val):
            m_content(new holder<T>(val))
        {}

        ~var_holder() {
            delete m_content;
        }

        var_holder &operator=(var_holder const & other) {
            var_holder(other).swap(*this);
            return *this;
        }

        template<typename T>
        T get() {
            return static_cast<var_holder::holder<T>*>(m_content)->val;
        }

        double get_double() { return get<double>(); }
        int64_t get_int() { return get<int64_t>(); }
        uint16_t get_uint16() { return get<uint16_t>(); }
        string get_string() { return string(get<char*>()); }

        template<typename T>
        T* getPtr() {
            return &static_cast<var_holder::holder<T>*>(m_content)->val;
        }

        char* get_string_ptr() { return *getPtr<char*>(); }

        void swap(var_holder& other) { std::swap(m_content, other.m_content); }

    private:
        struct container {
            virtual ~container() { }
            virtual container *clone() const = 0;
        };

        template<class T>
        struct holder: public container {
            holder(T v):
                container(),
                val(v)
            {}
            T val;

            container* clone() const {
                return new holder(val);
            }
        };

        container* m_content;
    };

    struct Scope {
        Scope(BytecodeFunction* f, Scope* p = 0):
            ip(0),
            parent(p),
            function(f),
            vars(f->localsNumber() + f->parametersNumber(), var_holder(0))
        {}

        uint32_t ip;
        Scope* parent;
        BytecodeFunction* function;
        vector<var_holder> vars;
    };

    var_holder pop_stack() {
        if(m_stack.empty()) throw std::string("Empty stack detected");
        var_holder res = m_stack.top();
        m_stack.pop();
        return res;
    }

    Scope* m_current_scope;
    stack<var_holder> m_stack;
};

}

#endif // BCINTERPRETER_H
