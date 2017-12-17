#include <stdio.h>

#include "../../../../include/mathvm.h"

#include "include/code_interpreter.h"

using namespace mathvm;

code_interpret::code_interpret(Code *_code) :
        _code(_code),
        _id_string(_code->makeStringConstant("")),
        _storage_local_variable(_id_string) {
    preallocateMemory();
    auto topFunction = _code->functionByName(topFunctionName);
    _function_stack.push_back(topFunction);
    newCounter();
    newContext(topFunction->localsNumber());
}


void code_interpret::execute() {
    while (!_function_stack.empty()) {
        executeInstruction();
    }
}

void code_interpret::preallocateMemory() {
    _function_stack.reserve(alloc_memory);
    stack_counter_prog.reserve(alloc_memory);
    _data_stack.reserve(alloc_memory);
    _stack_local_var.reserve(alloc_memory);
}

void code_interpret::executeInstruction() {
    Instruction instruction = currentBytecode()->getInsn(programCounter());
    next();
    switch (instruction) {
        case mathvm::BC_INVALID:
            throw exec_exception("invalid instruction");
        case BC_DLOAD:
            dload();
            break;
        case BC_ILOAD:
            iload();
            break;
        case BC_SLOAD:
            sload();
            break;
        case BC_DLOAD0:
            dload0();
            break;
        case BC_ILOAD0:
            iload0();
            break;
        case BC_SLOAD0:
            sload0();
            break;
        case BC_ILOAD1:
            iload1();
            break;
        case BC_DLOAD1:
            dload1();
            break;
        case BC_ILOADM1:
            iloadm1();
            break;
        case BC_DLOADM1:
            dloadm1();
            break;
        case BC_IADD:
            iadd();
            break;
        case BC_DADD:
            dadd();
            break;
        case BC_ISUB:
            isub();
            break;
        case BC_DSUB:
            dsub();
            break;
        case BC_IMUL:
            imul();
            break;
        case BC_DMUL:
            dmul();
            break;
        case BC_IDIV:
            idiv();
            break;
        case BC_DDIV:
            ddiv();
            break;
        case BC_IMOD:
            imod();
            break;
        case BC_INEG:
            ineg();
            break;
        case BC_DNEG:
            dneg();
            break;
        case BC_IAOR:
            iaor();
            break;
        case BC_IAAND:
            iaand();
            break;
        case BC_IAXOR:
            iaxor();
            break;
        case BC_IPRINT:
            iprint();
            break;
        case BC_DPRINT:
            dprint();
            break;
        case BC_SPRINT:
            sprint();
            break;
        case BC_I2D:
            i2d();
            break;
        case BC_D2I:
            d2i();
            break;
        case BC_SWAP:
            swap();
            break;
        case BC_POP:
            pop();
            break;
        case BC_LOADDVAR:
            loaddvar();
            break;
        case BC_LOADIVAR:
            loadivar();
            break;
        case BC_LOADSVAR:
            loadsvar();
            break;
        case BC_STOREDVAR:
            storedvar();
            break;
        case BC_STOREIVAR:
            storeivar();
            break;
        case BC_STORESVAR:
            storesvar();
            break;
        case BC_LOADCTXDVAR:
            loadctxdvar();
            break;
        case BC_LOADCTXIVAR:
            loadctxivar();
            break;
        case BC_LOADCTXSVAR:
            loadctxsvar();
            break;
        case BC_STORECTXDVAR:
            storectxdvar();
            break;
        case BC_STORECTXIVAR:
            storectxivar();
            break;
        case BC_STORECTXSVAR:
            storectxsvar();
            break;
        case BC_DCMP:
            dcmp();
            break;
        case BC_ICMP:
            icmp();
            break;
        case BC_JA:
            ja();
            break;
        case BC_IFICMPNE:
            ificmpne();
            break;
        case BC_IFICMPE:
            ificmpe();
            break;
        case BC_IFICMPG:
            ificmpg();
            break;
        case BC_IFICMPGE:
            ificmpge();
            break;
        case BC_IFICMPL:
            ificmpl();
            break;
        case BC_IFICMPLE:
            ificmple();
            break;
        case BC_DUMP:
            dump();
            break;
        case BC_STOP:
            throw exec_exception("stop");
        case BC_CALL:
            call();
            break;
        case BC_CALLNATIVE:
            throw exec_exception("natives not supported");
        case BC_RETURN:
            bc_return();
            break;
        case BC_BREAK:
            next();
            break;
        default:
            break;
    }
}

