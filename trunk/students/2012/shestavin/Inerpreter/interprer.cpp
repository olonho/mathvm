#include "interprer.h"

namespace mathvm {
    
    const int Interpreter::commandLen[] = {
#define ENUM_ELEM(b, d, l) l,
        FOR_BYTECODES(ENUM_ELEM)
#undef ENUM_ELEM
        1
    };


    Status* Interpreter::execute(vector<Var*> & vars){
        BytecodeFunction* functionToCall = (BytecodeFunction*) functionByName("<top>");
        funExec(functionToCall);
        return 0;
    }
    
    void Interpreter::funExec(BytecodeFunction* f) {
        size_t bci = 0;
        size_t k = 0;
        while (bci < f->bytecode()->length()) {
            ++k;
            Instruction insn = f->bytecode()->getInsn(bci);
            switch (insn) {
                case BC_INVALID: {
                    std::cerr << "Invalid instruction!\n";
                    return;
                    break;
                }
                case BC_DLOAD: {
                    Var var(VT_DOUBLE, "");
                    var.setDoubleValue(f->bytecode()->getDouble(bci + 1));
                    programStack.push(var);
                    break;
                }
                case BC_ILOAD: {
                    Var var(VT_INT, "");
                    var.setIntValue(f->bytecode()->getInt64(bci + 1));
                    programStack.push(var);
                    break;
                }
                case BC_SLOAD: {
                    Var var(VT_STRING, "");
                    var.setStringValue(constantById(f->bytecode()->getUInt16(bci + 1)).c_str());
                    programStack.push(var);
                    break;
                }
                case BC_DLOAD0: {
                    Var var(VT_DOUBLE, "");
                    var.setDoubleValue(0.0);
                    programStack.push(var);
                    break;
                }
                case BC_ILOAD0: {
                    Var var(VT_INT, "");
                    var.setIntValue(0L);
                    programStack.push(var);
                    break;
                }
                case BC_SLOAD0: {
                    Var var(VT_STRING, "");
                    var.setStringValue("");
                    programStack.push(var);
                    break;
                }
                case BC_DLOAD1: {
                    Var var(VT_DOUBLE, "");
                    var.setDoubleValue(1.0);
                    programStack.push(var);
                    break;
                }
                case BC_ILOAD1: {
                    Var var(VT_INT, "");
                    var.setIntValue(1L);
                    programStack.push(var);
                    break;
                }
                case BC_DLOADM1: {
                    Var var(VT_DOUBLE, "");
                    var.setDoubleValue(-1.0);
                    programStack.push(var);
                    break;
                }
                case BC_ILOADM1: {
                    Var var(VT_INT, "");
                    var.setIntValue(-1L);
                    programStack.push(var);
                    break;
                }
                case BC_DADD: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_DOUBLE, "");
                    result.setDoubleValue(leftOperand.getDoubleValue() + rightOperand.getDoubleValue());
                    programStack.push(result);
                    break;
                }
                case BC_IADD: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    result.setIntValue(leftOperand.getIntValue() + rightOperand.getIntValue());
                    programStack.push(result);
                    break;
                }
                case BC_DSUB: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_DOUBLE, "");
                    result.setDoubleValue(leftOperand.getDoubleValue() - rightOperand.getDoubleValue());
                    programStack.push(result);
                    break;
                }
                case BC_ISUB: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    result.setIntValue(leftOperand.getIntValue() - rightOperand.getIntValue());
                    programStack.push(result);
                    break;
                }
                case BC_DMUL: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_DOUBLE, "");
                    result.setDoubleValue(leftOperand.getDoubleValue() * rightOperand.getDoubleValue());
                    programStack.push(result);
                    break;
                }
                case BC_IMUL: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    result.setIntValue(leftOperand.getIntValue() * rightOperand.getIntValue());
                    programStack.push(result);
                    break;
                }
                case BC_DDIV: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_DOUBLE, "");
                    result.setDoubleValue(leftOperand.getDoubleValue() / rightOperand.getDoubleValue());
                    programStack.push(result);
                    break;
                }
                case BC_IDIV: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    result.setIntValue(leftOperand.getIntValue() / rightOperand.getIntValue());
                    programStack.push(result);
                    break;
                }
                case BC_IMOD: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    result.setIntValue(leftOperand.getIntValue() % rightOperand.getIntValue());
                    programStack.push(result);
                    break;
                }
                case BC_DNEG: {
                    Var operand = programStack.top();
                    programStack.pop();
                    Var result(VT_DOUBLE, "");
                    result.setDoubleValue(-operand.getDoubleValue());
                    programStack.push(result);
                    break;
                }
                case BC_INEG: {
                    Var operand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    result.setIntValue(-operand.getIntValue());
                    programStack.push(result);
                    break;
                }
                case BC_IPRINT: {
                    Var operand = programStack.top();
                    programStack.pop();
                    std::cout << operand.getIntValue();
                    break;
                }
                case BC_DPRINT: {
                    Var operand = programStack.top();
                    programStack.pop();
                    std::cout << operand.getDoubleValue();
                    break;
                }
                case BC_SPRINT: {
                    Var operand = programStack.top();
                    programStack.pop();
                    std::cout << operand.getStringValue();
                    break;
                }
                case BC_I2D: {
                    Var operand = programStack.top();
                    programStack.pop();
                    Var result(VT_DOUBLE, "");
                    result.setDoubleValue(static_cast<double>(operand.getIntValue()));
                    programStack.push(result);
                    break;
                }
                case BC_D2I: {
                    Var operand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    result.setIntValue(static_cast<int64_t>(operand.getDoubleValue()));
                    programStack.push(result);
                    break;
                }
                case BC_S2I: {
                    Var operand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    result.setIntValue(atoi(operand.getStringValue()));
                    programStack.push(result);
                    break;
                }
                case BC_SWAP: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    programStack.push(leftOperand);
                    programStack.push(rightOperand);
                    break;
                }
                case BC_POP: {
                    programStack.pop();
                    break;
                }
                case BC_LOADSVAR0:
                case BC_LOADIVAR0:
                case BC_LOADDVAR0: {
                    map<uint16_t, Var>::iterator iter = memory.find(0);
                    if (iter != memory.end()) {
                        programStack.push(iter->second);
                    } else {
                        std::cerr << "Can't load variable 0!\n";
                    }
                    break;
                }
                case BC_LOADSVAR1:
                case BC_LOADIVAR1:
                case BC_LOADDVAR1: {
                    map<uint16_t, Var>::iterator iter = memory.find(1);
                    if (iter != memory.end()) {
                        programStack.push(iter->second);
                    } else {
                        std::cerr << "Can't load variable 1!\n";
                    }

                    break;
                }
                case BC_LOADSVAR2:
                case BC_LOADIVAR2:
                case BC_LOADDVAR2: {
                    map<uint16_t, Var>::iterator iter = memory.find(2);
                    if (iter != memory.end()) {
                        programStack.push(iter->second);
                    } else {
                        std::cerr << "Can't load variable 2!\n";
                    }

                    break;
                }
                case BC_LOADSVAR3:
                case BC_LOADIVAR3:
                case BC_LOADDVAR3: {
                    map<uint16_t, Var>::iterator iter = memory.find(3);
                    if (iter != memory.end()) {
                        programStack.push(iter->second);
                    } else {
                        std::cerr << "Can't load variable 3!\n";
                    }

                    break;
                }
                case BC_STORESVAR0:
                case BC_STOREIVAR0:
                case BC_STOREDVAR0: {
                    map<uint16_t, Var>::iterator iter = memory.find(0);
                    if (iter != memory.end()) {
                        iter->second = programStack.top();
                    } else {
                        memory.insert(make_pair(0, programStack.top()));
                    }
                    programStack.pop();
                    break;
                }
                case BC_STORESVAR1:
                case BC_STOREIVAR1:
                case BC_STOREDVAR1: {
                    map<uint16_t, Var>::iterator iter = memory.find(1);
                    if (iter != memory.end()) {
                        iter->second = programStack.top();
                    } else {
                        memory.insert(make_pair(1, programStack.top()));
                    }
                    programStack.pop();
                    break;
                }
                case BC_STORESVAR2:
                case BC_STOREIVAR2:
                case BC_STOREDVAR2: {
                    map<uint16_t, Var>::iterator iter = memory.find(2);
                    if (iter != memory.end()) {
                        iter->second = programStack.top();
                    } else {
                        memory.insert(make_pair(2, programStack.top()));
                    }
                    programStack.pop();
                    break;
                }
                case BC_STORESVAR3:
                case BC_STOREIVAR3:
                case BC_STOREDVAR3: {
                    map<uint16_t, Var>::iterator iter = memory.find(3);
                    if (iter != memory.end()) {
                        iter->second = programStack.top();
                    } else {
                        memory.insert(make_pair(3, programStack.top()));
                    }
                    programStack.pop();
                    break;
                }
                case BC_LOADDVAR: {
                    uint16_t id = f->bytecode()->getUInt16(bci + 1);
                    map<uint16_t, Var>::iterator iter = memory.find(id);
                    if (iter != memory.end()) {
                        programStack.push(iter->second);
                    } else {
                        //std::cerr << "Can't load variable " << id << "!\n";
                        Var var(VT_DOUBLE, "");
                        var.setDoubleValue(0.0);
                        programStack.push(var);
                    }
                    break;
                }
                case BC_LOADIVAR: {
                    uint16_t id = f->bytecode()->getUInt16(bci + 1);
                    map<uint16_t, Var>::iterator iter = memory.find(id);
                    if (iter != memory.end()) {
                        programStack.push(iter->second);
                    } else {
                        //std::cerr << "Can't load variable " << id << "!\n";
                        Var var(VT_INT, "");
                        var.setIntValue(0);
                        programStack.push(var);
                    }
                    break;
                }
                case BC_LOADSVAR: {
                    uint16_t id = f->bytecode()->getUInt16(bci + 1);
                    map<uint16_t, Var>::iterator iter = memory.find(id);
                    if (iter != memory.end()) {
                        programStack.push(iter->second);
                    } else {
                        //std::cerr << "Can't load variable " << id << "!\n";
                        Var var(VT_STRING, "");
                        var.setStringValue("");
                        programStack.push(var);
                    }
                    break;
                }
                case BC_STOREDVAR:
                case BC_STOREIVAR:
                case BC_STORESVAR: {
                    uint16_t id = f->bytecode()->getUInt16(bci + 1);
                    map<uint16_t, Var>::iterator iter = memory.find(id);
                    if (iter != memory.end()) {
                        iter->second = programStack.top();
                    } else {
                        memory.insert(make_pair(id, programStack.top()));
                    }
                    programStack.pop();
                    break;
                }
                case BC_DCMP: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    if (leftOperand.getDoubleValue() > rightOperand.getDoubleValue()) {
                        result.setIntValue(1);
                    } else if (leftOperand.getDoubleValue() == rightOperand.getDoubleValue()) {
                        result.setIntValue(0);
                    } else {
                        result.setIntValue(-1);
                    }
                    programStack.push(result);
                    break;
                }
                case BC_ICMP: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.pop();
                    Var result(VT_INT, "");
                    if (leftOperand.getIntValue() > rightOperand.getIntValue()) {
                        result.setIntValue(1);
                    } else if (leftOperand.getIntValue() == rightOperand.getIntValue()) {
                        result.setIntValue(0);
                    } else {
                        result.setIntValue(-1);
                    }
                    programStack.push(result);
                    break;
                }
                case BC_JA: {
                    bci += (f->bytecode()->getInt16(bci + 1));
                    continue;
                }
                case BC_IFICMPNE: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.push(leftOperand);
                    if (leftOperand.getIntValue() != rightOperand.getIntValue()) {
                        bci += (f->bytecode()->getInt16(bci + 1));
                        continue;
                    }
                    break;
                }
                case BC_IFICMPE: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.push(leftOperand);
                    if (leftOperand.getIntValue() == rightOperand.getIntValue()) {
                        bci += (f->bytecode()->getInt16(bci + 1));
                        continue;
                    }
                    break;
                }
                case BC_IFICMPG: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.push(leftOperand);
                    if (leftOperand.getIntValue() > rightOperand.getIntValue()) {
                        bci += (f->bytecode()->getInt16(bci + 1));
                        continue;
                    }
                    break;
                }
                case BC_IFICMPGE: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.push(leftOperand);
                    if (leftOperand.getIntValue() >= rightOperand.getIntValue()) {
                        bci += (f->bytecode()->getInt16(bci + 1));
                        continue;
                    }
                    break;
                }
                case BC_IFICMPL: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.push(leftOperand);
                    if (leftOperand.getIntValue() < rightOperand.getIntValue()) {
                        bci += (f->bytecode()->getInt16(bci + 1));
                        continue;
                    }
                    break;
                }
                case BC_IFICMPLE: {
                    Var leftOperand = programStack.top();
                    programStack.pop();
                    Var rightOperand = programStack.top();
                    programStack.push(leftOperand);
                    if (leftOperand.getIntValue() <= rightOperand.getIntValue()) {
                        bci += (f->bytecode()->getInt16(bci + 1));
                        continue;
                    }
                    break;
                }
                case BC_STOP: {
                    return;
                }
                case BC_CALL: {
                    int id = f->bytecode()->getUInt16(bci + 1);
                    BytecodeFunction* functionToCall = (BytecodeFunction*) functionById(id);
                    
                    returnAddresses.push(bci + commandLen[insn]);
                    returnLocations.push(f);
                    
                    bci = 0;
                    f = functionToCall;
                    continue;
                }
                case BC_RETURN: {
                    f = returnLocations.top();
                    returnLocations.pop();
                    
                    bci = returnAddresses.top();
                    returnAddresses.pop();
                    continue;
                }
                default: {
                    std::cerr << "Unknown instruction!\n";
                    return;
                }
            }
            bci += commandLen[insn];
        }
    }
}