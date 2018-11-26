

#include <mathvm.h>
#include <include/parser.h>
#include "typeinference.h"
#include "bytecode.h"
#include "interpreter.h"
#include <iostream>
#include <memory>
#include <stack>

namespace mathvm {
    Status *InterpreterCodeImpl::execute(std::vector<mathvm::Var *> &vars) {
        TranslatedFunction *pFunction = functionByName("<top>");


        stack<StackValue> tos;

        BytecodeFunction *function = (BytecodeFunction *) pFunction;
        Bytecode *pBytecode = function->bytecode();

        uint32_t curPos = 0;
        while (true) {
            Instruction instruction = pBytecode->getInsn(curPos);

            size_t length = 0;
            bytecodeName(instruction, &length);

            switch (instruction) {
                case BC_ILOAD0: {
                    tos.emplace(0L);
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
                    tos.emplace(args);
                    break;
                }

                case BC_IPRINT: {
                    int64_t toPrint = tos.top().getIVal();
                    tos.pop();
                    std::cout << toPrint;
                    break;
                }

                case BC_DPRINT: {
                    const double &d = tos.top().getDVal();
                    tos.pop();
                    std::cout << d;
                    break;
                }

                case BC_SPRINT: {
                    const string &basic_string = tos.top().getSVal();
                    std::cout << basic_string;
                    tos.pop();
                    break;
                }

                case BC_IADD: {
                    int64_t left = tos.top().getIVal();
                    tos.pop();

                    int64_t right = tos.top().getIVal();
                    tos.pop();

                    tos.emplace(left + right);
                    break;
                }
                case BC_ISUB: {
                    int64_t left = tos.top().getIVal();
                    tos.pop();

                    int64_t right = tos.top().getIVal();
                    tos.pop();

                    tos.emplace(left - right);
                    break;
                }

                case BC_IDIV: {
                    int64_t left = tos.top().getIVal();
                    tos.pop();

                    int64_t right = tos.top().getIVal();
                    tos.pop();

                    tos.emplace(left / right);
                    break;
                }

                case BC_IMUL: {
                    int64_t left = tos.top().getIVal();
                    tos.pop();

                    int64_t right = tos.top().getIVal();
                    tos.pop();

                    tos.emplace(left * right);
                    break;
                }

                case BC_IMOD: {
                    int64_t left = tos.top().getIVal();
                    tos.pop();

                    int64_t right = tos.top().getIVal();
                    tos.pop();

                    tos.emplace(left % right);
                    break;
                }

                case BC_DADD: {
                    const double left = tos.top().getDVal();
                    tos.pop();

                    const double right = tos.top().getDVal();
                    tos.pop();

                    tos.emplace(left + right);
                    break;
                }

                case BC_DSUB: {
                    const double left = tos.top().getDVal();
                    tos.pop();

                    const double right = tos.top().getDVal();
                    tos.pop();

                    tos.emplace(left - right);
                    break;
                }
                case BC_DDIV: {
                    const double left = tos.top().getDVal();
                    tos.pop();

                    const double right = tos.top().getDVal();
                    tos.pop();

                    tos.emplace(left / right);
                    break;
                }
                case BC_DMUL: {
                    const double left = tos.top().getDVal();
                    tos.pop();

                    const double right = tos.top().getDVal();
                    tos.pop();

                    tos.emplace(left * right);
                    break;
                }


                case BC_S2I: {
                    // TODO
                    break;
                }

                case BC_I2D: {
                    int64_t i = tos.top().getIVal();
                    tos.pop();
                    tos.emplace((double) i);
                }


                case BC_D2I: {
                    double d = tos.top().getDVal();
                    tos.pop();
                    tos.emplace((int64_t) d);
                }

            }

            curPos += length;
            if (curPos >= pBytecode->length()) {
                break;
            }
        }

        return Status::Ok();
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


        // test
//        TestVisitor testVisitor;
//        testVisitor.visitFunctionNode(pFunction->node());

        BytecodeVisitor bytecodeVisitor(*code);
        pFunction->node()->visit(&bytecodeVisitor);


        return pStatus;

    }

    StackValue::StackValue(double dVal) : dVal(dVal) {}

    StackValue::StackValue(int64_t iVal) : iVal(iVal) {}

    StackValue::StackValue(const string &sVal) : sVal(sVal) {}

    const double &StackValue::getDVal() const {
        return dVal;
    }

    const int64_t &StackValue::getIVal() const {
        return iVal;
    }

    const string &StackValue::getSVal() const {
        return sVal;
    }
}

