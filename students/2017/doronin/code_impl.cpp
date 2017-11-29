#include "code_impl.h"

#define EXTRACT_TOP_2 \
    auto upper = _data_stack.top(); \
    _data_stack.pop(); \
    auto lower = _data_stack.top(); \
    _data_stack.pop();

namespace mathvm {

Status* CodeImpl::execute(vector<Var*>& vars) {
    BytecodeFunction *main = dynamic_cast<BytecodeFunction*>(functionById(0));
    _stack.push_back(main);

    _bci.push_back(0);

    _registers.clear();
    _registers.resize(4);

    _memory.clear();
    _memory.resize(_scopes.size() + 10);
    _memory[main->scopeId()].emplace_back();
    _memory[main->scopeId()].back().resize(_scope_vars[main->scopeId()].size() + 10);

    for (; get_bci() < get_bytecode()->length(); ) {
        auto bci_idx = get_bci_index();
        auto bci_old = get_bci(bci_idx);
        Instruction instruction = get_bytecode()->getInsn(bci_old);
        bytecodeName(instruction, &_length);
        switch (instruction)
        {
            #define INSTRUCTION_CASE(b, s, l) case BC_##b: b(); break;
                FOR_BYTECODES(INSTRUCTION_CASE)
            #undef INSTRUCTION_CASE
            default: assert(false);
        }

        if (bci_old == get_bci(bci_idx) && instruction != BC_RETURN) {
            get_bci(bci_idx) += _length;
        }

        if (is_stop) {
            break;
        }
    }

    return mathvm::Status::Ok();
}

void CodeImpl::INVALID() {
    exit(-1);
}

void CodeImpl::DLOAD() {
    _data_stack.push(Data { .d = get_bytecode()->getDouble(get_bci() + 1) });
}

void CodeImpl::ILOAD() {
    _data_stack.push(Data { .i = get_bytecode()->getInt64(get_bci() + 1) });
}

void CodeImpl::SLOAD() {
    _data_stack.push(Data { .s = constantById(get_bytecode()->getUInt16(get_bci() + 1)).c_str() });
}

void CodeImpl::DLOAD0() {
    _data_stack.push(Data { .d = 0.0 });
}

void CodeImpl::ILOAD0() {
    _data_stack.push(Data { .i = 0 });
}

void CodeImpl::SLOAD0() {
    _data_stack.push(Data { .s = constantById(0).c_str() });
}

void CodeImpl::DLOAD1() {
    _data_stack.push(Data { .d = 1.0 });
}

void CodeImpl::ILOAD1() {
    _data_stack.push(Data { .i = 1 });
}

void CodeImpl::DLOADM1() {
    _data_stack.push(Data { .d = -1.0 });
}

void CodeImpl::ILOADM1() {
    _data_stack.push(Data { .i = -1 });
}

void CodeImpl::DADD() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .d = upper.d + lower.d });
}

void CodeImpl::IADD() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.i + lower.i });
}

void CodeImpl::DSUB() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .d = upper.d - lower.d });
}

void CodeImpl::ISUB() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.i - lower.i });
}

void CodeImpl::DMUL() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .d = upper.d * lower.d });
}

void CodeImpl::IMUL() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.i * lower.i });
}

void CodeImpl::DDIV() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .d = upper.d / lower.d });
}

void CodeImpl::IDIV() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.i / lower.i });
}

void CodeImpl::IMOD() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.i % lower.i });
}

void CodeImpl::DNEG() {
    _data_stack.top().d = -_data_stack.top().d;
}

void CodeImpl::INEG() {
    _data_stack.top().i = -_data_stack.top().i;
}

void CodeImpl::IAOR() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.i | lower.i });
}

void CodeImpl::IAAND() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.i & lower.i });
}

void CodeImpl::IAXOR() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.i ^ lower.i });
}

void CodeImpl::IPRINT() {
    cout << _data_stack.top().i;
    _data_stack.pop();
}

void CodeImpl::DPRINT() {
    cout << _data_stack.top().d;
    _data_stack.pop();
}

void CodeImpl::SPRINT() {
    cout << _data_stack.top().s;
    _data_stack.pop();
}

void CodeImpl::I2D() {
    _data_stack.top().d = static_cast<double>(_data_stack.top().i);
}

void CodeImpl::D2I() {
    _data_stack.top().i = static_cast<int64_t>(_data_stack.top().d);
}

void CodeImpl::S2I() {
    _data_stack.top().i = static_cast<int64_t>(stoi(_data_stack.top().s));
}

