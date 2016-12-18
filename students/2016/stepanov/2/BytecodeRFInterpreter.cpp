#include <cinttypes>
#include <cmath>
#include "BytecodeRFInterpreter.h"
#include "../../../../libs/asmjit/asmjit.h"
#include <stack>

#ifdef MY_DEBUG
    #include "../1/StringUtils.h"
#endif

using namespace asmjit;
using namespace asmjit::x86;


namespace mathvm {
    std::stack<size_t> scopeOffsets[UINT16_MAX];
    const int MAX_RARGS = 6;
    const int MAX_XMM = 8;

    inline int countRegisterArguments(const Signature &signature, int &rArgsCnt, int &xmmCnt) {
        for (size_t i = 1; i < signature.size(); ++i) {
            rArgsCnt += signature[i].first != VT_DOUBLE;
            xmmCnt += signature[i].first == VT_DOUBLE;
        }
        rArgsCnt = std::min(rArgsCnt, MAX_RARGS);
        xmmCnt = std::min(xmmCnt, MAX_XMM);
        return rArgsCnt + xmmCnt;
    }

    X86GpReg rRegisters[] = {rdi, rsi, rdx, rcx, r8, r9};
    X86XmmReg xmmRegisters[] = {xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7};

    JitRuntime runtime;

    nativePtr buildNativeFunction(const Signature &signature, const void *call) {
        X86Assembler assembler(&runtime);
        //FileLogger logger(stdout);
        //assembler.setLogger(&logger);
        int rArgsCnt = 0, xmmCnt = 0;
        const int registersSize = countRegisterArguments(signature, rArgsCnt, xmmCnt);
        size_t stackSize = signature.size() - registersSize - 1;
        assembler.push(rbp);
        assembler.mov(rbp, rsp);
        assembler.push(r12);
        assembler.mov(rax, rdi); // we can clean

        int indexRarg = 0;
        int indexXmm = 0;

        if (stackSize & 1LL) {
            assembler.sub(rsp, 8 * (stackSize));
        } else {
            assembler.sub(rsp, 8 * (stackSize + 1)); // Stack aligned on 16 bytes boundary.
        }
        assembler.lea(r10, ptr(rsp));

        for (uint64_t i = 2; i <= signature.size(); ++i) {
            if ((signature[i - 1].first == VT_DOUBLE)) {
                if (indexXmm < MAX_XMM) {
                    assembler.movsd(xmmRegisters[indexXmm], ptr(rax));
                    if ((indexXmm == 0) && (xmmCnt >= 8)) {
                        assembler.mov(r11, rax);
                    }
                    ++indexXmm;
                } else {
                    assembler.movsd(xmm0, ptr(rax));
                    assembler.movsd(ptr(r10), xmm0);
                    assembler.add(r10, 8);
                }
            } else if ((signature[i - 1].first != VT_DOUBLE) && (indexRarg < MAX_RARGS)) {
                assembler.mov(rRegisters[indexRarg], ptr(rax));
                ++indexRarg;
            } else {
                assembler.mov(r12, ptr(rax));
                assembler.mov(ptr(r10), r12);
                assembler.add(r10, 8);
            }
            assembler.sub(rax, 8);
        }

        if (xmmCnt >= 8) {
            assembler.movsd(xmm0, ptr(r11));
        }
        assembler.mov(r11, imm_ptr((void *) call));
        assembler.mov(rax, xmmCnt);
        assembler.call(r11);
        if (stackSize & 1LL) {
            assembler.add(rsp, 8 * (stackSize));
        } else {
            assembler.add(rsp, 8 * (stackSize + 1));
        }

        assembler.pop(r12);
        assembler.leave();
        assembler.ret();
        return reinterpret_cast<int64_t (*)(int64_t *)>(assembler.make());
    }

}

std::vector<mathvm::StackItem> mathvm::variables;

mathvm::Instruction mathvm::InterScope::next() {
    return bytecode->getInsn(IP++);
}

