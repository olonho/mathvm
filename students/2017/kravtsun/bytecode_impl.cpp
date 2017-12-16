#include <iostream>
#include <csignal>
#include <dlfcn.h>
#include "ast.h"
#include "bytecode_impl.h"

#define LOGGING 0


namespace mathvm {

using namespace asmjit;

BytecodeImpl::BytecodeImpl()
        : current_function_(0)
        , ip_(0)
        , bc_(nullptr)
{
    memset(var_, 0, sizeof(var_));
}


BytecodeImpl::~BytecodeImpl() {
//    jit_runtime_.release()
//        auto it = NativeFunctionIterator(this);
//        while (it.hasNext()) {
//            jitRuntime.release(it.next().code());
//        }
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
    uint16_t string_id;
    char *string_ptr;
    string str;
#if LOGGING
#define PRINT_INST(name, msg, size) \
    if (instruction == BC_ ## name) { \
        cout << ip_ << ": " << #name << endl; \
    }
FOR_BYTECODES(PRINT_INST)
#undef PRINT_INST
#endif

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
        LOADER(S, makeStringConstant1(""), 0)
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
            string_id = popS();
            std::cout << stringById(string_id);
            break;
    
        case BC_I2D:
            push<double>(popI());
            break;
        case BC_D2I:
            push(static_cast<int64_t>(popD()));
            break;
        case BC_S2I:
            string_id = popS();
            string_ptr = stringById(string_id);
            str = string(string_ptr);
            if (str.empty()) {
                push<int64_t>(string_ptr != nullptr);
            } else if (std::all_of(str.begin(), str.end(), [](char c) -> bool { return isdigit(c); })) {
                push<int64_t>(stoll(str));
            } else {
                push<int64_t>(1LL);
            }
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
            callnative();
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
    auto function_id = bc_->getUInt16(ip_);
    ip_ += 2;
    
    call_frames_.emplace_back(current_function_, ip_);
    enter_function(function_id);
}


using namespace asmjit;

typedef int64_t (*IntReturnFunc)();
typedef uintptr_t (*PtrReturnFunc)();
typedef void (*VoidReturnFunc)();
typedef double (*DoubleReturnFunc)();

//void BytecodeImpl::callnative(uint16_t function_id, const std::string &native_name) {
void BytecodeImpl::callnative() {
    X86Assembler assembler_(&jit_runtime_);
    X86Compiler compiler_(&assembler_);
    
//    FileLogger logger_(stdout);
//    assembler_.setLogger(&logger_);

    auto function_id = bc_->getUInt16(ip_);
    ip_ += 2;

//    compiler_.reset(true);
//    compiler_.attach(&assembler_);
    const Signature *signature = nullptr;
    const string *name = nullptr;
    
    auto address = nativeById(function_id, &signature, &name);

#if LOGGING
    cout << "native calling: " << *name << endl;
#endif

    const auto ASMJIT_DOUBLE_TYPE = kX86VarTypeXmmSd;
    const auto ASMJIT_INT_TYPE = kVarTypeInt64;
    const auto ASMJIT_PTR_TYPE = kVarTypeUIntPtr;
    auto asmjit_type_from_var_type = [=](VarType type) -> uint32_t {
        switch (type) {
            case VT_STRING:
                return ASMJIT_PTR_TYPE;
            case VT_INT:
                return ASMJIT_INT_TYPE;
            case VT_DOUBLE:
                return ASMJIT_DOUBLE_TYPE;
            default:
                return kInvalidVar;
        }
    };

    FuncBuilderX native_prototype;
    for (size_t i = 0; i < signature->size(); ++i) {
        VarType sig_type = signature->at(i).first;
        auto asmjit_type = asmjit_type_from_var_type(sig_type);
        if (i == 0) {
            native_prototype.setRet(asmjit_type);
        } else {
            native_prototype.addArg(asmjit_type);
        }
    }

    auto main_prototype = FuncBuilderX();
    main_prototype.setRet(native_prototype.getRet());
    compiler_.addFunc(main_prototype);
    
    assert(signature != nullptr && !signature->empty());

    vector<asmjit::Var *> vars;
    
    const uint32_t SCOPE = kConstScopeGlobal;
    
    using namespace asmjit::x86;
    
    double double_value;
    int64_t integer_value;
    uint16_t string_id;
    char *string_ptr;

//    for (auto i = static_cast<uint32_t>(signature->size() - 1); i >= 1; --i) {
    for (uint32_t  i = 1; i < signature->size(); ++i) {
        const VarType sig_type = signature->at(i).first;
        auto parameter_name = signature->at(i).second;
        auto arg_var_name = ("arg" + std::to_string(i) + "_" + parameter_name).c_str();

        if (sig_type == VT_DOUBLE) {
            double_value = popD();
            auto constant = compiler_.newDoubleConst(SCOPE, double_value);
            auto double_var = new X86XmmVar(compiler_.newXmmSd(arg_var_name));
            compiler_.movsd(*double_var, constant);
            vars.push_back(double_var);
        } else if (sig_type == VT_INT) {
            integer_value = popI();
            auto constant = compiler_.newInt64Const(SCOPE, integer_value);
            auto integer_var = new X86GpVar(compiler_.newGpVar(ASMJIT_INT_TYPE, arg_var_name));
            compiler_.mov(*integer_var, constant);
            vars.push_back(integer_var);
        } else if (sig_type == VT_STRING) {
            string_id = popS();
            string_ptr = stringById(string_id);
            auto string_var = new X86GpVar(compiler_.newGpVar(ASMJIT_PTR_TYPE, arg_var_name));
            compiler_.mov(*string_var, imm_ptr(string_ptr));
            vars.push_back(string_var);
        } else {
            throw std::logic_error("Invalid sig_type");
        }
    }
    
    auto callNode = compiler_.call(imm_ptr(address), native_prototype);
    for (uint32_t i = 0; i < vars.size(); ++i) {
        callNode->setArg(i, *vars[i]);
    }

    auto return_type = signature->at(0).first;

    if (return_type == VT_DOUBLE) {
        X86XmmVar return_value = compiler_.newXmmSd("return_value");
        callNode->setRet(0, return_value);
        compiler_.ret(return_value);
    } else if (return_type != VT_VOID) {
        asmjit::X86GpVar return_value;
        if (return_type == VT_INT) {
            return_value = compiler_.newGpVar(ASMJIT_INT_TYPE, "return_value");
        } else if (return_type == VT_STRING) {
            return_value = compiler_.newGpVar(ASMJIT_PTR_TYPE, "return_value");
        } else {
            throw std::logic_error("running native: Unknown return type.");
        }
        callNode->setRet(0, return_value);
        compiler_.ret(return_value);
    } else {
        compiler_.ret();
    }
    
    compiler_.endFunc();
    auto compiler_error = compiler_.finalize();
    if (compiler_error != kErrorOk) {
        throw std::logic_error("AsmjitCompiler failed: " + std::to_string(compiler_error));
    }
    
    void *generated_code = assembler_.make();
    if (generated_code == nullptr) {
        throw std::logic_error("Failed to call native: " + std::to_string(assembler_.getLastError()));
    }
    
    { // retrieve_value.
        uintptr_t sptr;
        string string_value;
        switch (signature->front().first) {
            case VT_DOUBLE:
                double_value = reinterpret_cast<DoubleReturnFunc>(generated_code)();
                push(double_value);
                break;
            case VT_INT:
                integer_value = reinterpret_cast<IntReturnFunc>(generated_code)();
                push(integer_value);
                break;
            case VT_STRING:
                sptr = reinterpret_cast<PtrReturnFunc>(generated_code)();
                string_id = makeStringNonConstant1(reinterpret_cast<char *>(sptr));
                push(string_id);
                break;
            case VT_VOID:
                reinterpret_cast<VoidReturnFunc>(generated_code)();
            default:
                break;
        }
    }
    for (auto const &var : vars) {
        delete var;
    }
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