void CodeImpl::SWAP() {
    auto top = _data_stack.top();
    _data_stack.pop();
    swap(_data_stack.top(), top);
    _data_stack.push(top);
}

void CodeImpl::POP() {
    _data_stack.pop();
}

void CodeImpl::LOADDVAR0() {
    _data_stack.push(_registers[0]);
}

void CodeImpl::LOADDVAR1() {
    _data_stack.push(_registers[1]);
}

void CodeImpl::LOADDVAR2() {
    _data_stack.push(_registers[2]);
}

void CodeImpl::LOADDVAR3() {
    _data_stack.push(_registers[3]);
}

void CodeImpl::LOADIVAR0() {
    _data_stack.push(_registers[0]);
}

void CodeImpl::LOADIVAR1() {
    _data_stack.push(_registers[1]);
}

void CodeImpl::LOADIVAR2() {
    _data_stack.push(_registers[2]);
}

void CodeImpl::LOADIVAR3() {
    _data_stack.push(_registers[3]);
}

void CodeImpl::LOADSVAR0() {
    _data_stack.push(_registers[0]);
}

void CodeImpl::LOADSVAR1() {
    _data_stack.push(_registers[1]);
}

void CodeImpl::LOADSVAR2() {
    _data_stack.push(_registers[2]);
}

void CodeImpl::LOADSVAR3() {
    _data_stack.push(_registers[3]);
}

