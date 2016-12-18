//
// Created by dsavvinov on 13.11.16.
//

#include <ast.h>
#include "Interpreter.h"

namespace mathvm {

Status * Interpreter::executeProgram() {
    BytecodeFunction * topFunction = (BytecodeFunction *) code->functionByName(AstFunction::top_name);
    // TODO: get passed vars
    executeFunction(topFunction);
    return Status::Ok();
}

void Interpreter::executeFunction(BytecodeFunction * function) {
    Bytecode * bc = function->bytecode();
    enterFunction(function);
    uint32_t i = 0;
    while (i < bc->length()) {
        Instruction insn = bc->getInsn(i);
        switch(insn) {
            case BC_INVALID: throw ExecutionException("Found BC_INVALID");
            case BC_DLOAD:
                push(bc->getDouble(++i));
                i+= 8;
                break;
            case BC_ILOAD:
                push(bc->getInt64(++i));
                i+= 8;
                break;
            case BC_SLOAD:
                push(code->constantById(bc->getUInt16(++i)));
                i+= 2;
                break;
            case BC_DLOAD0:
                push(0.0);
                ++i;
                break;
            case BC_ILOAD0:
                push(0L);
                ++i;
                break;
            case BC_SLOAD0:
                push("");
                ++i;
                break;
            case BC_DLOAD1:
                push(1.0);
                ++i;
                break;
            case BC_ILOAD1:
                push(1L);
                ++i;
                break;
            case BC_DLOADM1:
                push(-1.0);
                ++i;
                break;
            case BC_ILOADM1:
                push(-1L);
                ++i;
                break;
            case BC_DADD: {
                double up = topDouble();
                pop();
                double low = topDouble();
                pop();
                push(up + low);
                ++i;
                break;
            }
            case BC_IADD: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();

                push(up + low);
                ++i;
                break;
            }
            case BC_DSUB:{
                double up = topDouble();
                pop();
                double low = topDouble();
                pop();
                push(up - low);
                ++i;
                break;
            }
            case BC_ISUB: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();

                push(up - low);
                ++i;
                break;
            }
            case BC_DMUL: {
                double up = topDouble();
                pop();
                double low = topDouble();
                pop();

                push(up * low);
                ++i;
                break;
            }
            case BC_IMUL: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();

                push(up * low);
                ++i;
                break;
            }
            case BC_DDIV: {
                double up = topDouble();
                pop();
                double low = topDouble();
                pop();

                push(up / low);
                ++i;
                break;
            }
            case BC_IDIV: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();

