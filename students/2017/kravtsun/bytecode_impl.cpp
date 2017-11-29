#include <iostream>
#include <csignal>
#include "ast.h"
#include "bytecode_impl.h"

namespace mathvm {

BytecodeImpl::BytecodeImpl()
        : current_function_(0)
        , ip_(0)
        , bc_(nullptr)
{
    memset(var_, 0, sizeof(var_));
}

Status *BytecodeImpl::execute(vector<Var *> &vars) {
    try {
        auto function = dynamic_cast<BytecodeFunction *>(functionByName(AstFunction::top_name));
        enter_function(function->id());
        for (ip_ = 0; ip_ < bc_->length();) {
            auto next_instruction = bc_->getInsn(ip_);
            ip_++;
            execute(next_instruction);
        }
    }
    catch (std::logic_error &e) {
        return Status::Error(e.what());
    }
    
    return Status::Ok();
}

template<typename T>
static int64_t compare(const T &lhs, const T &rhs) {
    if (lhs < rhs) {
        return -1LL;
    } else if (lhs > rhs) {
        return 1LL;
    } else {
        return 0LL;
    }
}

IDSValue &BytecodeImpl::get_var(uint16_t function_id, uint16_t var_id) {
    const int iframe = frame_reference_[function_id].back();
    return frames_[iframe].locals[var_id];
}

void BytecodeImpl::execute(Instruction instruction) {
    IDSValue first;
    IDSValue second;
    
#define LOCAL_VAR(letter) \
    get_var(current_function_, bc_->getUInt16(ip_)).letter

#define CONTEXT_VAR(letter) \
    get_var(bc_->getUInt16(ip_), bc_->getUInt16(ip_ + 2)).letter
    auto pop2I = [&]() {
        pop(first.I);
        pop(second.I);
    };
    
    auto pop2D = [&]() {
        pop(first.D);
        pop(second.D);
    };
    
#define LOADER(letter, value, suffix) \
    case BC_ ## letter ## LOAD ## suffix: \
        push(value); \
        break;
    
#define BIN_ARITHMETIC(letter, operation, sign) \
    case BC_ ## letter ## operation: \
        pop2 ## letter (); \
        push(second.letter sign first.letter); \
        break;
    
    switch (instruction) {
        case BC_INVALID:
            throw std::logic_error("invalid instruction");
        case BC_DLOAD:
            push(bc_->getDouble(ip_));
            ip_ += 8;
            break;
        case BC_ILOAD:
            push(bc_->getInt64(ip_));
            ip_ += 8;
            break;
        case BC_SLOAD:
            push(bc_->getUInt16(ip_));
            ip_ += 2;
            break;
            
        LOADER(D, 0.0, 0)
        LOADER(I, 0LL, 0)
        LOADER(S, makeStringConstant(""), 0)
        LOADER(D, 1., 1)
        LOADER(I, 1LL, 1)
        LOADER(D, -1., M1)
        LOADER(I, -1LL, M1)
        
        BIN_ARITHMETIC(D, ADD, +)
        BIN_ARITHMETIC(I, ADD, +)
        BIN_ARITHMETIC(D, SUB, -)
        BIN_ARITHMETIC(I, SUB, -)
        BIN_ARITHMETIC(D, MUL, *)
        BIN_ARITHMETIC(I, MUL, *)
        BIN_ARITHMETIC(D, DIV, /)
        BIN_ARITHMETIC(I, DIV, /)
        BIN_ARITHMETIC(I, MOD, %)
        
        case BC_DNEG:
            push(-popD());
            break;
        case BC_INEG:
            push(-popI());
            break;
    
        BIN_ARITHMETIC(I, AOR, |)
        BIN_ARITHMETIC(I, AAND, &)
        BIN_ARITHMETIC(I, AXOR, ^)
        
        case BC_IPRINT:
            std::cout << popI();
            break;
        case BC_DPRINT:
            std::cout << popD();
            break;
        case BC_SPRINT:
            std::cout << constantById(popS());
            break;
    
        case BC_I2D:
            push<double>(popI());
            break;
        case BC_D2I:
            push(static_cast<int64_t>(popD()));
            break;
        case BC_S2I:
            push<int64_t>(stoll(constantById(popS())));
            break;
    
        case BC_SWAP:
            pop2I();
            push(first.I);
            push(second.I);
            break;
    
        case BC_POP:
            pop<uint8_t>();
            break;

#define DO_FOUR_TIMES(action, arg) \
        action(arg, 0) \
        action(arg, 1) \
        action(arg, 2) \
        action(arg, 3) \
    
#define VAR_LOADER(letter, num) \
        LOADER(, var_[num], letter ## VAR ## num)
        
        DO_FOUR_TIMES(VAR_LOADER, D)
        DO_FOUR_TIMES(VAR_LOADER, I)
        DO_FOUR_TIMES(VAR_LOADER, S)
        
#define VAR_STORER(letter, num) \
        case BC_STORE ## letter ## VAR ## num: \
            pop(first.letter); \
            var_[num]. letter = first.letter; \
            break;
        
        DO_FOUR_TIMES(VAR_STORER, D)
        DO_FOUR_TIMES(VAR_STORER, I)
        DO_FOUR_TIMES(VAR_STORER, S)

#define DO_TYPES(action) \
        action(D) \
        action(I) \
        action(S)

#define LOAD_LOCAL(letter) \
        case BC_LOAD ## letter ## VAR: \
            push(LOCAL_VAR(letter)); \
            ip_ += 2; \
            break;
        DO_TYPES(LOAD_LOCAL)

#define STORE_LOCAL(letter) \
        case BC_STORE ## letter ## VAR: \
            LOCAL_VAR(letter) = pop ## letter (); \
            ip_ += 2; \
            break;
        DO_TYPES(STORE_LOCAL)

#define LOAD_CONTEXT(letter) \
        case BC_LOADCTX ## letter ## VAR: \
            push(CONTEXT_VAR(letter)); \
            ip_ += 4; \
            break;
        DO_TYPES(LOAD_CONTEXT)

#define STORE_CONTEXT(letter) \
        case BC_STORECTX ## letter ## VAR: \
            CONTEXT_VAR(letter) = pop ## letter(); \
            ip_ += 4; \
            break;
        DO_TYPES(STORE_CONTEXT)
        
        case BC_DCMP:
            pop2D();
            push(compare(second.D, first.D));
            break;
        case BC_ICMP:
            pop2I();
            push(compare(second.I, first.I));
            break;
        case BC_JA:
            ip_ += bc_->getInt16(ip_);
            break;
            
#define IF_COMPARE(suffix, operation) \
        case BC_IFICMP ## suffix: \
            pop2I(); \
            if (second.I operation first.I) {\
                ip_ += bc_->getInt16(ip_);\
            } else { \
                ip_ += 2; \
            } \
            break;
    
        IF_COMPARE(NE, !=)
        IF_COMPARE(E, ==)
        IF_COMPARE(G, >)
        IF_COMPARE(GE, >=)
        IF_COMPARE(L, <)
        IF_COMPARE(LE, <=)
        
        case BC_DUMP:
            pop(first.I);
            std::cout << first.I << std::endl;
            push(first.I);
            break;
        case BC_STOP:
            exit(0);
        case BC_CALL:
            call();
            break;
        case BC_CALLNATIVE:
            break;
        case BC_RETURN:
            ret();
            break;
        case BC_BREAK:
#ifdef _WIN32
            std::raise(SIGABRT);
#else
            std::raise(SIGINT);
#endif
            break;
        default:
            throw std::logic_error("invalid instruction");
    }
}

void BytecodeImpl::call() {
    call_frames_.emplace_back(current_function_, ip_ + 2);
    auto function_id = bc_->getUInt16(ip_);
    enter_function(function_id);
}

void BytecodeImpl::ret() {
    frames_.pop_back();
    assert(frame_reference_.at(current_function_).back() == static_cast<int>(frames_.size()));
    frame_reference_.at(current_function_).pop_back();
    
    auto previous_entry = call_frames_.back();
    call_frames_.pop_back();
    current_function_ = previous_entry.function_id;
    ip_ = previous_entry.ip;
    bc_ = dynamic_cast<BytecodeFunction *>(functionById(current_function_))->bytecode();
}

}