void CodeImpl::STOREDVAR0() {
    _registers[0] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STOREDVAR1() {
    _registers[1] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STOREDVAR2() {
    _registers[2] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STOREDVAR3() {
    _registers[3] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STOREIVAR0() {
    _registers[0] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STOREIVAR1() {
    _registers[1] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STOREIVAR2() {
    _registers[2] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STOREIVAR3() {
    _registers[3] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STORESVAR0() {
    _registers[0] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STORESVAR1() {
    _registers[1] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STORESVAR2() {
    _registers[2] = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STORESVAR3() {
    _registers[3] = _data_stack.top();
    _data_stack.pop();
}


void CodeImpl::LOADDVAR() {
    _data_stack.push(Data { .d = get_bytecode()->getDouble(get_bytecode()->getUInt16(get_bci() + 1)) });
}

void CodeImpl::LOADIVAR() {
    _data_stack.push(Data { .i = get_bytecode()->getInt64(get_bytecode()->getUInt16(get_bci() + 1)) });
}

void CodeImpl::LOADSVAR() {
    _data_stack.push(Data { .s = constantById(get_bytecode()->getUInt16(get_bytecode()->getUInt16(get_bci() + 1))).c_str() });
}

void CodeImpl::STOREDVAR() {
    get_bytecode()->setTyped(get_bytecode()->getUInt16(get_bci() + 1), _data_stack.top().d);
}

void CodeImpl::STOREIVAR() {
    get_bytecode()->setTyped(get_bytecode()->getUInt16(get_bci() + 1), _data_stack.top().i);
}

void CodeImpl::STORESVAR() {
    get_bytecode()->setTyped(get_bytecode()->getUInt16(get_bci() + 1), _data_stack.top().s);
}

void CodeImpl::LOADCTXDVAR() {
    uint16_t scope_id = get_bytecode()->getUInt16(get_bci() + 1);
    uint16_t variable_id = get_bytecode()->getUInt16(get_bci() + 3);
    _data_stack.push(get_variable(scope_id, variable_id));
}

void CodeImpl::LOADCTXIVAR() {
    uint16_t scope_id = get_bytecode()->getUInt16(get_bci() + 1);
    uint16_t variable_id = get_bytecode()->getUInt16(get_bci() + 3);
    auto r = get_variable(scope_id, variable_id);
    _data_stack.push(r);
}

void CodeImpl::LOADCTXSVAR() {
    uint16_t scope_id = get_bytecode()->getUInt16(get_bci() + 1);
    uint16_t variable_id = get_bytecode()->getUInt16(get_bci() + 3);
    _data_stack.push(get_variable(scope_id, variable_id));
}

void CodeImpl::STORECTXDVAR() {
    uint16_t scope_id = get_bytecode()->getUInt16(get_bci() + 1);
    uint16_t variable_id = get_bytecode()->getUInt16(get_bci() + 3);
    get_variable(scope_id, variable_id) = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STORECTXIVAR() {
    uint16_t scope_id = get_bytecode()->getUInt16(get_bci() + 1);
    uint16_t variable_id = get_bytecode()->getUInt16(get_bci() + 3);
    get_variable(scope_id, variable_id) = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::STORECTXSVAR() {
    uint16_t scope_id = get_bytecode()->getUInt16(get_bci() + 1);
    uint16_t variable_id = get_bytecode()->getUInt16(get_bci() + 3);
    get_variable(scope_id, variable_id) = _data_stack.top();
    _data_stack.pop();
}

void CodeImpl::DCMP() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.d == lower.d ? 0 : (upper.d > lower.d ? 1 : -1) });
}

void CodeImpl::ICMP() {
    EXTRACT_TOP_2
    _data_stack.push(Data { .i = upper.i == lower.i ? 0 : (upper.i > lower.i ? 1 : -1) });
}

void CodeImpl::JA() {
    get_bci() = get_bytecode()->getInt16(get_bci() + 1) + get_bci() + 1;
}

void CodeImpl::IFICMPNE() {
    EXTRACT_TOP_2
    if (upper.i != lower.i) {
        get_bci() = get_bytecode()->getInt16(get_bci() + 1) + get_bci() + 1;
    }
}

void CodeImpl::IFICMPE() {
    EXTRACT_TOP_2
    if (upper.i == lower.i) {
        get_bci() = get_bytecode()->getInt16(get_bci() + 1) + get_bci() + 1;
    }
}

void CodeImpl::IFICMPG() {
    EXTRACT_TOP_2
    if (upper.i > lower.i) {
        get_bci() = get_bytecode()->getInt16(get_bci() + 1) + get_bci() + 1;
    }
}

void CodeImpl::IFICMPGE() {
    EXTRACT_TOP_2
    if (upper.i >= lower.i) {
        get_bci() = get_bytecode()->getInt16(get_bci() + 1) + get_bci() + 1;
    }
}

void CodeImpl::IFICMPL() {
    EXTRACT_TOP_2
    if (upper.i < lower.i) {
        get_bci() = get_bytecode()->getInt16(get_bci() + 1) + get_bci() + 1;
    }
}

void CodeImpl::IFICMPLE() {
    EXTRACT_TOP_2
    if (upper.i <= lower.i) {
        get_bci() = get_bytecode()->getInt16(get_bci() + 1) + get_bci() + 1;
    }
}

void CodeImpl::DUMP() {
    cout << "Stacktrace" << endl;
    for (auto& fun : _stack)
        cout << fun->name() << endl;

    cout << "Stack dump" << endl;
    stack<Data> tmp(_data_stack);
    while (!tmp.empty()) {
        cout << tmp.top().i << " " << tmp.top().d << endl;
        tmp.pop();
    }
}

void CodeImpl::STOP() {
    is_stop = true;
}

void CodeImpl::CALL() {
    uint16_t function_id = get_bytecode()->getUInt16(get_bci() + 1);
    BytecodeFunction *function = dynamic_cast<BytecodeFunction*>(functionById(function_id));
    _stack.push_back(function);
    _bci.emplace_back(0);

    for (const auto& depId : _dependencies[_stack.back()->id()]) {
        _memory[depId].emplace_back();
        _memory[depId].back().resize(_scope_vars[depId].size() + 10);
    }

    _memory[function->scopeId()].emplace_back();
    _memory[function->scopeId()].back().resize(_scope_vars[function->scopeId()].size() + 10);
}

extern "C" void native_call(void*, void*, const void*, void*, void*);

void CodeImpl::CALLNATIVE() {
    uint16_t function_id = get_bytecode()->getUInt16(get_bci() + 1);
    const Signature* signtature = nullptr;
    const string* name = nullptr;
    auto function = nativeById(function_id, &signtature, &name);
    Data result;
    Data result_double;
    int count_int = 0;
    int count_double = 0;
    Data ints[10];
    Data doubles[10];

    for (size_t i = 1; i < signtature->size(); i++) {
        if (signtature->at(i).first == VT_DOUBLE) {
            doubles[count_double++] = _data_stack.top();
        } else {
            ints[count_int++] = _data_stack.top();
        }
        _data_stack.pop();
    }

    native_call(doubles, ints, function, &result, &result_double);

    if (signtature->at(0).first == VT_DOUBLE) {
        _data_stack.push(result_double);
    } else if (signtature->at(0).first != VT_VOID) {
        _data_stack.push(result);
    }
}

void CodeImpl::RETURN() {
    for (const auto& depId : _dependencies[_stack.back()->id()]) {
        _memory[depId].pop_back();
    }

    _memory[_stack.back()->scopeId()].pop_back();
    _stack.pop_back();
    _bci.pop_back();
}

void CodeImpl::BREAK() {
    asm("int $3");
}

}