                push(up / low);
                ++i;
                break;
            }
            case BC_IMOD: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();

                push(up % low);
                ++i;
                break;
            };
            case BC_DNEG: {
                double up = topDouble();
                pop();
                push (-up);
                ++i;
                break;
            }
            case BC_INEG: {
                int64_t up = topInt();
                pop();
                push (-up);
                ++i;
                break;
            }
            case BC_IAOR: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();

                push (up | low);
                ++i;
                break;
            }
            case BC_IAAND: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();

                push (up & low);
                ++i;
                break;
            }
            case BC_IAXOR: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();

                push (up ^ low);
                ++i;
                break;
            }
            case BC_IPRINT: {
                int64_t up = topInt();
                pop();

                std::cout << up;
                ++i;
                break;
            }
            case BC_DPRINT: {
                double up = topDouble();
                pop();

                std::cout << up;
                ++i;
                break;
            }
            case BC_SPRINT: {
                string * s = topString();
                std:: cout << *s;
                pop();
                ++i;
                break;
            };
            case BC_I2D: {
                int64_t up = topInt();
                pop();

                push (static_cast<double>(up));
                ++i;
                break;
            };
            case BC_D2I: {
                double up = topDouble();
                pop();

                push(static_cast<int64_t>(up));
                ++i;
                break;
            }
            case BC_S2I: {
                string * up = topString();
                push(*reinterpret_cast<int64_t *>(up));
                ++i;
                break;
            };
            case BC_SWAP: {
                Data up = topData();
                pop();
                Data low = topData();
                pop();

                pushData(up);
                pushData(low);
                ++i;
                break;
            };
            case BC_POP:
                pop();
                ++i;
                break;
            case BC_LOADDVAR0:
                push(getLocalData(0).getDouble());
                ++i;
                break;
            case BC_LOADDVAR1:
                push(getLocalData(1).getDouble());
                ++i;
                break;
            case BC_LOADDVAR2:
                push(getLocalData(2).getDouble());
                ++i;
                break;
            case BC_LOADDVAR3:
                push(getLocalData(3).getDouble());
                ++i;
                break;
            case BC_LOADIVAR0:
                push(getLocalData(0).getInt());
                ++i;
                break;
            case BC_LOADIVAR1:
                push(getLocalData(1).getInt());
                ++i;
                break;
            case BC_LOADIVAR2:
                push(getLocalData(2).getInt());
                ++i;
                break;
            case BC_LOADIVAR3:
                push(getLocalData(3).getInt());
                ++i;
                break;
            case BC_LOADSVAR0:
                push(*getLocalData(0).getString());
                ++i;
                break;
            case BC_LOADSVAR1:
                push(*getLocalData(1).getString());
                ++i;
                break;
            case BC_LOADSVAR2:
                push(*getLocalData(2).getString());
                ++i;
                break;
            case BC_LOADSVAR3:
                push(*getLocalData(3).getString());
                ++i;
                break;
            case BC_STOREDVAR0: {
                double up = topDouble();
                setLocalData(up, 0);
                pop();
                ++i;
                break;
            }
            case BC_STOREDVAR1: {
                double up = topDouble();
                setLocalData(up, 1);
                pop();
                ++i;
                break;
            }
            case BC_STOREDVAR2: {
                double up = topDouble();
                setLocalData(up, 2);
                pop();
                ++i;
                break;
            }
            case BC_STOREDVAR3: {
                double up = topDouble();
                setLocalData(up, 3);
                pop();
                ++i;
                break;
            }
            case BC_STOREIVAR0: {
                int64_t up = topInt();
                setLocalData(up, 0);
                pop();
                ++i;
                break;
            }
            case BC_STOREIVAR1:{
                int64_t up = topInt();
                setLocalData(up, 1);
                pop();
                ++i;
                break;
            }
            case BC_STOREIVAR2:{
                int64_t up = topInt();
                setLocalData(up, 2);
                pop();
                ++i;
                break;
            }
            case BC_STOREIVAR3:{
                int64_t up = topInt();
                setLocalData(up, 3);
                pop();
                ++i;
                break;
            }
            case BC_STORESVAR0: {
                string * up = topString();
                setLocalData(*up, 0);
                pop();
                ++i;
                break;
            }
            case BC_STORESVAR1:{
                string * up = topString();
                setLocalData(*up, 1);
                pop();
                ++i;
                break;
            }
            case BC_STORESVAR2:{
                string * up = topString();
                setLocalData(*up, 2);
                pop();
                ++i;
                break;
            }
            case BC_STORESVAR3:{
                string * up = topString();
                setLocalData(*up, 3);
                pop();
                ++i;
                break;
            }
            case BC_LOADDVAR: {
                uint16_t id = bc->getUInt16(++i);
                const Data &data = getLocalData(id);
                push(*data.doubleValue);
                i += 2;
                break;
            }
            case BC_LOADIVAR: {
                uint16_t id = bc->getUInt16(++i);
                const Data &data = getLocalData(id);
                push(*data.intValue);
                i += 2;
                break;
            }
            case BC_LOADSVAR: {
                uint16_t id = bc->getUInt16(++i);
                const Data &data = getLocalData(id);
                push(*data.stringValue);
                i += 2;
                break;
            }
            case BC_STOREDVAR: {
                double up = topDouble();
                pop();
                uint16_t id = bc->getUInt16(++i);
                setLocalData(up, id);
                i += 2;
                break;
            }
            case BC_STOREIVAR: {
                int64_t up = topInt();
                pop();
                uint16_t id = bc->getUInt16(++i);
                setLocalData(up, id);
                i += 2;
                break;
            }
            case BC_STORESVAR: {
                string * up = topString();
                uint16_t  id = bc->getUInt16(++i);
                setLocalData(*up, id);
                pop();
                i += 2;
                break;
            }
            case BC_LOADCTXDVAR: {
                ++i;
                uint16_t ctxID = bc->getUInt16(i);
                i += 2;
                uint16_t varID = bc->getUInt16(i);
                i += 2;

                Data data = getData(ctxID, varID);
                push(data.getDouble());
                break;
            }
            case BC_LOADCTXIVAR: {
                ++i;
                uint16_t ctxID = bc->getUInt16(i);
                i += 2;
                uint16_t varID = bc->getUInt16(i);
                i += 2;

                Data data = getData(ctxID, varID);
                push(data.getInt());
                break;
            }
            case BC_LOADCTXSVAR: {
                ++i;
                uint16_t ctxID = bc->getUInt16(i);
                i += 2;
                uint16_t varID = bc->getUInt16(i);
                i += 2;

                Data data = getData(ctxID, varID);
                push(*data.getString());
                break;
            }
            case BC_STORECTXDVAR: {
                ++i;
                uint16_t ctxID = bc->getUInt16(i);
                i += 2;
                uint16_t varID = bc->getUInt16(i);
                i += 2;

                double top = topDouble();
                setData(top, ctxID, varID);
                break;
            }
            case BC_STORECTXIVAR: {
                ++i;
                uint16_t ctxID = bc->getUInt16(i);
                i += 2;
                uint16_t varID = bc->getUInt16(i);
                i += 2;

                int64_t top = topInt();
                setData(top, ctxID, varID);
                break;
            }
            case BC_STORECTXSVAR: {
                ++i;
                uint16_t ctxID = bc->getUInt16(i);
                i += 2;
                uint16_t varID = bc->getUInt16(i);
                i += 2;

                string *top = topString();
                setData(*top, ctxID, varID);
                break;
            }
            case BC_DCMP: {
                double up = topDouble();
                pop();
                double low = topDouble();
                pop();

                if (up < low) {
                    push (-1L);
                } else if (up > low) {
                    push(1L);
                } else {
                    push(0L);
                }
                ++i;
                break;
            }
            case BC_ICMP: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();

                if (up < low) {
                    push (-1L);
                } else if (up > low) {
                    push(1L);
                } else {
                    push(0L);
                }
                ++i;
                break;
            }
            case BC_JA: {
                int16_t offset = bc->getInt16(++i);
                i += offset;
                break;
            }
            case BC_IFICMPNE: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();
                int16_t offset = bc->getInt16(++i);
                if (up != low) {
                    i += offset;
                } else {
                    i += 2;
                }
                break;
            }
            case BC_IFICMPE: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();
                int16_t offset = bc->getInt16(++i);
                if (up == low) {
                    i += offset;
                } else {
                    i += 2;
                }
                break;
            }
            case BC_IFICMPG:  {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();
                int16_t offset = bc->getInt16(++i);
                if (up > low) {
                    i += offset;
                } else {
                    i += 2;
                }
                break;
            }
            case BC_IFICMPGE: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();
                int16_t offset = bc->getInt16(++i);
                if (up >= low) {
                    i += offset;
                } else {
                    i += 2;
                }
                break;
            }
            case BC_IFICMPL: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();
                int16_t offset = bc->getInt16(++i);
                if (up < low) {
                    i += offset;
                } else {
                    i += 2;
                }
                break;
            }
            case BC_IFICMPLE: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();
                int16_t offset = bc->getInt16(++i);
                if (up <= low) {
                    i += offset;
                } else {
                    i += 2;
                }
                break;
            }
            case BC_DUMP:
                throw ExecutionException("bc_dump not implemented yet");
            case BC_STOP:
                return;
            case BC_CALL: {
                int16_t id = bc->getInt16(++i);
                executeFunction((BytecodeFunction *) code->functionById((uint16_t) id));
                i += 2;
                break;
            }
            case BC_CALLNATIVE: {
                throw ExecutionException("call native not implemented yet");
            }
            case BC_RETURN: {
                exitFunction();
                return;
            };
            case BC_BREAK:
                throw ExecutionException("bc_break not implemented yet");
            case BC_LAST:
                throw ExecutionException("BC_LAST found");
        }
    }
    exitFunction();
}

Data Interpreter::getData(uint16_t ctxID, uint16_t varID) {
    return storage[ctxID].back()[varID];
}

Data Interpreter::getLocalData(uint16_t varID) {
    return getData(callStack.top(), varID);
}

template<class T>
void Interpreter::setLocalData(T value, uint16_t varID) {
    setData(value, callStack.top(), varID);
}

template <class T>
void Interpreter::setData(T value, uint16_t ctxID, uint16_t varID) {
    storage[ctxID].back()[varID] = Data(value);
}

void Interpreter::enterFunction(BytecodeFunction * function) {
    if (function->id() >= storage.size()) {
        storage.resize(function->id() + 1);
    }
    callStack.push(function->id());
    storage[function->id()].push_back(vector <Data> (function->localsNumber()));
}

void Interpreter::exitFunction() {
    uint16_t curFuncID = callStack.top();
    storage[curFuncID].pop_back();
    callStack.pop();
}


}