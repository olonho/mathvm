#ifndef BYTECODE_INTERPRETER_H
#define BYTECODE_INTERPRETER_H

#include "mathvm.h"
#include <bits/stdc++.h>

namespace mathvm {
using namespace std;

typedef union {
    int64_t I;
    double D;
    uint16_t S;
} IDSValue;

struct EntryPoint {
    uint16_t function_id;
    uint16_t ip;
    
    EntryPoint(uint16_t function_id, uint16_t ip)
            : function_id(function_id)
            , ip(ip)
    {}
};

struct Frame {
    uint16_t function_id;
    vector<IDSValue> locals;
    
    Frame(uint16_t function_id, size_t num_locals)
            : function_id(function_id)
            , locals(num_locals, IDSValue())
    {}
};

class BytecodeImpl : public Code {
    
public:
    BytecodeImpl();
    
    Status *execute(vector<Var *> &vars) override;
    
private:
    vector<Frame> frames_;
    map<uint16_t, vector<int>> frame_reference_;
    vector<uint8_t> stack_;
    std::vector<EntryPoint> call_frames_;
    
    uint16_t current_function_;
    uint32_t ip_;
    Bytecode *bc_;
    IDSValue var_[4];
    
    void execute(Instruction instruction);
    
    void call();
    
    void ret();
    
    template<class T>
    void pop(T &result) {
        const int tsize = sizeof(T);
        auto bytes = reinterpret_cast<uint8_t *>(&result);
        for (int i = tsize - 1; i >= 0; --i) {
            bytes[i] = stack_.back();
            stack_.pop_back();
        }
    }
    
    template<class T>
    T pop() {
        T result;
        pop(result);
        return result;
    }
    
    double popD() { return pop<double>(); }
    int64_t popI() { return pop<int64_t>(); }
    uint16_t popS() { return pop<uint16_t>(); }
    
    template<class T>
    void push(T value) {
        const int tsize = sizeof(T);
        auto bytes = reinterpret_cast<uint8_t  *>(&value);
        std::copy(bytes, bytes + tsize, std::back_inserter(stack_));
    }
    
    IDSValue &get_var(uint16_t function_id, uint16_t var_id);
    
    void enter_function(uint16_t function_id) {
        auto function = dynamic_cast<BytecodeFunction *>(functionById(function_id));
        bc_ = function->bytecode();
        current_function_ = function_id;
        ip_ = 0;
        auto iframe = static_cast<int>(frames_.size());
        frame_reference_[current_function_].push_back(iframe);
        frames_.emplace_back(function_id, function->localsNumber());
    }
};
}

#endif // BYTECODE_INTERPRETER_H
