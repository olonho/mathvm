#include <cstdint>
#include "SimpleInterpreter.hpp"
#include "Errors.hpp"
#include "logger.hpp"

namespace mathvm {

    Status *SimpleInterpreter::execute(vector<Var *> &vars) {
        try {
            stringstream ss;
            run(ss);
            LOG << "---------RESULT-----------" << endl;
            cout << ss.str();
        } catch (InterpretationError e) {
            return Status::Error(e.what());
        }

        return Status::Ok();
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types"

    void SimpleInterpreter::run(ostream &out) {
        stack_t programStack;
        std::vector<Bytecode *> bytecodes;
        std::vector<uint32_t> indices;
        vars_t vars;
        bytecodes.push_back(bytecode);
        indices.push_back(0);

        while (!bytecodes.empty()) {
            uint32_t &index = indices.back();
            Bytecode &bytecode = *bytecodes.back();
            Instruction instruction = bytecode.getInsn(index);
            size_t instructionLength;
            LOG << "index: " << index << ", instruction: " << bytecodeName(instruction, &instructionLength) << endl;
            switch (instruction) {
                case BC_DLOAD: {
                    Var var(VT_DOUBLE, "");
                    var.setDoubleValue(bytecode.getDouble(index + 1));
                    programStack.push_back(var);
                    break;
                }
                case BC_ILOAD: {
                    Var var(VT_INT, "");
                    var.setIntValue(bytecode.getInt64(index + 1));
                    programStack.push_back(var);
                    break;
                }
                case BC_SLOAD: {
                    Var var(VT_STRING, "");
                    var.setStringValue(constantById(bytecode.getUInt16(index + 1)).c_str());
                    programStack.push_back(var);
                    break;
                }
                case BC_DLOAD0: {
                    Var var(VT_DOUBLE, "");
                    var.setDoubleValue(0);
                    programStack.push_back(var);
                    break;
                }
                case BC_ILOAD0: {
                    Var var(VT_INT, "");
                    var.setIntValue(0);
                    programStack.push_back(var);
                    break;
                }
                case BC_SLOAD0: {
                    Var var(VT_STRING, "");
                    var.setStringValue("");
                    programStack.push_back(var);
                    break;
                }
                case BC_DLOAD1: {
                    Var var(VT_DOUBLE, "");
                    var.setDoubleValue(1);
                    programStack.push_back(var);
                    break;
                }
                case BC_ILOAD1: {
                    Var var(VT_INT, "");
                    var.setIntValue(1);
                    programStack.push_back(var);
                    break;
                }
                case BC_DLOADM1: {
                    Var var(VT_DOUBLE, "");
                    var.setDoubleValue(-1);
                    programStack.push_back(var);
                    break;
                }
                case BC_ILOADM1: {
                    Var var(VT_INT, "");
                    var.setIntValue(-1);
                    programStack.push_back(var);
                    break;
                }
                case BC_DADD:
                    binary_operation(VT_DOUBLE, programStack, add<double>);
                    break;
                case BC_IADD:
                    binary_operation(VT_INT, programStack, add<int64_t>);
                    break;
                case BC_DSUB:
                    binary_operation(VT_DOUBLE, programStack, sub<double>);
                    break;
                case BC_ISUB:
                    binary_operation(VT_INT, programStack, sub<int64_t>);
                    break;
                case BC_DMUL:
                    binary_operation(VT_DOUBLE, programStack, mul<double>);
                    break;
                case BC_IMUL:
                    binary_operation(VT_INT, programStack, mul<int64_t>);
                    break;
                case BC_DDIV:
                    binary_operation(VT_DOUBLE, programStack, _div<double>);
                    break;
                case BC_IDIV:
                    binary_operation(VT_INT, programStack, _div<int64_t>);
                    break;
                case BC_IMOD:
                    binary_operation(VT_INT, programStack, mod<int64_t>);
                    break;
                case BC_DNEG:
                    unary_operation(VT_DOUBLE, programStack, neg<double>);
                    break;
                case BC_INEG:
                    unary_operation(VT_INT, programStack, neg<int64_t>);
                    break;
                case BC_IAOR:
                    binary_operation(VT_INT, programStack, _or<int64_t>);
                    break;
                case BC_IAAND:
                    binary_operation(VT_INT, programStack, _and<int64_t>);
                    break;
                case BC_IAXOR:
                    binary_operation(VT_INT, programStack, _xor<int64_t>);
                    break;
                case BC_IPRINT: {
                    Var v = programStack.back();
                    programStack.pop_back();
                    out << v.getIntValue();
                    break;
                }
                case BC_DPRINT: {
                    Var v = programStack.back();
                    programStack.pop_back();
                    out << v.getDoubleValue();
                    break;
                }
                case BC_SPRINT: {
                    Var v = programStack.back();
                    programStack.pop_back();
                    out << v.getStringValue();
                    break;
                }
                case BC_SWAP: {
                    Var v1 = programStack.back();
                    programStack.pop_back();
                    Var v2 = programStack.back();
                    programStack.pop_back();
                    programStack.push_back(v1);
                    programStack.push_back(v2);
                    break;
                }
                case BC_STOREDVAR0:
                case BC_STOREIVAR0:
                    storeVariable(programStack, vars, 0);
                    break;
                case BC_STOREDVAR1:
                case BC_STOREIVAR1:
                    storeVariable(programStack, vars, 1);
                    break;
                case BC_STOREDVAR2:
                case BC_STOREIVAR2:
                    storeVariable(programStack, vars, 2);
                    break;
                case BC_STOREDVAR3:
                case BC_STOREIVAR3:
                    storeVariable(programStack, vars, 3);
                    break;
                case BC_LOADDVAR:
                case BC_LOADIVAR:
                case BC_LOADSVAR:
                    programStack.push_back(vars[bytecode.getUInt16(index + 1)].back());
                    break;
                case BC_STOREDVAR:
                case BC_STOREIVAR:
                    storeVariable(programStack, vars, bytecode.getUInt16(index + 1));
                    break;
                case BC_DCMP:
                    break;
                case BC_ICMP:
                    break;
                case BC_JA: {
                    index += bytecode.getInt16(index + 1) + 1;
                    continue;
                }
                case BC_IFICMPNE: {
                    if (!check_condition(programStack, _neq<int64_t>))
                        break;
                    index += bytecode.getInt16(index + 1) + 1;
                    continue;
                }
                case BC_IFICMPE: {
                    if (!check_condition(programStack, _eq<int64_t>))
                        break;
                    index += bytecode.getInt16(index + 1) + 1;
                    continue;
                }
                case BC_IFICMPG: {
                    if (!check_condition(programStack, _g<int64_t>))
                        break;
                    index += bytecode.getInt16(index + 1) + 1;
                    continue;
                }
                case BC_IFICMPGE: {
                    if (!check_condition(programStack, _ge<int64_t>))
                        break;
                    index += bytecode.getInt16(index + 1) + 1;
                    continue;
                }
                case BC_IFICMPL: {
                    if (!check_condition(programStack, _l<int64_t>))
                        break;
                    index += bytecode.getInt16(index + 1) + 1;
                    continue;
                }
                case BC_IFICMPLE: {
                    if (!check_condition(programStack, _le<int64_t>))
                        break;
                    index += bytecode.getInt16(index + 1) + 1;
                    continue;
                }
                case BC_STOP: {
                    indices.clear();
                    bytecodes.clear();
                    continue;
                }
                case BC_CALL: {
                    TranslatedFunction *f = functionById(bytecode.getUInt16(index + 1));
                    bytecodes.push_back(static_cast<BytecodeFunction *>(f)->bytecode());
                    indices.push_back(0);
                    continue;
                }
                case BC_RETURN: {
                    indices.pop_back();
                    bytecodes.pop_back();
                    if (!indices.empty()) {
                        size_t len;
                        bytecodeName(BC_CALL, &len);
                        indices.back() += len;
                    }
                    continue;
                }
                case BC_INVALID:
                    throw InterpretationError("BC_Invalid instruction");
                default:
                    throw InterpretationError(string("Unknown instruction: ") + bytecodeName(instruction, 0));
            }
            index += instructionLength;
        }
    }

#pragma clang diagnostic pop
}