mathvm::InterScope::InterScope(mathvm::BytecodeFunction *bf, InterScope *parent) {
    this->bf = bf;
    this->parent = parent;
    variableOffset = variables.size();
    variables.resize(variableOffset + bf->localsNumber(), STACK_EMPTY);
    scopeOffsets[bf->scopeId()].push(variableOffset);
    bytecode = bf->bytecode();
}

uint16_t mathvm::InterScope::nextUint16t() {
    const uint32_t currentIP = IP;
    IP += sizeof(uint16_t);
    return bytecode->getUInt16(currentIP);
}

int64_t mathvm::InterScope::nextInt() {
    const uint32_t currentIP = IP;
    IP += sizeof(int64_t);
    return bytecode->getInt64(currentIP);
}

double mathvm::InterScope::nextDouble() {
    const uint32_t currentIP = IP;
    IP += sizeof(double);
    return bytecode->getDouble(currentIP);
}

void mathvm::InterScope::jump() {
    const uint32_t currentIP = IP;
    IP += bytecode->getInt16(currentIP);
    #ifdef MY_DEBUG
        std::cout << (currentIP-1) << "|: jump to " << IP << std::endl;
    #endif
}

void mathvm::InterScope::skipUint16t() {
    IP += sizeof(uint16_t);
}

mathvm::InterScope::~InterScope() {
    variables.resize(variableOffset);
    scopeOffsets[bf->scopeId()].pop();
    delete(status);
}

