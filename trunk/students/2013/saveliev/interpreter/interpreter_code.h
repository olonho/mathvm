#ifndef INTERPRETER_CODE_H
#define INTERPRETER_CODE_H

#include "mathvm.h"
#include "ast.h"

#include <vector>
#include <map>
#include <stack>
#include <iostream>
#include <sstream>
//#include <tuple>


#define DEB 0
#define VRB 0

#if DEB
    #define INFO(msg) { std::cout << msg << std::endl; }
    #define DEBUG(msg) { std::cerr << msg << std::endl; }
    #define SHOW(msg) { std::cerr << #msg << ": " << msg << std::endl; }
#else
    #define INFO(msg)
    #define DEBUG(msg)
    #define SHOW(msg) 
#endif
#if VRB
    #define VERBOSE(msg) { DEBUG(msg) }
#else
    #define VERBOSE(msg)
#endif

namespace mathvm {

class InterpreterCodeImpl: public Code { 
public:
    InterpreterCodeImpl() {}

    virtual ~InterpreterCodeImpl() {}

    virtual Status* execute(std::vector<Var*>& vars);
    
    union Val {
        double double_;
        int64_t int64;
        uint16_t uint16;

        Val() {}
//        val_t(const val_t& v): this(v) {}
        explicit Val(double d): double_(d) {}
        explicit Val(int64_t i): int64(i) {}
        explicit Val(uint16_t s): uint16(s) {}
        
        operator double() { return double_; }
        operator int64_t() { return int64; }
        operator uint16_t() { return uint16; }
        
        friend std::ostream& operator << (std::ostream &os, const Val& v) {
            return os << v.int64;
        }
    };

private:
    class Context {
        Context* _parent;
        uint32_t _bci;
        BytecodeFunction* _function;
        std::vector<Val> _vars;
        
    public:
        Context(Context* parent, BytecodeFunction* func):
            _parent(parent), _bci(0), _function(func),
            _vars(func->parametersNumber() + func->localsNumber()) {
        }
        
        void saveBci(uint32_t bci) { _bci = bci; }
        
        uint32_t bci() const { return _bci; }

        Bytecode* bc() { return _function->bytecode(); }

        Context* parent() { return _parent; }

        uint16_t id() { return _function->id(); }
        
        uint16_t variablesNum() const { return _vars.size(); }

        Val* getVar(uint16_t index) {
            assert(index < _vars.size());
            return &_vars[index]; 
        }        
    };
    Context* _context;
    std::stack<Val> _stack;
    Bytecode* _bc;
    uint32_t _bci;
           
private:
    std::map<uint16_t, char*> pointers;
    uint16_t makePointer(char* ptr);
    char* constantById(uint16_t id);
    
    void push(Val v) {
        _stack.push(v);        
    }
    
    template<typename T>
    void pushTyped(T v) {
        VERBOSE("  push " << v);
        push(Val(v));
    }    
    void pushInt64(int64_t v);
    void pushDouble(double v) { pushTyped(v); }
    void pushUInt16(uint16_t v) { pushTyped(v); }
       
    Val pop() { 
        assert(!_stack.empty());
        Val v = _stack.top();
        _stack.pop();
        return v;
    }
            
    template<typename T>
    T popTyped() {
        assert(!_stack.empty());
        T v = T(_stack.top());
        _stack.pop();
        return v;
    }    
    double popDouble() { return popTyped<double>(); }
    int64_t popInt64() { return popTyped<int64_t>(); }
    uint16_t popUInt16() { 
        uint16_t v = popTyped<uint16_t>();
        VERBOSE("  pop uint16 " << v << ", as string: '" << constantById(v) << "'");
        return v; 
    }
    
    Val top() {
        assert(!_stack.empty());
        return _stack.top();
    }
    
    template<typename T> 
    T readTyped() {
        T val = _bc->getTyped<T>(_bci);
        _bci += sizeof(T);
        VERBOSE("  readTyped " << val << ", sizeof(T) = " << sizeof(T));
        return val;
    }
    
    double readDouble() { return readTyped<double>(); }
    int64_t readInt64() { return readTyped<int64_t>(); }
    uint16_t readUInt16() { return readTyped<uint16_t>(); }
    int16_t readInt16() { return readTyped<int16_t>(); }
    
    Context* findContext(uint16_t ctxId);

    void loadVar(uint16_t varId); 
    void storeVar(uint16_t varId);
    void loadVar(uint16_t varId, Context* context);
    void storeVar(uint16_t varId, Context* context);

    void call(uint16_t funcId);
    void return_();
    void callNative(uint16_t funcId);  

    const std::string valToStringWithType(InterpreterCodeImpl::Val val, VarType type) {
        if (type == VT_STRING)
            return constantById((uint16_t) val);
        
        std::stringstream ss;
        switch (type) {
            case VT_INT: ss << (int64_t) val; break;
            case VT_DOUBLE: ss << (double) val; break;
            default: assert(false); break;
        }
        return ss.str();
    } 
};

static const struct {
    const char* name;
    Instruction insn;
    size_t length;
} instructions[] = {
    #define BC_NAME(b, d, l) {#b, BC_##b, l},
    FOR_BYTECODES(BC_NAME)
};

#define insnName(insn) instructions[insn].name
#define insnLength(insn) instructions[insn].length

void printInsn(Bytecode* bc, size_t bci, ostream& out = cerr, int indent = 0);

inline void InterpreterCodeImpl::pushInt64(int64_t v) {
    pushTyped(v);
}

}

#endif /* INTERPRETER_CODE_H */
