#ifndef INTERPRETER_CODE_H
#define INTERPRETER_CODE_H

#include "mathvm.h"
#include "ast.h"

#include <vector>
#include <stack>
#include <iostream>
    
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


class InterpreterCodeImpl: public Code { 
public:
    InterpreterCodeImpl() {}

    virtual ~InterpreterCodeImpl() {}

    virtual Status* execute(std::vector<Var*>& vars);
    
private:
    struct val_t {
        union {
            double _double;
            int64_t _int;
            uint16_t _stringId;
        };        
        val_t() {}
        val_t(const val_t& sv): _int(sv._int) {}
        explicit val_t(double d): _double(d) {}
        explicit val_t(int64_t i): _int(i) {}
        explicit val_t(uint16_t s): _stringId(s) {}
        
        operator double() { return _double; }
        operator int64_t() { return _int; }
        operator uint16_t() { return _stringId; }
        
        friend std::ostream& operator << (std::ostream &os, const val_t& v) {
            return os << v._int;
        }
    };

    class Context {
        Context* _parent;
        uint32_t _bci;
        BytecodeFunction* _function;
        std::vector<val_t> _vars;
        
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

        val_t* getVar(uint16_t index) {
            assert(index < _vars.size());
            return &_vars[index]; 
        }        
    };
    Context* _context;
    std::stack<val_t> _stack;
    Bytecode* _bc;
    uint32_t _bci;
           
private:    
    void push(val_t v) {
        _stack.push(v);        
    }
    
    template<typename T>
    void pushTyped(T v) {
        VERBOSE("  push " << v);
        push(val_t(v));
    }    
    void pushInt(int64_t v) { pushTyped(v); }
    void pushDouble(double v) { pushTyped(v); }
    void pushUInt16(uint16_t v) { pushTyped(v); }

    val_t pop() { 
        assert(!_stack.empty());
        val_t v = _stack.top();
        _stack.pop();
        return v;
    }
            
    template<typename T>
    T popTyped() {
        assert(!_stack.empty());
        T v = T(_stack.top());
        _stack.pop();
        VERBOSE("  popTyped: " << v);
        return v;
    }    
    double popDouble() { return popTyped<double>(); }
    int64_t popInt() { return popTyped<int64_t>(); }
    uint16_t popUInt16() { return popTyped<uint16_t>(); }
    
    val_t top() {
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
};

}

#endif /* INTERPRETER_CODE_H */
