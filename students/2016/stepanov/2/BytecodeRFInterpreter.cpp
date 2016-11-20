/*  [Attention]
 * В интерператоре не поддерживаются только native-call. Если он обязателен, то он появится к следующему milestone.
 */


#include <cinttypes>
#include <cmath>
#include "BytecodeRFInterpreter.h"

mathvm::Status *mathvm::InterScope::getStatus() const {
    return status;
}

mathvm::Instruction mathvm::InterScope::next() {
    return bytecode->getInsn(IP++);
}

mathvm::InterScope::InterScope(mathvm::BytecodeFunction *bf, InterScope *parent) {
    this->bf = bf;
    this->parent = parent;
    variables.resize(bf->localsNumber(), STACK_EMPTY);
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
}

void mathvm::InterScope::skipUint16t() {
    IP += sizeof(uint16_t);
}

void mathvm::InterScope::skip2xUint16t() {
    IP += sizeof(uint16_t) + sizeof(uint16_t);
}


bool mathvm::BytecodeRFInterpreterCode::evaluateThis(mathvm::Instruction instr) {
    static StackItem top((int64_t) 0);
    static StackItem second((int64_t) 0);
    static StackItem *stackItemPtr;
    static InterScope *oldScope;
    static uint16_t scopeId;
    static uint16_t itemId;
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
            std::cout << constantById(stack.back().getStringId()).c_str();
            stack.pop_back();
            break;
        case BC_I2D:
            stack.back().value.doubleValue = (double) stack.back().value.intValue;
            stack.back().type = VT_DOUBLE;
            break;
        case BC_D2I:
            stack.back().value.intValue = (int) stack.back().value.doubleValue;
            stack.back().type = VT_INT;
            break;
        case BC_S2I:
            stack.back().value.intValue = (int) stack.back().value.stringIdValue;
            stack.back().type = VT_INT;
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
            stack.push_back(is->variables.front());
            break;
        case BC_LOADDVAR1:
        case BC_LOADIVAR1:
        case BC_LOADSVAR1:
            // .at() very slow =)
            stack.push_back(variablesPtr->operator[](1));
            break;
        case BC_LOADDVAR2:
        case BC_LOADIVAR2:
        case BC_LOADSVAR2:
            stack.push_back(variablesPtr->operator[](2));
            break;
        case BC_LOADDVAR3:
        case BC_LOADIVAR3:
        case BC_LOADSVAR3:
            stack.push_back(variablesPtr->operator[](3));
            break;
        case BC_STOREIVAR0:
        case BC_STORESVAR0:
        case BC_STOREDVAR0:
            variablesPtr->front() = stack.back();
            stack.pop_back();
            break;
        case BC_STOREDVAR1:
        case BC_STOREIVAR1:
        case BC_STORESVAR1:
            variablesPtr->operator[](1) = stack.back();
            stack.pop_back();
            break;
        case BC_STORESVAR2:
        case BC_STOREDVAR2:
        case BC_STOREIVAR2:
            variablesPtr->operator[](2) = stack.back();
            variablesPtr->at(2) = stack.back();
            stack.pop_back();
            break;
        case BC_STOREDVAR3:
        case BC_STOREIVAR3:
        case BC_STORESVAR3:
            variablesPtr->operator[](3) = stack.back();
            stack.pop_back();
            break;
        case BC_LOADDVAR:
        case BC_LOADIVAR:
        case BC_LOADSVAR:
            itemId = is->nextUint16t();
            stack.push_back(variablesPtr->operator[](itemId));
            break;
        case BC_STOREDVAR:
        case BC_STOREIVAR:
        case BC_STORESVAR:
            itemId = is->nextUint16t();
            variablesPtr->operator[](itemId) = stack.back();
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
            stack.back().type = VT_INT;
            if (top.value.doubleValue < stack.back().value.doubleValue) {
                stack.back().value.intValue = -1;
            } else if (fabs(top.value.doubleValue - stack.back().value.doubleValue) <= 1E-10) {
                stack.back().value.intValue = 0;
            } else {
                stack.back().value.intValue = 1;
            }
            break;
        case BC_ICMP:
            top = stack.back();
            stack.pop_back();
            if (top.value.intValue < stack.back().value.intValue) {
                stack.back().value.intValue = -1;
            } else if (top.value.intValue == stack.back().value.intValue) {
                stack.back().value.intValue = 0;
            } else {
                stack.back().value.intValue = 1;
            }
            break;
        case BC_JA:
            is->jump();
            break;
        case BC_IFICMPNE:
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            if (top.value.intValue != second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPE:
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            if (top.value.intValue == second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPG:
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            if (top.value.intValue > second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPGE:
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            if (top.value.intValue >= second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPL:
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

            if (top.value.intValue < second.value.intValue) {
                is->jump();
            } else {
                is->skipUint16t();
            }
            break;
        case BC_IFICMPLE:
            top = stack.back();
            stack.pop_back();
            second = stack.back();
            stack.pop_back();

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
            itemId = is->nextUint16t();
            is = new InterScope((BytecodeFunction *) functionById(itemId), is);
            variablesPtr = &(is->variables);
            break;
        case BC_CALLNATIVE:
            std::cerr << ("native calls in interpretator are not supported yet");
            assert(false);
            break;
        case BC_RETURN:
            oldScope = is;
            is = is->parent;
            variablesPtr = &(is->variables);
            delete (oldScope);
            if (is == nullptr) {
                return false;
            }
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

