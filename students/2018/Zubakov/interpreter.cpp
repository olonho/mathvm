#include <mathvm.h>
#include <include/parser.h>
#include "typeinference.h"
#include "bytecode.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stack>
#include "interpreter.h"

namespace mathvm {
    Status *InterpreterCodeImpl::execute(std::vector<mathvm::Var *> &vars) {
        TranslatedFunction *pFunction = functionByName("<top>");


        Environment env(77); // TODO normal count instead of hardcoded value
        env.emplace(pFunction->scopeId(), pFunction->localsNumber());

        stack<Value> tos;
        stack<pair<BytecodeFunction *, uint32_t >> callStack;
        uint32_t curPos = 0;

        auto *curFunc = (BytecodeFunction *) pFunction;
        auto *pBytecode = curFunc->bytecode();
        while (true) {
            Instruction instruction = pBytecode->getInsn(curPos);

            size_t length = 0;
            bytecodeName(instruction, &length);
            switch (instruction) {
                case BC_ILOAD0: {
                    tos.emplace(0L);
                    break;
                }
                case BC_ILOAD1: {
                    tos.emplace(1L);
                    break;
                }
                case BC_ILOADM1: {
                    tos.emplace(-1L);
                    break;
                }

                case BC_ILOAD: {
                    tos.emplace(pBytecode->getInt64(curPos + 1));
                    break;
                }

                case BC_DLOAD0: {
                    tos.emplace(0.0);
                    break;
                }

                case BC_DLOAD1: {
                    tos.emplace(1.0);
                    break;
                }
                case BC_DLOADM1: {
                    tos.emplace(-1.0);
                    break;
                }

                case BC_DLOAD: {
                    tos.emplace(pBytecode->getDouble(curPos + 1));
                    break;
                }

                case BC_SLOAD0: {
                    tos.emplace("");
                    break;
                }

                case BC_SLOAD: {
                    uint16_t i = pBytecode->getUInt16(curPos + 1);
                    const string &args = constantById(i);
                    tos.emplace(args.c_str());
                    break;
                }

                case BC_IPRINT: {
                    int64_t toPrint = tos.top().getIntValue();
                    tos.pop();
                    std::cout << toPrint;
                    break;
                }

                case BC_DPRINT: {
                    const double &d = tos.top().getDoubleValue();
                    tos.pop();
                    std::cout << d;
                    break;
                }

                case BC_SPRINT: {
                    const string &basic_string = tos.top().getStringValue();
                    std::cout << basic_string;
                    tos.pop();
                    break;
                }

                case BC_INEG: {
                    const int64_t &i = tos.top().getIntValue();
                    tos.pop();
                    tos.emplace(-i);
                    break;
                }

                case BC_DNEG: {
                    const double &i = tos.top().getDoubleValue();
                    tos.pop();
                    tos.emplace(-i);
                    break;
                }

                case BC_IADD: {
                    int64_t left = tos.top().getIntValue();
                    tos.pop();

                    int64_t right = tos.top().getIntValue();
                    tos.pop();

                    tos.emplace(left + right);
                    break;
                }
                case BC_ISUB: {
                    int64_t left = tos.top().getIntValue();
                    tos.pop();

                    int64_t right = tos.top().getIntValue();
                    tos.pop();

                    tos.emplace(left - right);
                    break;
                }

                case BC_IDIV: {
                    int64_t left = tos.top().getIntValue();
                    tos.pop();

                    int64_t right = tos.top().getIntValue();
                    tos.pop();

                    tos.emplace(left / right);
                    break;
                }

                case BC_IMUL: {
                    int64_t left = tos.top().getIntValue();
                    tos.pop();

                    int64_t right = tos.top().getIntValue();
                    tos.pop();

                    tos.emplace(left * right);
                    break;
                }

                case BC_IMOD: {
                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    tos.emplace(left % right);
                    break;
                }

                case BC_DADD: {
                    const double &left = tos.top().getDoubleValue();
                    tos.pop();

                    const double &right = tos.top().getDoubleValue();
                    tos.pop();

                    tos.emplace(left + right);
                    break;
                }

                case BC_DMUL: {
                    const double left = tos.top().getDoubleValue();
                    tos.pop();

                    const double right = tos.top().getDoubleValue();
                    tos.pop();

                    tos.emplace(left * right);
                    break;
                }

                case BC_DSUB: {
                    const double left = tos.top().getDoubleValue();
                    tos.pop();

                    const double right = tos.top().getDoubleValue();
                    tos.pop();

                    tos.emplace(left - right);
                    break;
                }
                case BC_DDIV: {
                    const double left = tos.top().getDoubleValue();
                    tos.pop();

                    const double right = tos.top().getDoubleValue();
                    tos.pop();

                    tos.emplace(left / right);
                    break;
                }
                case BC_IAAND: {
                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    tos.emplace(left & right);
                    break;
                }
                case BC_IAOR: {
                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    tos.emplace(left | right);
                    break;
                }

                case BC_IAXOR: {
                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    tos.emplace(left ^ right);
                    break;
                }
                case BC_S2I: {
                    // TODO
                    break;
                }

                case BC_I2D: {
                    int64_t i = tos.top().getIntValue();
                    tos.pop();
                    tos.emplace((double) i);
                    break;
                }


                case BC_D2I: {
                    double d = tos.top().getDoubleValue();
                    tos.pop();
                    tos.emplace((int64_t) d);
                    break;
                }

                case BC_LOADIVAR0: {
                    tos.emplace(env.getInt(curFunc->scopeId(), 0));
                    break;
                }

                case BC_LOADIVAR1: {
                    tos.emplace(env.getInt(curFunc->scopeId(), 1));
                    break;
                }
                case BC_LOADIVAR2: {
                    tos.emplace(env.getInt(curFunc->scopeId(), 2));
                    break;
                }
                case BC_LOADIVAR3: {
                    tos.emplace(env.getInt(curFunc->scopeId(), 3));
                    break;
                }
                case BC_LOADIVAR: {
                    uint16_t idx = pBytecode->getUInt16(curPos + 1);
                    tos.emplace(env.getInt(curFunc->scopeId(), idx));
                    break;
                }

                case BC_LOADCTXIVAR: {
                    uint16_t ctx_id = pBytecode->getUInt16(curPos + 1);
                    uint16_t var_id = pBytecode->getUInt16(curPos + 3);

                    tos.emplace(env.getInt(ctx_id, var_id));
                    break;
                }

                case BC_STOREIVAR0: {
                    env.setInt(curFunc->scopeId(), 0, tos.top().getIntValue());
                    tos.pop();
                    break;
                }
                case BC_STOREIVAR1: {
                    env.setInt(curFunc->scopeId(), 1, tos.top().getIntValue());
                    tos.pop();
                    break;
                }

                case BC_STOREIVAR2: {
                    env.setInt(curFunc->scopeId(), 2, tos.top().getIntValue());
                    tos.pop();
                    break;
                }
                case BC_STOREIVAR3: {
                    env.setInt(curFunc->scopeId(), 3, tos.top().getIntValue());
                    tos.pop();
                    break;
                }

                case BC_STOREIVAR: {
                    uint16_t idx = pBytecode->getUInt16(curPos + 1);
                    env.setInt(curFunc->scopeId(), idx, tos.top().getIntValue());
                    tos.pop();
                    break;
                }
                case BC_STORECTXIVAR: {
                    uint16_t ctx_id = pBytecode->getUInt16(curPos + 1);
                    uint16_t var_id = pBytecode->getUInt16(curPos + 3);
                    env.setInt(ctx_id, var_id, tos.top().getIntValue());
                    tos.pop();
                    break;
                }

                case BC_LOADDVAR0: {
                    tos.emplace(env.getDouble(curFunc->scopeId(), 0));
                    break;
                }

                case BC_LOADDVAR1: {
                    tos.emplace(env.getDouble(curFunc->scopeId(), 1));
                    break;
                }
                case BC_LOADDVAR2: {
                    tos.emplace(env.getDouble(curFunc->scopeId(), 2));
                    break;
                }
                case BC_LOADDVAR3: {
                    tos.emplace(env.getDouble(curFunc->scopeId(), 3));
                    break;
                }
                case BC_LOADDVAR: {
                    uint16_t idx = pBytecode->getUInt16(curPos + 1);
                    tos.emplace(env.getDouble(curFunc->scopeId(), idx));
                    break;
                }

                case BC_LOADCTXDVAR: {
                    uint16_t ctx_id = pBytecode->getUInt16(curPos + 1);
                    uint16_t var_id = pBytecode->getUInt16(curPos + 3);

                    tos.emplace(env.getDouble(ctx_id, var_id));
                    break;
                }


                case BC_STOREDVAR0: {
                    env.setDouble(curFunc->scopeId(), 0, tos.top().getDoubleValue());
                    tos.pop();
                    break;
                }
                case BC_STOREDVAR1: {
                    env.setDouble(curFunc->scopeId(), 1, tos.top().getDoubleValue());
                    tos.pop();
                    break;
                }

                case BC_STOREDVAR2: {
                    env.setDouble(curFunc->scopeId(), 2, tos.top().getDoubleValue());
                    tos.pop();
                    break;
                }
                case BC_STOREDVAR3: {
                    env.setDouble(curFunc->scopeId(), 3, tos.top().getDoubleValue());
                    tos.pop();
                    break;
                }

                case BC_STOREDVAR: {
                    uint16_t idx = pBytecode->getUInt16(curPos + 1);
                    env.setDouble(curFunc->scopeId(), idx, tos.top().getDoubleValue());
                    tos.pop();
                    break;
                }
                case BC_STORECTXDVAR: {
                    uint16_t ctx_id = pBytecode->getUInt16(curPos + 1);
                    uint16_t var_id = pBytecode->getUInt16(curPos + 3);
                    env.setDouble(ctx_id, var_id, tos.top().getDoubleValue());
                    tos.pop();
                    break;

                }

                case BC_STORESVAR0: {
                    env.setString(curFunc->scopeId(), 0, tos.top().getStringValue());
                    tos.pop();
                    break;
                }
                case BC_STORESVAR1: {
                    env.setString(curFunc->scopeId(), 1, tos.top().getStringValue());
                    tos.pop();
                    break;
                }

                case BC_STORESVAR2: {
                    env.setString(curFunc->scopeId(), 2, tos.top().getStringValue());
                    tos.pop();
                    break;
                }
                case BC_STORESVAR3: {
                    env.setString(curFunc->scopeId(), 3, tos.top().getStringValue());
                    tos.pop();
                    break;
                }

                case BC_STORESVAR: {
                    uint16_t idx = pBytecode->getUInt16(curPos + 1);
                    env.setString(curFunc->scopeId(), idx, tos.top().getStringValue());
                    tos.pop();
                    break;
                }
                case BC_STORECTXSVAR: {
                    uint16_t ctx_id = pBytecode->getUInt16(curPos + 1);
                    uint16_t var_id = pBytecode->getUInt16(curPos + 1);
                    env.setString(ctx_id, var_id, tos.top().getStringValue());
                }

                case BC_DCMP: {
                    const double &left = tos.top().getDoubleValue();
                    tos.pop();

                    const double &right = tos.top().getDoubleValue();
                    tos.pop();

                    if (left < right) {
                        tos.emplace(-1L);
                    } else if (left == right) {
                        tos.emplace(0L);
                    } else {
                        tos.emplace(1L);
                    }
                    break;
                }
                case BC_ICMP: {
                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    if (left < right) {
                        tos.emplace(-1L);
                    } else if (left == right) {
                        tos.emplace(0L);
                    } else {
                        tos.emplace(1L);
                    }
                    break;
                }

                case BC_IFICMPNE: {
                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    if (left != right) {
                        ++curPos;
                        curPos += pBytecode->getInt16(curPos);
                        continue;
                    }
                    break;
                }
                case BC_IFICMPE: {
                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    if (left == right) {
                        ++curPos;
                        curPos += pBytecode->getInt16(curPos);
                        continue;
                    }
                    break;
                }
                case BC_IFICMPG: {
                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    if (left > right) {
                        ++curPos;
                        curPos += pBytecode->getUInt16(curPos);
                        continue;
                    }
                    break;

                }
                case BC_IFICMPGE: {
                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    if (left >= right) {
                        ++curPos;
                        curPos += pBytecode->getInt16(curPos);
                        continue;
                    }
                    break;

                }
                case BC_IFICMPL: {
                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    if (left < right) {
                        ++curPos;
                        curPos += pBytecode->getInt16(curPos);
                        continue;
                    }
                    break;

                }
                case BC_IFICMPLE: {
                    const int64_t &right = tos.top().getIntValue();
                    tos.pop();

                    const int64_t &left = tos.top().getIntValue();
                    tos.pop();

                    if (left <= right) {
                        ++curPos;
                        curPos += pBytecode->getInt16(curPos);
                        continue;
                    }
                    break;
                }

                case BC_JA: {
                    ++curPos;
                    curPos += pBytecode->getInt16(curPos);
                    continue;
                    break;
                }

                case BC_CALL: {
                    callStack.emplace(curFunc, curPos + length);

                    uint16_t funcId = pBytecode->getUInt16(curPos + 1);
                    TranslatedFunction *pTranslatedFunction = functionById(funcId);

                    curFunc = (BytecodeFunction *) pTranslatedFunction;
                    curPos = 0;
                    env.emplace(curFunc->scopeId(), curFunc->localsNumber());
                    pBytecode = curFunc->bytecode();
                    continue;
                }

                case BC_RETURN: {
                    env.pop(curFunc->scopeId());

                    curFunc = callStack.top().first;
                    pBytecode = curFunc->bytecode();
                    curPos = callStack.top().second;

                    callStack.pop();
                    continue;
                }

                case BC_POP:
                    tos.pop();
                    break;

                case BC_SWAP: {
                    Value val1 = tos.top();
                    tos.pop();
                    Value val2 = tos.top();
                    tos.pop();

                    tos.push(val1);
                    tos.push(val2);
                    break;
                }
                case BC_STOP:
                    return Status::Ok();

                case BC_INVALID: {
                    assert(0);
                }

                default:
                    assert(0);

            }

            curPos += length;
        }
    }

    Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {
        auto *interpreterCode = new InterpreterCodeImpl();
        *code = interpreterCode;
        return translateBytecode(program, &interpreterCode);
    }

    Status *BytecodeTranslatorImpl::translateBytecode(const std::string &program, mathvm::InterpreterCodeImpl **code) {
        Parser parser;
        Status *pStatus = parser.parseProgram(program);
        if (!pStatus->isOk()) {
            return pStatus;
        }

        AstFunction *pFunction = parser.top();

        vector<std::unique_ptr<Info>> whomp; // :c
        TypeInferenceVisitor typeInferenceVisitor(whomp);
        typeInferenceVisitor.visitFunctionNode(pFunction->node());


        BytecodeVisitor bytecodeVisitor(*code);
        bytecodeVisitor.registerFunction(pFunction);
        bytecodeVisitor.visitAstFunction(pFunction);

        return pStatus;

    }


}