bool mathvm::BytecodeRFInterpreterCode::evaluateThis(mathvm::Instruction instr) {
    static StackItem top((int64_t) 0);
    static StackItem second((int64_t) 0);
    static StackItem *stackItemPtr;
    static InterScope *oldScope;
    static uint16_t scopeId;
    static uint16_t itemId;
    int64_t *ptr;
    static const Signature *signature;
    static const string *name;
    static const void *call;
    static nativePtr target;
    switch (instr) {
        case BC_DLOAD:
            stack.push_back(StackItem(is->nextDouble()));
            break;
        case BC_ILOAD:
            stack.push_back(StackItem(is->nextInt()));
            break;
        case BC_SLOAD:
            stack.push_back(StackItem(is->nextUint16t()));
            break;
        case BC_DLOAD0:
            stack.push_back(StackItem(0.0));
            break;
        case BC_ILOAD0:
            stack.push_back(StackItem((int64_t) 0));
            break;
        case BC_SLOAD0:
            stack.push_back(StackItem(emptyString()));
            break;
        case BC_DLOAD1:
            stack.push_back(StackItem(1.0));
            break;
        case BC_ILOAD1:
            stack.push_back(StackItem((int64_t) 1));
            break;
        case BC_DLOADM1:
            stack.push_back(StackItem(-1.0));
            break;
        case BC_ILOADM1:
            stack.push_back(StackItem((int64_t) -1));
            break;
        case BC_DADD:
            top = stack.back();
            stack.pop_back();
            stack.back().value.doubleValue += top.value.doubleValue;
            break;
        case BC_IADD:
            top = stack.back();
            stack.pop_back();
            stack.back().value.intValue += top.value.intValue;
            break;
        case BC_DSUB:
            top = stack.back();
            stack.pop_back();
            stack.back().value.doubleValue = top.value.doubleValue - stack.back().value.doubleValue;
            break;
        case BC_ISUB:
            top = stack.back();
            stack.pop_back();
            stack.back().value.intValue = top.value.intValue - stack.back().value.intValue;
            break;
        case BC_DMUL:
            top = stack.back();
            stack.pop_back();
            stack.back().value.doubleValue *= top.value.doubleValue;
            break;
        case BC_IMUL:
            top = stack.back();
            stack.pop_back();
            stack.back().value.intValue *= top.value.intValue;
            break;
        case BC_DDIV:
            top = stack.back();
            stack.pop_back();
            stack.back().value.doubleValue = top.value.doubleValue / stack.back().value.doubleValue;
            break;
        case BC_IDIV:
            top = stack.back();
            stack.pop_back();
            stack.back().value.intValue = top.value.intValue / stack.back().value.intValue;
            break;
        case BC_IMOD:
            top = stack.back();
            stack.pop_back();
            stack.back().value.intValue = top.value.intValue % stack.back().value.intValue;
            break;
        case BC_DNEG:
            stack.back().value.doubleValue = 0 - stack.back().value.doubleValue;
            break;
        case BC_INEG:
            stack.back().value.intValue = 0 - stack.back().value.intValue;
            break;
        case BC_IAOR:
            top = stack.back();
            stack.pop_back();
            stack.back().value.intValue = top.value.intValue | stack.back().value.intValue;
            break;
        case BC_IAAND:
            top = stack.back();
            stack.pop_back();
            stack.back().value.intValue = top.value.intValue & stack.back().value.intValue;
            break;
        case BC_IAXOR:
            top = stack.back();
            stack.pop_back();
            stack.back().value.intValue = top.value.intValue ^ stack.back().value.intValue;
            break;
        case BC_IPRINT:
            //printf("%" PRId64, stack.back().value.intValue);
            std::cout << stack.back().value.intValue;
            stack.pop_back();
            break;
        case BC_DPRINT:
            //printf("%lf", stack.back().value.doubleValue);
            std::cout << stack.back().value.doubleValue;
            stack.pop_back();
            break;
        case BC_SPRINT:
            //printf("%s", constantById(stack.back().getStringId()).c_str());
            std::cout << (char *) getStringConstantPtrById(stack.back().value.stringIdValue);
            stack.pop_back();
            break;
        case BC_I2D:
            stack.back().value.doubleValue = (double) stack.back().value.intValue;
            break;
        case BC_D2I:
            stack.back().value.intValue = (int64_t) stack.back().value.doubleValue;
            break;
        case BC_S2I:
            stack.back().value.intValue = getStringConstantPtrById(stack.back().value.stringIdValue);
            break;
        case BC_SWAP:
            std::swap(stack.back(), stack[stack.size() - 2]);
            break;
        case BC_POP:
            stack.pop_back();
            break;
        case BC_LOADDVAR0:
        case BC_LOADIVAR0:
        case BC_LOADSVAR0:
            stack.push_back(variables[variablesOffset]);
            break;
        case BC_LOADDVAR1:
        case BC_LOADIVAR1:
        case BC_LOADSVAR1:
            // .at() very slow =)
            stack.push_back(variables[variablesOffset + 1]);
            break;
        case BC_LOADDVAR2:
        case BC_LOADIVAR2:
        case BC_LOADSVAR2:
            stack.push_back(variables[variablesOffset + 2]);
            break;
        case BC_LOADDVAR3:
        case BC_LOADIVAR3:
        case BC_LOADSVAR3:
            stack.push_back(variables[variablesOffset + 3]);
            break;
        case BC_STOREIVAR0:
        case BC_STORESVAR0:
        case BC_STOREDVAR0:
            variables[variablesOffset] = stack.back();
            stack.pop_back();
            break;
        case BC_STOREDVAR1:
        case BC_STOREIVAR1:
        case BC_STORESVAR1:
            variables[variablesOffset + 1] = stack.back();
            stack.pop_back();
            break;
        case BC_STORESVAR2:
        case BC_STOREDVAR2:
        case BC_STOREIVAR2:
            variables[variablesOffset + 2] = stack.back();
            stack.pop_back();
            break;
        case BC_STOREDVAR3:
        case BC_STOREIVAR3:
        case BC_STORESVAR3:
            variables[variablesOffset + 3] = stack.back();
            stack.pop_back();
            break;
        case BC_LOADDVAR:
        case BC_LOADIVAR:
        case BC_LOADSVAR:
            itemId = is->nextUint16t();
            stack.push_back(variables[variablesOffset + itemId]);
            break;
        case BC_STOREDVAR:
        case BC_STOREIVAR:
        case BC_STORESVAR:
            itemId = is->nextUint16t();
            variables[variablesOffset + itemId] = stack.back();
            stack.pop_back();
            break;
        case BC_LOADCTXDVAR:
        case BC_LOADCTXIVAR:
        case BC_LOADCTXSVAR:
            scopeId = is->nextUint16t();
            itemId = is->nextUint16t();
            stackItemPtr = is->variableLookup(scopeId, itemId);
            assert(stackItemPtr != nullptr);
            stack.push_back(StackItem(*stackItemPtr));
            break;
        case BC_STORECTXDVAR:
        case BC_STORECTXIVAR:
        case BC_STORECTXSVAR:
            scopeId = is->nextUint16t();
            itemId = is->nextUint16t();
            stackItemPtr = is->variableLookup(scopeId, itemId);
            assert(stackItemPtr != nullptr);
            *stackItemPtr = stack.back();
            stack.pop_back();
            break;
        case BC_DCMP:
            top = stack.back();
            stack.pop_back();
            if (top.value.doubleValue < stack.back().value.doubleValue) {
                stack.back().value.intValue = -1;
            } else if (fabs(top.value.doubleValue - stack.back().value.doubleValue) <= 1E-10) {
                stack.back().value.intValue = 0;
            } else {
                stack.back().value.intValue = 1;
            }
            break;
        case BC_ICMP:
            #ifdef MY_DEBUG
                std::cout << (is->IP - 1) << "|:";
            #endif
            top = stack.back();
            stack.pop_back();
            #ifdef MY_DEBUG
                std::cout << "compare " << top.value.intValue << " and " << stack.back().value.intValue;
            #endif
            if (top.value.intValue < stack.back().value.intValue) {
                stack.back().value.intValue = -1;
            } else if (top.value.intValue == stack.back().value.intValue) {
                stack.back().value.intValue = 0;
            } else {
                stack.back().value.intValue = 1;
            }
            #ifdef MY_DEBUG
                std::cout << " result=" << stack.back().value.intValue << std::endl;
            #endif
            break;
        case BC_JA:
            is->jump();
            break;
        case BC_IFICMPNE:
            #ifdef MY_DEBUG
                std::cout << (is->IP - 1) << "|:";
            #endif
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            #ifdef MY_DEBUG
                std::cout << top.value.intValue <<" != "<< second.value.intValue  <<": " << (top.value.intValue != second.value.intValue) << std::endl;
            #endif
            if (top.value.intValue != second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPE:
            #ifdef MY_DEBUG
                std::cout << (is->IP - 1) << "|:";
            #endif
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            #ifdef MY_DEBUG
                std::cout  << top.value.intValue <<" == "<< second.value.intValue  <<": " << (top.value.intValue == second.value.intValue) << std::endl;
            #endif
            if (top.value.intValue == second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPG:
            #ifdef MY_DEBUG
                std::cout << (is->IP - 1) << "|:";
            #endif
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            #ifdef MY_DEBUG
                std::cout << top.value.intValue <<" > "<< second.value.intValue  <<": " << (top.value.intValue > second.value.intValue) << std::endl;
            #endif

            if (top.value.intValue > second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPGE:
            #ifdef MY_DEBUG
                std::cout << (is->IP - 1) << "|:";
            #endif
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            #ifdef MY_DEBUG
                std::cout <<  top.value.intValue <<" >= "<< second.value.intValue  <<": " << (top.value.intValue >= second.value.intValue) << std::endl;
            #endif
            if (top.value.intValue >= second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPL:
            #ifdef MY_DEBUG
                std::cout << (is->IP - 1) << "|:";
            #endif
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            #ifdef MY_DEBUG
                std::cout << top.value.intValue <<" < "<< second.value.intValue  <<": " << (top.value.intValue < second.value.intValue) << std::endl;
            #endif
            if (top.value.intValue < second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPLE:
            #ifdef MY_DEBUG
                std::cout << (is->IP - 1) << "|:";
            #endif
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            #ifdef MY_DEBUG
                std::cout << top.value.intValue <<" <= "<< second.value.intValue  <<": " << (top.value.intValue <= second.value.intValue) << std::endl;
            #endif
            if (top.value.intValue <= second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_DUMP:
            stack.push_back(stack.back());
            break;
        case BC_STOP:
            return true;
            break;
        case BC_CALL:
            #ifdef MY_DEBUG
                std::cout << (is->IP - 1) << "|:";
            #endif
            itemId = is->nextUint16t();
            #ifdef MY_DEBUG
                std::cout <<   "call " << itemId << "[" <<  functionById(itemId)->name() << "]" <<std::endl;
            #endif
            is = new InterScope((BytecodeFunction *) functionById(itemId), is);
            variablesOffset = is->variableOffset;
            break;
        case BC_CALLNATIVE:
            #ifdef MY_DEBUG
                        std::cout << (is->IP - 1) << "|:";
            #endif
            itemId = is->nextUint16t();
            call = nativeById(itemId, &signature, &name);
            #ifdef MY_DEBUG
                std::cout << "native call " << itemId << "[" << *name << "]"<< std::endl;
            #endif

            if (nativeFunctions[itemId] == 0) {
                nativeFunctions[itemId] = buildNativeFunction(*signature, (void *) call);
            }

            target = nativeFunctions[itemId];

            for (uint64_t i = 1; i < signature->size(); ++i) {
                if (signature->operator[](i).first == VT_STRING) {
                    stack[stack.size() - i].value.intValue = getStringConstantPtrById(
                            stack[stack.size() - i].value.stringIdValue);
                }
                ++itemId;
            }

            ptr = &(stack.back().value.intValue);

            switch (signature->operator[](0).first) {
                case VT_VOID:
                    reinterpret_cast<nativeVoidPtr >(target)(ptr);
                    break;
                case VT_DOUBLE:
                    top.value.doubleValue = reinterpret_cast<nativeDoublePtr>(target)(ptr);
                    break;
                case VT_INT:
                    top.value.intValue = target(ptr);
                    break;
                case VT_STRING:
                    top.value.intValue = target(ptr);
                    second.value.intValue = top.value.intValue;
                    top.value.stringIdValue = makeStringConstant((char *) top.value.intValue);
                    registerStringConstantPtrById(top.value.stringIdValue, second.value.intValue);
                    break;
                case VT_INVALID:
                    break;
            }

            stack.resize(stack.size() - signature->size() + 1);
            if (signature->operator[](0).first != VT_VOID) {
                stack.push_back(top);
            }
            break;
        case BC_RETURN:
            oldScope = is;
            is = is->parent;
            delete (oldScope);
            if (is == nullptr) {
                return false;
            }
            variablesOffset = is->variableOffset;
            break;
        case BC_BREAK:
        case BC_LAST:
        case BC_INVALID:
        default:
            return false;
    }
    return true;
}


uint16_t mathvm::BytecodeRFInterpreterCode::emptyString() {
    static uint16_t emptyStringId = makeStringConstant("");
    return emptyStringId;
}

int64_t mathvm::nativeLinks[UINT16_MAX];
mathvm::nativePtr mathvm::nativeFunctions[UINT16_MAX];

int64_t mathvm::BytecodeRFInterpreterCode::getStringConstantPtrById(uint16_t id) {
    #ifdef MY_DEBUG
        std::cout << "get string " << id << " = [" ;
        int64_t ptr;
        if (nativeLinks[id] != 0) {
            ptr = nativeLinks[id];
        } else {
            ptr =  (int64_t) &(constantById(id)[0]);
        }
        std:: cout  << ptr << "] {" << utils::StringUtils::escapeString((char*) ptr) << "}" <<std::endl;
    #endif
    if (nativeLinks[id] != 0) {
        return nativeLinks[id];
    } else {
        return (int64_t) &(constantById(id)[0]);
    }
}

void mathvm::BytecodeRFInterpreterCode::registerStringConstantPtrById(uint16_t id, int64_t ptr) {
    #ifdef MY_DEBUG
        std::cout << "register string " << id << " = [" <<  ptr << "]" <<std::endl;
    #endif
    nativeLinks[id] = ptr;
}