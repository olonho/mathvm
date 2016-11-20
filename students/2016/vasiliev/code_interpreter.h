
#ifndef MATHVM_CODE_INTERPRETER_H
#define MATHVM_CODE_INTERPRETER_H

#include "mathvm.h"

using namespace mathvm;

typedef union {
    int64_t i;
    double d;
    int16_t s;
} universal;

class code_interpreter: public Code {
    vector<universal> locals;
    vector<uint8_t> stack;
    vector<pair<uint16_t, uint32_t>> calls;


    uint16_t current_id;
    uint32_t locals_offset;
    uint32_t index;
    Bytecode* bytecode;
    uint16_t locals_count;

    void execute_insn();

    void call(uint16_t id) {
        BytecodeFunction* f = (BytecodeFunction*) functionById(id);
        bytecode = f->bytecode();
        calls.push_back(make_pair(current_id, index));
        index = 0;
        current_id = id;
        locals_offset += locals_count;
        locals_count = f->localsNumber();
    }

    void ret() {
        auto p = calls.back();
        calls.pop_back();
        index = p.second;
        current_id = p.first;
        BytecodeFunction* f = (BytecodeFunction*) functionById(current_id);
        bytecode = f->bytecode();
        locals_count = f->localsNumber();
        locals_offset -= locals_count;
    }


    template<class T> T pop_typed() {
        union {
            T val;
            uint8_t bytes[sizeof(T)];
        } u;
        for (uint8_t i = sizeof(T) - 1; i != 255; --i) {
            u.bytes[i] = stack.back();
            stack.pop_back();
        }
        return u.val;
    }

    template<class T> void push_typed(T val) {
        union {
            T val;
            uint8_t bytes[sizeof(T)];
        } u;
        u.val = val;
        for (uint8_t i = 0; i < sizeof(T); ++i) {
            stack.push_back(u.bytes[i]);
        }
    }

    int64_t get_local_i(uint16_t id) {
        return locals[locals_offset + id].i;
    }

    void put_local_i(uint16_t id, int64_t val) {
        locals[locals_offset + id].i = val;
    }

    double get_local_d(uint16_t id) {
        return locals[locals_offset + id].d;
    }

    void put_local_d(uint16_t id, double val) {
        locals[locals_offset + id].d = val;
    }

    uint16_t get_local_s(uint16_t id) {
        return locals[locals_offset + id].s;
    }

    void put_local_s(uint16_t id, uint16_t val) {
        locals[locals_offset + id].s = val;
    }

//        union {
//            T val;
//            uint8_t bytes[sizeof(T)];
//        } u;
//        for (uint8_t i = 0; i < sizeof(T); ++i) {
//            u.bytes[id][i] = locals[c_offset + i];
//        }
//        return u.val;

//        union {
//            T val;
//            uint8_t bytes[sizeof(T)];
//        } u;
//        u.val = val;
//        for (uint8_t i = 0; i < sizeof(T); ++i) {
//            locals[c_offset + id * 8 + i] = u.bytes[id][i];
//        }

public:
    code_interpreter();
    virtual Status* execute(vector<Var*>& vars);
};


#endif //MATHVM_CODE_INTERPRETER_H
