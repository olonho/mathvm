#include "interpreter_impl.h"
#include <cstdio>
#include <inttypes.h>

namespace mathvm {

Status* InterpreterCodeImpl::execute(vector<Var*>& vars) {
    size_t functionCnt = 0;
    FunctionIterator it(this);
    while(it.hasNext()) {
        it.next();
        ++functionCnt;
    }
    lastFunctionFrame.resize(functionCnt);
    
    initNatives();

    bci = 0;
    bc = 0;
    currentFrame = 0;
    currentFunId = 0;
    callFunction(0);
    run();

    return Status::Ok();
}

void InterpreterCodeImpl::initNatives() {
    NativeCallBuilder builder;
    NativeFunctionIterator it(this);

    while(it.hasNext()) {
        nativeCall cur = builder.makeCall(it.next());
        natives.push_back(cur);
    }
}

void InterpreterCodeImpl::push(Value val) {
    stack.push_back(val);
}

Value InterpreterCodeImpl::pop() {
    Value res = stack.back();

    stack.pop_back();
    return res;
}

void InterpreterCodeImpl::popTwo(Value& left, Value& right) {
    left = pop();
    right = pop();
}

BytecodeFunction* InterpreterCodeImpl::functionById(uint16_t id) const {
    return (BytecodeFunction*) Code::functionById(id);
}

template<class T>
int InterpreterCodeImpl::cmp(T const & left, T const & right) {
    if(left == right) {
        return 0;
    }

    if(left < right) {
        return -1;
    }
    else {
        return 1;
    }
}

void InterpreterCodeImpl::run() {
    Value val, left, right;
    bool end = false;
    bool ret = false;

    while(!end && !callStack.empty()) {
    ret = false;
    while(!ret && bci < bc->length()) {
        Instruction insn = bc->getInsn(bci);
        size_t nextOffset;
        bytecodeName(insn, &nextOffset);
        
        switch(insn) {
            case BC_DLOAD:
            {
                val.doubleVal = bc->getDouble(bci + 1);
                push(val);
                break;
            }
            case BC_ILOAD:
            {
                val.intVal = bc->getInt64(bci + 1);
                push(val);
                break;
            }
            case BC_SLOAD:
            {
                uint16_t strId = bc->getInt16(bci + 1);
                val.stringVal = constantById(strId).c_str();
                push(val);
                break;
            }
            case BC_DLOAD0:
            {
                val.doubleVal = 0;
                push(val);
                break;
            }

            case BC_ILOAD0:
            {
                val.intVal = 0;
                push(val);
                break;
            }

            case BC_SLOAD0:
            {
                val.stringVal = 0;
                push(val);
                break;
            }

            case BC_DLOAD1:
            {
                val.doubleVal = 1;
                push(val);
                break;
            }

            case BC_ILOAD1:
            {
                val.intVal = 1;
                push(val);
                break;
            }

            case BC_DLOADM1:
            {
                val.doubleVal = -1;
                push(val);
                break;
            }

            case BC_ILOADM1:
            {
                val.intVal = -1;
                push(val);
                break;
            }

            case BC_DADD:
            {
                popTwo(left, right);
                val.doubleVal = left.doubleVal + right.doubleVal;
                push(val);
                break;
            }

            case BC_IADD:
            {
                popTwo(left, right);
                val.intVal = left.intVal + right.intVal;
                push(val);
                break;
            }

            case BC_DSUB:
            {
                popTwo(left, right);
                val.doubleVal = left.doubleVal - right.doubleVal;
                push(val);
                break;
            }

            case BC_ISUB:
            {
                popTwo(left, right);
                val.intVal = left.intVal - right.intVal;
                push(val);
                break;
            }

            case BC_DMUL:
            {
                popTwo(left, right);
                val.doubleVal = left.doubleVal * right.doubleVal;
                push(val);
                break;
            }

            case BC_IMUL:
            {
                popTwo(left, right);
                val.intVal = left.intVal * right.intVal;
                push(val);
                break;
            }

            case BC_DDIV:
            {
                popTwo(left, right);
                val.doubleVal = left.doubleVal / right.doubleVal;
                push(val);
                break;
            }

            case BC_IDIV:
            {
                popTwo(left, right);
                val.intVal = left.intVal / right.intVal;
                push(val);
                break;
            }

            case BC_IMOD:
            {
                popTwo(left, right);
                val.intVal = left.intVal % right.intVal;
                push(val);
                break;
            }

            case BC_DNEG:
            {
                val.doubleVal = -pop().doubleVal;
                push(val);
                break;
            }

            case BC_INEG:
            {
                val.intVal = -pop().intVal;
                push(val);
                break;
            }

            case BC_IAOR:
            {
                popTwo(left, right);
                val.intVal = left.intVal | right.intVal;
                push(val);
                break;
            }

            case BC_IAAND:
            {
                popTwo(left, right);
                val.intVal = left.intVal & right.intVal;
                push(val);
                break;
            }

            case BC_IAXOR:
            {
                popTwo(left, right);
                val.intVal = left.intVal ^ right.intVal;
                push(val);
                break;
            }

            case BC_IPRINT:
            {
                val = pop();
                cout << val.intVal;
                // printf("%" PRId64 "", val.intVal);
                break;
            }

            case BC_DPRINT:
            {
                val = pop();
                cout << val.doubleVal;
                // printf("%f", val.doubleVal);
                break;
            }

            case BC_SPRINT:
            {
                val = pop();
                cout << val.stringVal;
                // printf("%s", val.stringVal);
                break;
            }

            case BC_I2D:
            {
                val.doubleVal = static_cast<double>(pop().intVal);
                push(val);
                break;
            }

            case BC_D2I:
            {
                val.intVal = static_cast<int64_t>(pop().doubleVal);
                push(val);
                break;
            }

            case BC_S2I:
            {
                // nop
                break;
            }

            case BC_SWAP:
            {
                popTwo(left, right);
                push(left);
                push(right);
                break;
            }

            case BC_POP:
            {
                pop();
                break;
            }

            case BC_LOADDVAR0:
            case BC_LOADIVAR0:
            case BC_LOADSVAR0:
            {
                val = stack[currentFrame + 0];
                push(val);
                break;
            }

            case BC_LOADDVAR1:
            case BC_LOADIVAR1:
            case BC_LOADSVAR1:
            {
                val = stack[currentFrame + 1];
                push(val);
                break;
            }

            case BC_LOADDVAR2:
            case BC_LOADIVAR2:
            case BC_LOADSVAR2:
            {
                val = stack[currentFrame + 2];
                push(val);
                break;
            }

            case BC_LOADDVAR3:
            case BC_LOADIVAR3:
            case BC_LOADSVAR3:
            {
                val = stack[currentFrame + 3];
                push(val);
                break;
            }


            case BC_STOREDVAR0:
            case BC_STOREIVAR0:
            case BC_STORESVAR0:
            {
                stack[currentFrame + 0] = pop();
                break;
            }

            case BC_STOREDVAR1:
            case BC_STOREIVAR1:
            case BC_STORESVAR1:
            {
                stack[currentFrame + 1] = pop();
                break;
            }

            case BC_STOREDVAR2:
            case BC_STOREIVAR2:
            case BC_STORESVAR2:
            {
                stack[currentFrame + 2] = pop();
                break;
            }

            case BC_STOREDVAR3:
            case BC_STOREIVAR3:
            case BC_STORESVAR3:
            {
                stack[currentFrame + 3] = pop();
                break;
            }

            case BC_LOADDVAR:
            case BC_LOADIVAR:
            case BC_LOADSVAR:
            {
                uint16_t id = bc->getUInt16(bci + 1);
                val = stack[currentFrame + id];
                push(val);
                break;
            }

            case BC_STOREDVAR:
            case BC_STOREIVAR:
            case BC_STORESVAR:
            {
                uint16_t id = bc->getUInt16(bci + 1);
                stack[currentFrame + id] = pop();
                break;
            }

            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR:
            {
                uint16_t funId = bc->getUInt16(bci + 1);
                uint16_t varId = bc->getUInt16(bci + 3);
                int frame = lastFunctionFrame[funId];

                val = stack[frame + varId];
                push(val);
                break;
            }


            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR:
            {
                uint16_t funId = bc->getUInt16(bci + 1);
                uint16_t varId = bc->getUInt16(bci + 3);
                int frame = lastFunctionFrame[funId];

                stack[frame + varId] = pop();
                break;
            }

            case BC_DCMP:
            {
                popTwo(left, right);
                val.intVal = cmp(left.doubleVal, right.doubleVal);
                push(val);
                break;
            }

            case BC_ICMP:
            {
                popTwo(left, right);
                val.intVal = cmp(left.intVal, right.intVal);
                push(val);
                break;
            }

            case BC_JA:
            {
                nextOffset = bc->getInt16(bci + 1) + 1;
                break;
            }

            case BC_IFICMPNE:
            {
                popTwo(left, right);
                if(left.intVal != right.intVal) {
                    nextOffset = bc->getInt16(bci + 1) + 1;
                }
                break;
            }

            case BC_IFICMPE:
            {
                popTwo(left, right);
                if(left.intVal == right.intVal) {
                    nextOffset = bc->getInt16(bci + 1) + 1;
                }
                break;
            }

            case BC_IFICMPG:
            {
                popTwo(left, right);
                if(left.intVal > right.intVal) {
                    nextOffset = bc->getInt16(bci + 1) + 1;
                }
                break;
            }

            case BC_IFICMPGE:
            {
                popTwo(left, right);
                if(left.intVal >= right.intVal) {
                    nextOffset = bc->getInt16(bci + 1) + 1;
                }
                break;
            }

            case BC_IFICMPL:
            {
                popTwo(left, right);
                if(left.intVal < right.intVal) {
                    nextOffset = bc->getInt16(bci + 1) + 1;
                }
                break;
            }

            case BC_IFICMPLE:
            {
                popTwo(left, right);
                if(left.intVal <= right.intVal) {
                    nextOffset = bc->getInt16(bci + 1) + 1;
                }
                break;
            }

            case BC_DUMP:
            {
                // nop
                break;
            }

            case BC_STOP:
            {
                ret = true;
                end = true;
                break;
            }

            case BC_CALL:
            {
                uint16_t funId = bc->getUInt16(bci + 1);
                bci += nextOffset;
                callFunction(funId);
                nextOffset = 0;
                break;
            }

            case BC_CALLNATIVE:
            {
                uint16_t id = bc->getUInt16(bci + 1);
                callNative(id);
                break;
            }

            case BC_RETURN:
            {
                ret = true;
                break;
            }

            case BC_BREAK:
            {
                // nop
                break;
            }
            default:
                assert(false);
        }
    
        bci += nextOffset;
    } 

    if(!end) {
        retFunction();
    }
    } 
}

void InterpreterCodeImpl::callFunction(uint16_t funId) {
    BytecodeFunction* function = functionById(funId);

    RetRecord record(currentFunId, bci, lastFunctionFrame[funId]);
    callStack.push_back(record);

    currentFrame = stack.size() - function->parametersNumber();
    lastFunctionFrame[funId] = currentFrame;
    bci = 0;
    bc = function->bytecode();
    currentFunId = funId;
    
    stack.resize(stack.size() + function->localsNumber());
}

void InterpreterCodeImpl::retFunction() {
    RetRecord record = callStack.back();
    callStack.pop_back();
    
    BytecodeFunction* function = functionById(currentFunId);
    Value retVal;

    if(function->returnType() != VT_VOID) {
        retVal = pop();
    }

    stack.resize(currentFrame);
    
    lastFunctionFrame[currentFunId] = record.lastFrame;
    currentFunId = record.funId;
    bci = record.bci;
    bc = functionById(currentFunId)->bytecode();
    currentFrame = lastFunctionFrame[currentFunId];

    if(function->returnType() != VT_VOID) {
        push(retVal);
    }
}

void InterpreterCodeImpl::callNative(uint16_t id) {
    const Signature* sig;
    const string* name;

    nativeById(id, &sig, &name);
    
    size_t argOffset = stack.size() + 1 - sig->size();
    
    Value res;
    res.intVal = natives[id]((int64_t*)&stack[argOffset]);
    
    stack.resize(argOffset);

    if(sig->at(0).first != VT_VOID) {
        push(res);
    }
}
}
