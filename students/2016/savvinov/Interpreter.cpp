//
// Created by dsavvinov on 13.11.16.
//

#include <ast.h>
#include "Interpreter.h"
#include "TranslationException.h"

namespace mathvm {

Status * Interpreter::executeProgram() {
    BytecodeFunction * topFunction = (BytecodeFunction *) code->functionByName(AstFunction::top_name);
    frames.push_back(StackFrame());
    executeFunction(topFunction);
    return Status::Ok();
}

void Interpreter::executeFunction(BytecodeFunction * function) {
    Bytecode * bc = function->bytecode();
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
                push(frames.back().getLocal(0).getDouble());
                ++i;
                break;
            case BC_LOADDVAR1:
                push(frames.back().getLocal(1).getDouble());
                ++i;
                break;
            case BC_LOADDVAR2:
                push(frames.back().getLocal(2).getDouble());
                ++i;
                break;
            case BC_LOADDVAR3:
                push(frames.back().getLocal(3).getDouble());
                ++i;
                break;
            case BC_LOADIVAR0:
                push(frames.back().getLocal(0).getInt());
                ++i;
                break;
            case BC_LOADIVAR1:
                push(frames.back().getLocal(1).getInt());
                ++i;
                break;
            case BC_LOADIVAR2:
                push(frames.back().getLocal(2).getInt());
                ++i;
                break;
            case BC_LOADIVAR3:
                push(frames.back().getLocal(3).getInt());
                ++i;
                break;
            case BC_LOADSVAR0:
                push(*frames.back().getLocal(0).getString());
                ++i;
                break;
            case BC_LOADSVAR1:
                push(*frames.back().getLocal(1).getString());
                ++i;
                break;
            case BC_LOADSVAR2:
                push(*frames.back().getLocal(2).getString());
                ++i;
                break;
            case BC_LOADSVAR3:
                push(*frames.back().getLocal(3).getString());
                ++i;
                break;
            case BC_STOREDVAR0: {
                double up = topDouble();
                frames.back().setLocal(up, 0);
                pop();
                ++i;
                break;
            }
            case BC_STOREDVAR1: {
                double up = topDouble();
                frames.back().setLocal(up, 1);
                pop();
                ++i;
                break;
            }
            case BC_STOREDVAR2: {
                double up = topDouble();
                frames.back().setLocal(up, 2);
                pop();
                ++i;
                break;
            }
            case BC_STOREDVAR3: {
                double up = topDouble();
                frames.back().setLocal(up, 3);
                pop();
                ++i;
                break;
            }
            case BC_STOREIVAR0: {
                int64_t up = topInt();
                frames.back().setLocal(up, 0);
                pop();
                ++i;
                break;
            }
            case BC_STOREIVAR1:{
                int64_t up = topInt();
                frames.back().setLocal(up, 1);
                pop();
                ++i;
                break;
            }
            case BC_STOREIVAR2:{
                int64_t up = topInt();
                frames.back().setLocal(up, 2);
                pop();
                ++i;
                break;
            }
            case BC_STOREIVAR3:{
                int64_t up = topInt();
                frames.back().setLocal(up, 3);
                pop();
                ++i;
                break;
            }
            case BC_STORESVAR0: {
                string * up = topString();
                frames.back().setLocal(*up, 0);
                pop();
                ++i;
                break;
            }
            case BC_STORESVAR1:{
                string * up = topString();
                frames.back().setLocal(*up, 1);
                pop();
                ++i;
                break;
            }
            case BC_STORESVAR2:{
                string * up = topString();
                frames.back().setLocal(*up, 2);
                pop();
                ++i;
                break;
            }
            case BC_STORESVAR3:{
                string * up = topString();
                frames.back().setLocal(*up, 3);
                pop();
                ++i;
                break;
            }
            case BC_LOADDVAR: {
                int16_t id = bc->getInt16(++i);
                const Data &data = frames.back().getLocal(id);
                push(*data.doubleValue);
                i += 2;
                break;
            }
            case BC_LOADIVAR: {
                int16_t id = bc->getInt16(++i);
                const Data &data = frames.back().getLocal(id);
                push(*data.intValue);
                i += 2;
                break;
            }
            case BC_LOADSVAR: {
                int16_t id = bc->getInt16(++i);
                const Data &data = frames.back().getLocal(id);
                push(*data.stringValue);
                i += 2;
                break;
            }
            case BC_STOREDVAR: {
                double up = topDouble();
                pop();
                int16_t id = bc->getInt16(++i);
                frames.back().setLocal(up, id);
                i += 2;
                break;
            }
            case BC_STOREIVAR: {
                int64_t up = topInt();
                pop();
                int16_t id = bc->getInt16(++i);
                frames.back().setLocal(up, id);
                i += 2;
                break;
            }
            case BC_STORESVAR: {
                string * up = topString();
                int16_t  id = bc->getInt16(++i);
                frames.back().setLocal(*up, id);
                pop();
                i += 2;
                break;
            }
            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR:
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR:
                throw ExecutionException("ctx stores not implemented yet");
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
                i += (uint32_t) offset;
                break;
            }
            case BC_IFICMPNE: {
                int64_t up = topInt();
                pop();
                int64_t low = topInt();
                pop();
                int16_t offset = bc->getInt16(++i);
                if (up != low) {
                    i += (uint32_t) offset;
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
                    i += (uint32_t) offset;
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
                    i += (uint32_t) offset;
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
                    i += (uint32_t) offset;
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
                    i += (uint32_t) offset;
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
                    i += (uint32_t) offset;
                } else {
                    i += 2;
                }
                break;
            }
            case BC_DUMP:
                throw ExecutionException("bc_dump not implemented yet");
            case BC_STOP: {
                return;
            }
            case BC_CALL: {
                int16_t id = bc->getInt16(++i);
                frames.push_back(StackFrame());
                frames.back().returnAddress = (int16_t) i;
//                if (dbg < topInt()) {
//                    dbg = topInt();
//                    std::cout << "Calling with arg = " << dbg << std::endl;
//                }

                executeFunction((BytecodeFunction *) code->functionById((uint16_t) id));
                i += 2;
                break;
            }
            case BC_CALLNATIVE: {
                throw ExecutionException("call native not implemented yet");
            }
            case BC_RETURN: {
                frames.pop_back();
                return;
            };
            case BC_BREAK:
                throw ExecutionException("bc_break not implemented yet");
            case BC_LAST:
                throw ExecutionException("bc_last not implemented yet");
        }
    }
}

Data StackFrame::getLocal(int16_t id) {
    if (localById.find(id) == localById.end()) {
        throw ExecutionException("Local not found!");
    }
    return localById[id];
}

template<class T>
void StackFrame::setLocal(T value, int16_t id) {
    localById[id] = Data(value);
}
}