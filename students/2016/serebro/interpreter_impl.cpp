//
// Created by andy on 11/22/16.
//

#include <vector>
#include <stack>
#include <algorithm>
#include "interpreter_impl.h"

#define POP2                                                        \
        AnonymousVar lhs = operation_stack.top();                   \
        operation_stack.pop();                                      \
        AnonymousVar rhs = operation_stack.top();                   \
        operation_stack.pop()

#define POP2_AND_PUSH_BINOP_RESULT_INT(OP)                              \
    do                                                                  \
    {                                                                   \
        POP2;                                                           \
        assert(lhs.type == VT_INT && rhs.type == VT_INT);               \
        lhs.intValue OP##= rhs.intValue;                                \
        operation_stack.push(lhs);                                      \
    } while(0)

#define POP2_AND_PUSH_BINOP_RESULT_DOUBLE(OP)                           \
    do                                                                  \
    {                                                                   \
        POP2;                                                           \
        assert(lhs.type == VT_DOUBLE && rhs.type == VT_DOUBLE);         \
        lhs.doubleValue OP##= rhs.doubleValue;                          \
        operation_stack.push(lhs);                                      \
    } while(0)

#define CASE_LOADDVAR(NUM)                                        \
    case BC_LOADDVAR##NUM:{                                       \
        operation_stack.push(currentScope->getDoubleValue(NUM));  \
        break;                                                    \
    }

#define CASE_LOADIVAR(NUM)                                      \
    case BC_LOADIVAR##NUM: {                                    \
        operation_stack.push(currentScope->getIntValue(NUM));   \
        break;                                                  \
    }

#define CASE_LOADSVAR(NUM)                                          \
    case BC_LOADSVAR##NUM:{                                         \
        operation_stack.push(currentScope->getStringValue(NUM));    \
        break;                                                      \
    }

#define CASE_STOREDVAR(NUM)                                                         \
    case BC_STOREDVAR##NUM:{                                                        \
        currentScope->setDoubleValue(NUM, operation_stack.top().getDoubleValue());  \
        operation_stack.pop();                                                      \
        break;                                                                      \
    }

#define CASE_STOREIVAR(NUM)                                                     \
    case BC_STOREIVAR##NUM:{                                                    \
        currentScope->setIntValue(NUM, operation_stack.top().getIntValue());    \
        operation_stack.pop();                                                  \
        break;                                                                  \
    }

#define CASE_STORESVAR(NUM)                                                          \
    case BC_STORESVAR##NUM:{                                                         \
        currentScope->setStringValue(NUM, operation_stack.top().getStringValue());   \
        operation_stack.pop();                                                       \
        break;                                                                       \
    }
#define FIND_SCOPE                                                      \
    uint16_t scopeId = currentCode->getUInt16(currentIndex);            \
    currentIndex += sizeof(uint16_t);                                   \
    uint16_t varId = currentCode->getUInt16(currentIndex);              \
    currentIndex += sizeof(uint16_t);                                   \
    assert(currentScope->depth >= scopeId);                             \
    BytecodeScope *neededScope = currentScope;                          \
    do {                                                                \
        for (uint16_t i = currentScope->depth; i != scopeId; i--) {     \
            neededScope = neededScope->parent;                          \
        }                                                               \
    } while (0)

#define CONDITIONAL_JUMP(COMP_OP)                                           \
    do {                                                                    \
        int16_t offset = currentCode->getInt16(currentIndex);               \
        POP2;                                                               \
        int64_t result = signum(lhs.getIntValue() - rhs.getIntValue());     \
        if (result COMP_OP 0) {                                             \
            currentIndex += offset;                                         \
        } else {                                                            \
            currentIndex += sizeof (int16_t);                               \
        }                                                                   \
    } while(0)

