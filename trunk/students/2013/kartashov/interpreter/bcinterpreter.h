#ifndef BCINTERPRETER_H
#define BCINTERPRETER_H

#include "mathvm.h"
#include "ast.h"
#include <stack>

namespace mathvm {

struct InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl() : Code() {}
    Status *execute(vector<Var *> &vars);

private:
    Bytecode *currentBC() {
        return currentScope->function->bytecode();
    }

    uint32_t &currentIP() {
        return currentScope->ip;
    }

#define GET_FROM_BC(type) \
    type v = currentBC()->getTyped<type>(currentIP()); \
    currentIP() += sizeof(type); \
    return v;

    uint16_t getUInt16() { GET_FROM_BC(uint16_t) }
    double getDouble() { GET_FROM_BC(double) }
    int16_t getInt16() { GET_FROM_BC(int16_t) }
    int64_t getInt64() { GET_FROM_BC(int64_t) }
#undef GET_FROM_BC

    void doubleBinOp(TokenKind op);
    void intBinOp(TokenKind op);

    inline void loadVar(uint16_t index);
    inline void loadVar(uint16_t cid, uint16_t index);
    inline void storeVar(uint32_t index);
    inline void storeVar(uint16_t cid, uint16_t index);

    void callFunction(uint16_t id) {
        BytecodeFunction *f = (BytecodeFunction*)functionById(id);
        currentScope = new IScope(f, currentScope);
    }

    void callNative(uint16_t id);

    //------------------------------------------------------------

    class Any {
    public:
        Any() : content(0) {}
        Any(const Any &other) : content (other.content ? other.content->clone() : 0) {}

        template<typename T>
        Any(const T &val) : content(new Holder<T>(val)) {}

        ~Any() {
            delete content;
        }

        Any &operator=(const Any &other) {
            Any(other).swap(*this);
            return *this;
        }

        template<typename T>
        T get() {
            return static_cast<Any::Holder<T> *>(content)->val;
        }

        double getDouble()      { return get<double>(); }
        int64_t getInt()        { return get<int64_t>(); }
        uint16_t getUInt16()    { return get<uint16_t>(); }
        std::string getString() { return std::string(get<char *>()); }

        template<typename T>
        T *getPtr() {
            return &static_cast<Any::Holder<T> *>(content)->val;
        }

        double *getDoublePtr()      { return getPtr<double>(); }
        int64_t *getIntPtr()        { return getPtr<int64_t>(); }
        uint16_t *getUInt16Ptr()    { return getPtr<uint16_t>(); }
        char *getStringPtr()        { return *getPtr<char *>(); }

        void swap(Any &other) {
            std::swap(content, other.content);
        }

    private:
        class AnyContainer {
        public:
            virtual ~AnyContainer() { }
            virtual AnyContainer *clone() const = 0;
        };

        template<class T>
        struct Holder : public AnyContainer {
            Holder(T v) : AnyContainer(), val(v) {}
            T val;

            AnyContainer *clone() const {
                return new Holder(val);
            }
        };

        AnyContainer *content;
    };

    class IScope {
    public:
        IScope(BytecodeFunction *f, IScope *p = 0) : ip(0), parent(p), function(f), vars(f->localsNumber() + f->parametersNumber()) {
        }

        uint32_t ip;
        IScope *parent;
        BytecodeFunction *function;
        std::vector<Any> vars;
    };

    //------------------------------------------------------------

    Any popStack() {
        if(pstack.empty()) throw std::string("Empty stack detected");
        Any res = pstack.top();
        pstack.pop();
        return res;
    }

    IScope *currentScope;
    std::stack<Any> pstack;
};

}

#endif // BCINTERPRETER_H