void code_interpret::call() {
    int16_t id = int16();
    _function_stack.push_back(_code->functionById(id));

    newCounter();
    newContext(currentFunction()->localsNumber());
}

void code_interpret::ja() {
    int16_t relativeOffset = currentBytecode()->getInt16(programCounter());
    addRelativeOffset(relativeOffset);
}

void code_interpret::ificmpne() {
    auto args = topPair();
    if (args.first.value_int != args.second.value_int) {
        ja();
    } else {
        int16();
    }
}

void code_interpret::ificmpe() {
    auto args = topPair();
    if (args.first.value_int == args.second.value_int) {
        ja();
    } else {
        int16();
    }
}

void code_interpret::ificmpg() {
    auto args = topPair();
    push(args.second);
    if (args.first.value_int > args.second.value_int) {
        ja();
    } else {
        int16();
    }
}

void code_interpret::ificmpge() {
    auto args = topPair();
    push(args.second);
    if (args.first.value_int >= args.second.value_int) {
        ja();
    } else {
        int16();
    }
}

void code_interpret::ificmpl() {
    auto args = topPair();
    push(args.second);
    if (args.first.value_int < args.second.value_int) {
        ja();
    } else {
        int16();
    }
}

void code_interpret::ificmple() {
    auto args = topPair();
    push(args.second);
    if (args.first.value_int <= args.second.value_int) {
        ja();
    } else {
        int16();
    }
}

void code_interpret::loadivar() {
    int16_t id = int16();
    ipush(_storage_local_variable.iload(id));
}

void code_interpret::loaddvar() {
    int16_t id = int16();
    dpush(_storage_local_variable.dload(id));
}

void code_interpret::loadsvar() {
    int16_t id = int16();
    spush(_storage_local_variable.sload(id));
}

void code_interpret::storeivar() {
    int16_t id = int16();
    auto arg = tos();
    _storage_local_variable.istore(id, arg.value_int);
}

void code_interpret::storedvar() {
    int16_t id = int16();
    auto arg = tos();
    _storage_local_variable.dstore(id, arg.value_double);
}

void code_interpret::storesvar() {
    int16_t id = int16();
    auto arg = tos();
    _storage_local_variable.sstore(id, arg.id);
}

void code_interpret::loadctxivar() {
    int16_t ctxId = int16();
    int16_t id = int16();
    ipush(_storage_local_variable.iload(ctxId, id));
}

void code_interpret::loadctxdvar() {
    int16_t ctxId = int16();
    int16_t id = int16();
    dpush(_storage_local_variable.dload(ctxId, id));
}

void code_interpret::loadctxsvar() {
    int16_t ctxId = int16();
    int16_t id = int16();
    spush(_storage_local_variable.sload(ctxId, id));
}

void code_interpret::storectxivar() {
    int16_t ctxId = int16();
    int16_t id = int16();
    auto arg = tos();
    _storage_local_variable.istore(ctxId, id, arg.value_int);
}

void code_interpret::storectxdvar() {
    int16_t ctxId = int16();
    int16_t id = int16();
    auto arg = tos();
    _storage_local_variable.dstore(ctxId, id, arg.value_double);
}

void code_interpret::storectxsvar() {
    int16_t ctxId = int16();
    int16_t id = int16();
    auto arg = tos();
    _storage_local_variable.sstore(ctxId, id, arg.id);
}

void code_interpret::dload() {
    dpush(currentBytecode()->getDouble(programCounter()));
    next8();
}

void code_interpret::iload() {
    ipush(currentBytecode()->getInt64(programCounter()));
    next8();
}

void code_interpret::sload() {
    int16_t id = int16();
    spush(id);
}

void code_interpret::dload0() {
    dpush((double) 0.0);
}

void code_interpret::iload0() {
    ipush((int64_t) 0);
}

void code_interpret::sload0() {
    int16_t id = _code->makeStringConstant("");
    spush(id);
}

void code_interpret::iload1() {
    ipush((int64_t) 1);
}

void code_interpret::dload1() {
    dpush((double) 1.0);
}

void code_interpret::iloadm1() {
    ipush((int64_t) -1);
}

void code_interpret::dloadm1() {
    dpush((double) -1.0);
}

void code_interpret::iadd() {
    auto args = topPair();
    ipush(args.first.value_int + args.second.value_int);
}

void code_interpret::dadd() {
    auto args = topPair();
    dpush(args.first.value_double + args.second.value_double);
}

void code_interpret::isub() {
    auto args = topPair();
    ipush(args.first.value_int - args.second.value_int);
}

void code_interpret::dsub() {
    auto args = topPair();
    dpush(args.first.value_double - args.second.value_double);
}