namespace mathvm
{

namespace {

template <class T>
int64_t signum(T val) {
    return (val > 0) - (val < 0);
}

}

Status* InterpreterCodeImpl::execute(vector<Var *> &vars) {
    BytecodeFunction *entry = dynamic_cast<BytecodeFunction*>(functionById(0));
    if (nullptr == entry) {
        throw logic_error{"Entry function not found"};
    }


    stack<uint32_t> return_address;
    stack<BytecodeFunction*> callStack;
    Bytecode* currentCode = entry->bytecode();
    BytecodeFunction* currentFunction = entry;
    uint32_t currentIndex = 0;
    stack<AnonymousVar> operation_stack;

    // add global variable binding
    BytecodeScope *currentScope = _info.funcToScope.at(0);
    stack<unordered_map<BytecodeScope*, size_t>> nextChildToVisitInScope; // stack because we can call functions recursive

    {
        nextChildToVisitInScope.push({});
        nextChildToVisitInScope.top().insert({currentScope, 0});
        currentScope->enter();
        for (auto var: vars) {
            uint16_t varId = _info.toplevelVarNameToId.at(var->name());
            currentScope->setValue(varId, *var);
        }
    }

    while (return_address.size() || currentIndex < currentCode->length()) {
        Instruction ins = currentCode->getInsn(currentIndex);
        currentIndex++; // every instruction occupies one byte
        switch (ins) {
            case BC_INVALID:
                throw logic_error{"Invalid instruction in byte code"};
            case BC_DLOAD: {
                operation_stack.push({currentCode->getDouble(currentIndex)});
                currentIndex += sizeof(double);
                break;
            }
            case BC_ILOAD: {
                operation_stack.push({currentCode->getInt64(currentIndex)});
                currentIndex += sizeof(int64_t);
                break;
            }
            case BC_SLOAD: {
                uint16_t stringId = currentCode->getUInt16(currentIndex);
                operation_stack.push({constantById(stringId).c_str()});
                currentIndex += sizeof(uint16_t);
                break;
            }
            case BC_DLOAD0: {
                operation_stack.push({0.0});
                break;
            }
            case BC_ILOAD0: {
                operation_stack.push({int64_t(0)});
                break;
            }
            case BC_SLOAD0: {
                operation_stack.push({constantById(0).c_str()});
                break;
            }
            case BC_DLOAD1: {
                operation_stack.push({1.0});
                break;
            }
            case BC_ILOAD1: {
                operation_stack.push({int64_t(1)});
                break;
            }
            case BC_DLOADM1:{
                operation_stack.push({-1.0});
                break;
            }
            case BC_ILOADM1:{
                operation_stack.push({int64_t(-1)});
                break;
            }
            case BC_DADD:{
                POP2_AND_PUSH_BINOP_RESULT_DOUBLE(+);
                break;
            }
            case BC_IADD:{
                POP2_AND_PUSH_BINOP_RESULT_INT(+);
                break;
            };
            case BC_DSUB:{
                POP2_AND_PUSH_BINOP_RESULT_DOUBLE(-);
                break;
            }
            case BC_ISUB:{
                POP2_AND_PUSH_BINOP_RESULT_INT(-);
                break;
            }
            case BC_DMUL:{
                POP2_AND_PUSH_BINOP_RESULT_DOUBLE(*);
                break;
            }
            case BC_IMUL:{
                POP2_AND_PUSH_BINOP_RESULT_INT(*);
                break;
            }
            case BC_DDIV:{
                POP2_AND_PUSH_BINOP_RESULT_DOUBLE(/);
                break;
            }
            case BC_IDIV:{
                POP2_AND_PUSH_BINOP_RESULT_INT(/);
                break;
            }
            case BC_IMOD:{
                POP2_AND_PUSH_BINOP_RESULT_INT(%);
                break;
            }
            case BC_DNEG:{
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                operation_stack.push(-val.getDoubleValue());
                break;
            }
            case BC_INEG:{
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                operation_stack.push(-val.getIntValue());
                break;
            }
            case BC_IAOR:{
                POP2_AND_PUSH_BINOP_RESULT_INT(|);
                break;
            }
            case BC_IAAND:{
                POP2_AND_PUSH_BINOP_RESULT_INT(&);
                break;
            }
            case BC_IAXOR:{
                POP2_AND_PUSH_BINOP_RESULT_INT(^);
                break;
            }
            case BC_IPRINT:{
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                cout << val.getIntValue();
                break;
            }
            case BC_DPRINT:{
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                cout << val.getDoubleValue();
                break;
            }
            case BC_SPRINT:{
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                cout << val.getStringValue();
                break;
            }
            case BC_I2D:{
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                operation_stack.push((double)val.getIntValue());
                break;
            }
            case BC_D2I:{
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                operation_stack.push((int64_t)val.getDoubleValue());
                break;
            }
            case BC_S2I:{
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();

                const char *strVal = val.getStringValue();
                if (!strVal) {
                    // using uninitialized variable
                    // depending on run flags, it should be either silently eaten or errored or warned
                    // for now we will silently eat it, since no proper initialization check
                    // is performed for other types
                    operation_stack.push((int64_t)0); // assume it is initialized to ""; Anyway, it's UB, who cares
                    break;
                }

                operation_stack.push((int64_t)makeStringConstant(strVal));
                break;
            }
            case BC_SWAP: {
                POP2;
                operation_stack.push(lhs);
                operation_stack.push(rhs);
                break;
            }
            case BC_POP:{
                operation_stack.pop();
                break;
            }
            CASE_LOADDVAR(0)
            CASE_LOADDVAR(1)
            CASE_LOADDVAR(2)
            CASE_LOADDVAR(3)
            CASE_LOADIVAR(0)
            CASE_LOADIVAR(1)
            CASE_LOADIVAR(2)
            CASE_LOADIVAR(3)
            CASE_LOADSVAR(0)
            CASE_LOADSVAR(1)
            CASE_LOADSVAR(2)
            CASE_LOADSVAR(3)
            CASE_STOREDVAR(0)
            CASE_STOREDVAR(1)
            CASE_STOREDVAR(2)
            CASE_STOREDVAR(3)
            CASE_STOREIVAR(0)
            CASE_STOREIVAR(1)
            CASE_STOREIVAR(2)
            CASE_STOREIVAR(3)
            CASE_STORESVAR(0)
            CASE_STORESVAR(1)
            CASE_STORESVAR(2)
            CASE_STORESVAR(3)
            case BC_LOADDVAR:{
                uint16_t id = currentCode->getUInt16(currentIndex);
                currentIndex += sizeof(uint16_t);
                operation_stack.push(currentScope->getDoubleValue(id));
                break;
            }
            case BC_LOADIVAR: {
                uint16_t id = currentCode->getUInt16(currentIndex);
                currentIndex += sizeof(uint16_t);
                operation_stack.push(currentScope->getIntValue(id));
                break;
            }
            case BC_LOADSVAR: {
                uint16_t id = currentCode->getUInt16(currentIndex);
                currentIndex += sizeof(uint16_t);
                operation_stack.push(currentScope->getStringValue(id));
                break;
            }
            case BC_STOREDVAR:{
                uint16_t id = currentCode->getUInt16(currentIndex);
                currentIndex += sizeof(uint16_t);
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                currentScope->setDoubleValue(id, val.getDoubleValue());
                break;
            }
            case BC_STOREIVAR:{
                uint16_t id = currentCode->getUInt16(currentIndex);
                currentIndex += sizeof(uint16_t);
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                currentScope->setIntValue(id, val.getIntValue());
                break;
            }
            case BC_STORESVAR:{
                uint16_t id = currentCode->getUInt16(currentIndex);
                currentIndex += sizeof(uint16_t);
                AnonymousVar val = operation_stack.top();
                operation_stack.pop();
                currentScope->setStringValue(id, val.getStringValue());
                break;
            }
            case BC_LOADCTXDVAR: {
                FIND_SCOPE;
                operation_stack.push(neededScope->getDoubleValue(varId));
                break;
            }
            case BC_LOADCTXIVAR: {
                FIND_SCOPE;
                operation_stack.push(neededScope->getIntValue(varId));
                break;
            }
            case BC_LOADCTXSVAR: {
                FIND_SCOPE;
                operation_stack.push(neededScope->getStringValue(varId));
                break;
            }
            case BC_STORECTXDVAR:{
                FIND_SCOPE;
                AnonymousVar var = operation_stack.top();
                operation_stack.pop();
                neededScope->setDoubleValue(varId, var.getDoubleValue());
                break;
            }
            case BC_STORECTXIVAR:{
                FIND_SCOPE;
                AnonymousVar var = operation_stack.top();
                operation_stack.pop();
                neededScope->setIntValue(varId, var.getIntValue());
                break;
            }
            case BC_STORECTXSVAR:{
                FIND_SCOPE;
                AnonymousVar var = operation_stack.top();
                operation_stack.pop();
                neededScope->setStringValue(varId, var.getStringValue());
                break;
            }
            case BC_DCMP: {
                POP2;
                int64_t result = signum(lhs.getDoubleValue() - rhs.getDoubleValue());
                operation_stack.push(result);
                break;
            }
            case BC_ICMP: {
                POP2;
                int64_t result = signum(lhs.getIntValue() - rhs.getIntValue());
                operation_stack.push(result);
                break;
            }
            case BC_JA: {
                int16_t offset = currentCode->getInt16(currentIndex);
                currentIndex += offset;
                break;
            }
            case BC_IFICMPNE: {
                CONDITIONAL_JUMP(!=);
                break;
            }
            case BC_IFICMPE:{
                CONDITIONAL_JUMP(==);
                break;
            }
            case BC_IFICMPG: {
                CONDITIONAL_JUMP(>);
                break;
            }
            case BC_IFICMPGE: {
                CONDITIONAL_JUMP(>=);
                break;
            }
            case BC_IFICMPL: {
                CONDITIONAL_JUMP(<);
                break;
            }
            case BC_IFICMPLE: {
                CONDITIONAL_JUMP(<=);
                break;
            }
            case BC_DUMP:break; // TODO
            case BC_STOP:break; // TODO
            case BC_CALL: {
                uint16_t funcId = currentCode->getUInt16(currentIndex);
                currentIndex += sizeof(uint16_t);
                return_address.push(currentIndex);
                currentIndex = 0;
                callStack.push(currentFunction);

                currentFunction = dynamic_cast<BytecodeFunction*>(functionById(funcId));
                currentScope = _info.funcToScope.at(currentFunction->id());
                currentScope->enter();
                nextChildToVisitInScope.push({});
                nextChildToVisitInScope.top().insert({currentScope, 0});

                for (int i = currentFunction->parametersNumber() - 1; i >= 0; i--) {
                    AnonymousVar var = operation_stack.top();
                    operation_stack.pop();

                    switch (currentFunction->parameterType(i)) {
                        case VT_DOUBLE:
                            currentScope->setDoubleValue(i, var.getDoubleValue());
                            break;
                        case VT_INT:
                            currentScope->setIntValue(i, var.getIntValue());
                            break;
                        case VT_STRING:
                            currentScope->setStringValue(i, var.getStringValue());
                            break;
                        default:
                            assert(false);
                    }
                }
                currentCode = currentFunction->bytecode();


                break;
            }
            case BC_CALLNATIVE:{
                uint16_t funcId = currentCode->getUInt16(currentIndex);
                currentIndex += sizeof(uint16_t);
                const Signature *sig;
                const string *name;
                nativeById(funcId, &sig, &name);
                // TODO
                break;
            }
            case BC_RETURN: {
                // pop all the nested scope variables
                BytecodeScope* rootScope = _info.funcToScope.at(currentFunction->id());
                while (currentScope != rootScope) {
                    currentScope->exit();
                    currentScope = currentScope->parent;
                }

                if (currentFunction->id() == 0) { // toplevel can have return <why not>, we just finish execution
                    goto FINISH;
                }
                rootScope->exit();

                currentIndex = return_address.top();
                return_address.pop();
                nextChildToVisitInScope.pop();
                
                currentFunction = callStack.top();
                callStack.pop();
                currentScope = _info.funcToScope.at(currentFunction->id());
                currentCode = currentFunction->bytecode();
                break;
            }
            case BC_BREAK:break; // I guess only debugger interpreter should bother with it
            default:
                assert(false);
        }

        while (currentScope && currentScope->_bytecodeEndIndex == currentIndex) {
            if (currentFunction->id() == 0 && !currentScope->parent) { // toplevel can have return <why not>, we just finish execution
                goto FINISH;
            }

            currentScope->exit();
            currentScope = currentScope->parent;
        }

        while (true) {
            size_t nextChild = nextChildToVisitInScope.top().at(currentScope);

            if (nextChild >= currentScope->children.size()
                || !currentScope->children[nextChild]->contains(currentIndex)) {
                break;
            }

            nextChildToVisitInScope.top()[currentScope]++;
            currentScope = currentScope->children[nextChild];
            nextChildToVisitInScope.top().insert({currentScope, 0});
            currentScope->enter();
        }
    }
FINISH:
    for (auto var: vars) {
        uint16_t varId = _info.toplevelVarNameToId.at(var->name());
        switch (var->type()) {
            case VT_DOUBLE:
                var->setDoubleValue(currentScope->getDoubleValue(varId));
                break;
            case VT_INT:
                var->setIntValue(currentScope->getIntValue(varId));
                break;
            case VT_STRING:
                var->setStringValue(currentScope->getStringValue(varId));
                break;
            default:
                assert(false);
        }
    }

    return Status::Ok();
}

}

#undef POP2
#undef POP2_AND_PUSH_BINOP_RESULT_DOUBLE
#undef POP2_AND_PUSH_BINOP_RESULT_INT
#undef CASE_LOADDVAR
#undef CASE_LOADIVAR
#undef CASE_LOADSVAR
#undef CASE_STOREDVAR
#undef CASE_STOREIVAR
#undef CASE_STORESVAR
#undef FIND_SCOPE
#undef CONDITIONAL_JUMP