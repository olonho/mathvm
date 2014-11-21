#include <cstdint>
#include <cstdlib>
#include "SimpleInterpreter.hpp"
#include "Errors.hpp"

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
        programStack.clear();
        bytecodes.clear();
        indices.clear();
        vars.clear();
        bytecodes.push_back(bytecode);
        indices.push_back(0);
        contextID.push_back(0);
        callsCounter.push_back(0);

        while (!bytecodes.empty()) {
            indexType &currentIndex = indices.back();
            Bytecode &bytecode = *bytecodes.back();
            Instruction instruction = bytecode.getInsn(currentIndex);
            size_t instructionLength;
            LOG << "index: " << currentIndex << ", instruction: " << bytecodeName(instruction, &instructionLength) << endl;
            switch (instruction) {
                case BC_DLOAD:
                    pushVariable(bytecode.getDouble(currentIndex + 1));
                    break;
                case BC_ILOAD:
                    pushVariable(bytecode.getInt64(currentIndex + 1));
                    break;
                case BC_SLOAD:
                    pushVariable(constantById(bytecode.getUInt16(currentIndex + 1)).c_str());
                    break;
                case BC_DLOAD0:
                    pushVariable(0.0);
                    break;
                case BC_ILOAD0:
                    pushVariable((signedIntType) 0);
                    break;
                case BC_SLOAD0:
                    pushVariable("");
                    break;
                case BC_DLOAD1:
                    pushVariable(1.0);
                    break;
                case BC_ILOAD1:
                    pushVariable((signedIntType) 1);
                    break;
                case BC_DLOADM1:
                    pushVariable(-1.0);
                    break;
                case BC_ILOADM1:
                    pushVariable((signedIntType) -1);
                    break;
                case BC_DADD:
                    binary_operation(VT_DOUBLE, add<double>);
                    break;
                case BC_IADD:
                    binary_operation(VT_INT, add<signedIntType>);
                    break;
                case BC_DSUB:
                    binary_operation(VT_DOUBLE, sub<double>);
                    break;
                case BC_ISUB:
                    binary_operation(VT_INT, sub<signedIntType>);
                    break;
                case BC_DMUL:
                    binary_operation(VT_DOUBLE, mul<double>);
                    break;
                case BC_IMUL:
                    binary_operation(VT_INT, mul<signedIntType>);
                    break;
                case BC_DDIV:
                    binary_operation(VT_DOUBLE, _div<double>);
                    break;
                case BC_IDIV:
                    binary_operation(VT_INT, _div<signedIntType>);
                    break;
                case BC_IMOD:
                    binary_operation(VT_INT, mod<signedIntType>);
                    break;
                case BC_DNEG:
                    unary_operation(VT_DOUBLE, neg<double>);
                    break;
                case BC_INEG:
                    unary_operation(VT_INT, neg<signedIntType>);
                    break;
                case BC_IAOR:
                    binary_operation(VT_INT, _or<signedIntType>);
                    break;
                case BC_IAAND:
                    binary_operation(VT_INT, _and<signedIntType>);
                    break;
                case BC_IAXOR:
                    binary_operation(VT_INT, _xor<signedIntType>);
                    break;
                case BC_IPRINT:
                    out << popVariable().getIntValue();
                    break;
                case BC_DPRINT:
                    out << popVariable().getDoubleValue();
                    break;
                case BC_SPRINT:
                    out << popVariable().getStringValue();
                    break;
                case BC_SWAP: {
                    Var v1 = popVariable();
                    Var v2 = popVariable();
                    programStack.push_back(v1);
                    programStack.push_back(v2);
                    break;
                }
                case BC_STOREDVAR0:
                case BC_STOREIVAR0:
                    storeVariable(0);
                    break;
                case BC_STOREDVAR1:
                case BC_STOREIVAR1:
                    storeVariable(1);
                    break;
                case BC_STOREDVAR2:
                case BC_STOREIVAR2:
                    storeVariable(2);
                    break;
                case BC_STOREDVAR3:
                case BC_STOREIVAR3:
                    storeVariable(3);
                    break;
                case BC_LOADDVAR:
                case BC_LOADIVAR:
                case BC_LOADSVAR:
                    programStack.push_back(loadVariable(bytecode.getUInt16(currentIndex + 1)));
                    break;
                case BC_STOREDVAR:
                case BC_STOREIVAR:
                    storeVariable(bytecode.getUInt16(currentIndex + 1));
                    break;
                case BC_LOADCTXDVAR:
                case BC_LOADCTXIVAR:
                case BC_LOADCTXSVAR:
                    programStack.push_back(loadVariable(bytecode.getUInt16(currentIndex + 1), bytecode.getUInt16(currentIndex + 3)));
                    break;
                case BC_STORECTXDVAR:
                case BC_STORECTXIVAR:
                case BC_STORECTXSVAR:
                    storeVariable(bytecode.getUInt16(currentIndex + 1), bytecode.getUInt16(currentIndex + 3));
                    break;
                case BC_DCMP:
                    binary_operation<double, signedIntType>(VT_DOUBLE, _cmp<double>, VT_INT);
                    break;
                case BC_ICMP:
                    binary_operation(VT_INT, _cmp<signedIntType>);
                    break;
                case BC_JA: {
                    currentIndex += bytecode.getInt16(currentIndex + 1) + 1;
                    continue;
                }
                case BC_IFICMPNE: {
                    if (!check_condition(_neq<signedIntType>))
                        break;
                    currentIndex += bytecode.getInt16(currentIndex + 1) + 1;
                    continue;
                }
                case BC_IFICMPE: {
                    if (!check_condition(_eq<signedIntType>))
                        break;
                    currentIndex += bytecode.getInt16(currentIndex + 1) + 1;
                    continue;
                }
                case BC_IFICMPG: {
                    if (!check_condition(_g<signedIntType>))
                        break;
                    currentIndex += bytecode.getInt16(currentIndex + 1) + 1;
                    continue;
                }
                case BC_IFICMPGE: {
                    if (!check_condition(_ge<signedIntType>))
                        break;
                    currentIndex += bytecode.getInt16(currentIndex + 1) + 1;
                    continue;
                }
                case BC_IFICMPL: {
                    if (!check_condition(_l<signedIntType>))
                        break;
                    currentIndex += bytecode.getInt16(currentIndex + 1) + 1;
                    continue;
                }
                case BC_IFICMPLE: {
                    if (!check_condition(_le<signedIntType>))
                        break;
                    currentIndex += bytecode.getInt16(currentIndex + 1) + 1;
                    continue;
                }
                case BC_STOP: {
                    indices.clear();
                    bytecodes.clear();
                    continue;
                }
                case BC_CALL: {
                    TranslatedFunction *f = functionById(bytecode.getUInt16(currentIndex + 1));
                    bytecodes.push_back(static_cast<BytecodeFunction *>(f)->bytecode());
                    indices.push_back(0);
                    contextID.push_back(f->id());
                    detectCallWithFunctionID(contextID.back());
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
                    if (callsCounter[contextID.back()] > 0) {
                        callsCounter[contextID.back()]--;
                    }
                    contextID.pop_back();
                    continue;
                }
                case BC_D2I:
                    pushVariable((signedIntType) popVariable().getDoubleValue());
                    break;
                case BC_I2D:
                    pushVariable((double) popVariable().getIntValue());
                    break;
                case BC_S2I:
                    throw InterpretationError("BC_S2I instruction deprecated");
                case BC_BREAK:
                    break;
                case BC_INVALID:
                    throw InterpretationError("BC_Invalid instruction");
                default:
                    throw InterpretationError(string("Unknown interpreting instruction: ") + bytecodeName(instruction, 0));
            }
            currentIndex += instructionLength;
        }
    }

#pragma clang diagnostic pop
}