void code_interpret::imul() {
    auto args = topPair();
    ipush(args.first.value_int * args.second.value_int);
}

void code_interpret::dmul() {
    auto args = topPair();
    dpush(args.first.value_double * args.second.value_double);
}

void code_interpret::idiv() {
    auto args = topPair();
    ipush(args.first.value_int / args.second.value_int);
}

void code_interpret::ddiv() {
    auto args = topPair();
    dpush(args.first.value_double / args.second.value_double);
}

void code_interpret::imod() {
    auto args = topPair();
    ipush(args.first.value_int % args.second.value_int);
}

void code_interpret::ineg() {
    auto args = tos();
    ipush((int64_t) -args.value_int);
}

void code_interpret::dneg() {
    auto args = tos();
    dpush(-args.value_double);
}

void code_interpret::iaor() {
    auto args = topPair();
    ipush(args.first.value_int | args.second.value_int);
}

void code_interpret::iaand() {
    auto args = topPair();
    ipush(args.first.value_int & args.second.value_int);
}

void code_interpret::iaxor() {
    auto args = topPair();
    ipush(args.first.value_int ^ args.second.value_int);
}

void code_interpret::iprint() {
    auto args = tos();
    printf("%ld", args.value_int);
}

void code_interpret::dprint() {
    auto args = tos();
    printf("%g", args.value_double);
}

void code_interpret::sprint() {
    auto args = tos();
    printf("%s", _code->constantById(args.id).c_str());
}

void code_interpret::i2d() {
    auto args = tos();
    dpush((double) args.value_int);
}

void code_interpret::d2i() {
    auto args = tos();
    ipush((double) args.value_double);
}

void code_interpret::bc_return() {
    _function_stack.pop_back();
    stack_counter_prog.pop_back();
    popContext();
}

void code_interpret::swap() {
    std::iter_swap(_data_stack.rbegin(), _data_stack.rbegin() + 1);
}

void code_interpret::icmp() {
    auto args = topPair();
    if (args.first.value_int == args.second.value_int) {
        ipush((int64_t) 0);
    } else if (args.first.value_int < args.second.value_int) {
        ipush((int64_t) -1);
    } else {
        ipush((int64_t) 1);
    }
}

void code_interpret::dcmp() {
    auto args = topPair();
    if (args.first.value_double == args.second.value_double) {
        ipush((int64_t) 0);
    } else if (args.first.value_double < args.second.value_double) {
        ipush((int64_t) -1);
    } else {
        ipush((int64_t) 1);
    }
}

void code_interpret::dump() {
    auto args = top();
    printf("%d\n", args.id);
    printf("%ld\n", args.value_int);
    printf("%f\n", args.value_double);
}

int32_t code_interpret::programCounter() { return stack_counter_prog.back(); }

int32_t code_interpret::stackFrame() { return _stack_frame.back(); }

void code_interpret::push(data holder) {
    _data_stack.push_back(holder);
}

void code_interpret::spush(int16_t value) {
    _data_stack.push_back(data(value));
}

void code_interpret::ipush(int64_t value) {
    _data_stack.push_back(data(value));
}

void code_interpret::dpush(double value) {
    _data_stack.push_back(data(value));
}

void code_interpret::pop() {
    _data_stack.pop_back();
}

data code_interpret::top() { return _data_stack.back(); }

code_interpret::data_pair code_interpret::topPair() {
    code_interpret::data_pair result;
    result.first = tos();
    result.second = tos();
    return result;
}

data code_interpret::tos() {
    auto result = top();
    pop();
    return result;
}

Bytecode *code_interpret::currentBytecode() {
    return static_cast<BytecodeFunction *>(currentFunction())->bytecode();
}

TranslatedFunction *code_interpret::currentFunction() { return _function_stack.back(); }

void code_interpret::newContext(int32_t localsNumber) {
    _storage_local_variable.newContext(localsNumber);
}

void code_interpret::popContext() {
    _storage_local_variable.popContext();
}

void code_interpret::addRelativeOffset(int16_t relativeOffset) {
    stack_counter_prog.back() += relativeOffset;
}

int16_t code_interpret::int16() {
    int16_t result = currentBytecode()->getInt16(programCounter());
    next2();
    return result;
}

void code_interpret::next(size_t i) {
    stack_counter_prog.back() += i;
}

void code_interpret::next2() {
    next(2);
}

void code_interpret::next3() {
    next(3);
}

void code_interpret::next4() {
    next(4);
}

void code_interpret::next8() {
    next(8);
}

void code_interpret::newCounter() {
    stack_counter_prog.push_back(0);
}

