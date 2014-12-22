#include <ast.h>
#include "interpreter_code_impl.h"
#include "stack_var.h"
#include <iostream>

namespace mathvm {
    template<>
    void StackVar::setVal<int64_t>(int64_t v) {
        intVal = v;
    }

    template<>
    void StackVar::setVal<double>(double v) {
        doubleVal = v;
    }

    template<>
    void StackVar::setVal<uint16_t>(uint16_t v) {
        stringId = v;
    }

    template<>
    int64_t StackVar::val<int64_t>() {
        return intVal;
    }

    template<>
    double StackVar::val<double>() {
        return doubleVal;
    }

    template<>
    uint16_t StackVar::val<uint16_t>() {
        return stringId;
    }


    struct InterpreterScope : public ErrorInfoHolder {
        BytecodeFunction *currentFunction;
        vector<StackVar *> currentContext;
        StackVar *nullStackVar;
        InterpreterScope *parent;
        int32_t currentPosition = 0;

        InterpreterScope(BytecodeFunction *bytecodeFunction, InterpreterScope *parent = 0)
                : nullStackVar(0), parent(parent) {
            currentFunction = bytecodeFunction;
            currentContext = vector<StackVar *>(bytecodeFunction->localsNumber(), 0);
            currentPosition = 0;
        }

        uint16_t context_id() {
            return currentFunction->id();
        }

        void error(const char *format, ...) {
            va_list args;
            va_start(args, format);
            verror(currentPosition, format, args);
        }


        StackVar *&lookUpVar(uint16_t context, uint16_t id) {
            if (context_id() == context) {
                return currentContext[id];
            } else {
                if (parent) {
                    return parent->lookUpVar(context, id);
                } else return nullStackVar;
            }
        }

        StackVar *&lookUpVar(uint16_t id) {
            return currentContext[id];
        }

        template<class T>
        void setVar(T v, uint16_t context_id, uint16_t id) {
            StackVar *&stackVar = lookUpVar(context_id, id);
            stackVar = new StackVar();
            stackVar->setVal<T>(v);
        }

        template<class T>
        void setVar(T v, uint16_t id) {
            StackVar *&stackVar = lookUpVar(id);
            stackVar = new StackVar();
            stackVar->setVal<T>(v);
        }


        Bytecode *byteCode() {
            return currentFunction->bytecode();
        }

        Instruction next() {
            if (currentPosition >= byteCode()->length()) {
                error("no bytecode left for %s", currentFunction->name().c_str());
            }
            Instruction instruction = byteCode()->getInsn(currentPosition);
            currentPosition += 1;
            return instruction;
        }

        template<class T>
        T getTyped() {
            T val = byteCode()->getTyped<T>(currentPosition);
            currentPosition += sizeof(T);
            return val;
        }

    };

    class Interpreter {
        typedef uint16_t Id;
        typedef uint16_t Context;
        typedef uint16_t String;
        typedef int16_t Ofset;

        InterpreterScope *currentScope;
        vector<StackVar> stack;


        void pushScope(BytecodeFunction *function) {
            InterpreterScope *newScope = new InterpreterScope(function, currentScope);
            currentScope = newScope;
        }

        void popScope() {
            currentScope = currentScope->parent;
        }

        void checkEmpty() {
            if (stack.empty()) {
                currentScope->error("empty stack");
            }
        }

        void checkInit(StackVar *stackVar, Id id) {
            if (stackVar == 0) {
                currentScope->error("uninitized var %d", id);
            }
        }


    public :
        Status *execute(InterpreterCodeImpl *code) {
            BytecodeFunction *main = dynamic_cast<BytecodeFunction *> (code->functionById(0));
            currentScope = new InterpreterScope(main);
            StackVar tempVar;
            uint16_t tempContext;
            uint16_t tempId;
            int64_t tempInt;
            double tempDouble;

            while (true) {
                Instruction instruction = currentScope->next();
                switch (instruction) {
                    case BC_INVALID: {
                        currentScope->error("INVALID BYTECODE");
                    };
//---------------------------------PRINTS
                    case (BC_IPRINT): {
                        checkEmpty();
                        tempVar = stack.back();
                        tempInt = tempVar.val<int64_t>();
                        cout << tempInt;
                        stack.pop_back();
                        break;
                    };
                    case (BC_DPRINT): {
                        checkEmpty();
                        tempVar = stack.back();
                        tempDouble = tempVar.val<double>();
                        cout << tempDouble;
                        stack.pop_back();
                        break;
                    };
                    case (BC_SPRINT): {
                        checkEmpty();
                        tempVar = stack.back();
                        tempId = tempVar.val<String>();
                        cout << code->constantById(tempId);
                        stack.pop_back();
                        break;
                    };
//---------------------------------LOADS
                    case (BC_DLOAD): {
                        StackVar var;
                        var.setVal<double>(currentScope->getTyped<double>());
                        stack.push_back(var);
                        break;
                    };
                    case (BC_ILOAD): {
                        StackVar var;
                        var.setVal<int64_t>(currentScope->getTyped<int64_t>());
                        stack.push_back(var);
                        break;
                    };
                    case (BC_ILOAD0): {
                        StackVar var;
                        var.setVal<int64_t>(0);
                        stack.push_back(var);
                        break;
                    };
                    case (BC_ILOAD1): {
                        StackVar var;
                        var.setVal<int64_t>(1);
                        stack.push_back(var);
                        break;
                    }
                    case (BC_ILOADM1): {
                        StackVar var;
                        var.setVal<int64_t>(-1);
                        stack.push_back(var);
                        break;
                    }
                    case (BC_SLOAD): {
                        StackVar var;
                        tempId = currentScope->getTyped<String>();
                        var.setVal<int64_t>(tempId);
                        stack.push_back(var);
                        break;
                    }
//--------------------------------LOAD VARS
                    case BC_LOADIVAR:
                    case BC_LOADDVAR:
                    case BC_LOADSVAR: {
                        tempId = currentScope->getTyped<Id>();
                        StackVar *tempVarPoiner = currentScope->lookUpVar(tempId);
                        checkInit(tempVarPoiner, tempId);
                        stack.push_back(*tempVarPoiner);
                        break;
                    }
// --------------------------------LOAD CONTEXT VARS
                    case BC_LOADCTXIVAR:
                    case BC_LOADCTXDVAR:
                    case BC_LOADCTXSVAR: {
                        tempContext = currentScope->getTyped<Context>();
                        tempId = currentScope->getTyped<Id>();
                        StackVar *tempVarPoiner = currentScope->lookUpVar(tempContext, tempId);
                        checkInit(tempVarPoiner, tempId);
                        stack.push_back(*tempVarPoiner);
                        break;
                    }
                    case (BC_STOREIVAR): {
                        checkEmpty();
                        tempVar = stack.back();
                        stack.pop_back();
                        tempId = currentScope->getTyped<Id>();
                        currentScope->setVar(tempVar.val<int64_t>(), tempId);
                        break;
                    }
                    case (BC_STOREDVAR): {
                        checkEmpty();
                        tempVar = stack.back();
                        stack.pop_back();
                        tempId = currentScope->getTyped<uint16_t>();
                        currentScope->setVar(tempVar.val<double>(), tempId);
                        break;
                    }
                    case (BC_STORESVAR): {
                        checkEmpty();
                        tempVar = stack.back();
                        stack.pop_back();
                        tempId = currentScope->getTyped<Id>();
                        currentScope->setVar(tempVar.val<String>(), tempId);
                        break;
                    }
                    case (BC_STORECTXIVAR): {
                        checkEmpty();
                        tempVar = stack.back();
                        stack.pop_back();
                        tempId = currentScope->getTyped<Id>();
                        tempContext = currentScope->getTyped<Context>();
                        currentScope->setVar(tempVar.val<int64_t>(), tempContext, tempId);
                        break;
                    }
                    case (BC_STORECTXDVAR): {
                        checkEmpty();
                        tempVar = stack.back();
                        stack.pop_back();
                        tempId = currentScope->getTyped<uint16_t>();
                        tempContext = currentScope->getTyped<Context>();
                        currentScope->setVar(tempVar.val<double>(), tempContext, tempId);
                        break;
                    }
                    case (BC_STORECTXSVAR): {
                        checkEmpty();
                        tempVar = stack.back();
                        stack.pop_back();
                        tempId = currentScope->getTyped<Id>();
                        tempContext = currentScope->getTyped<Context>();
                        currentScope->setVar(tempVar.val<String>(), tempContext, tempId);
                        break;
                    }
//---------------------------------CALL_RETURN
                    case (BC_CALL): {
                        tempContext = currentScope->getTyped<Context>();
                        pushScope(dynamic_cast<BytecodeFunction *>(code->functionById(tempContext)));
                        break;
                    }

                    case BC_RETURN: {
                        popScope();
                        break;
                    }

//----------------------CASTS
                    case BC_D2I: {
                        double upper = stack.back().doubleVal;
                        stack.back().setVal<int64_t>((int64_t) upper);
                        break;
                    };
                    case BC_I2D: {
                        int64_t upper = stack.back().intVal;
                        stack.back().setVal<double>((double) upper);
                        break;
                    };
//------------------------UNARY
                    case BC_DNEG: {
                        double upper = stack.back().doubleVal;
                        stack.back().setVal<double>(-1*upper);
                        break;
                    };
                    case BC_INEG: {
                        int64_t upper = stack.back().intVal;
                        stack.back().setVal<int64_t>(-1*upper);
                        break;
                    };


//------------------------------------BYNARY
                    case BC_DADD: {
                        double upper = stack.back().doubleVal;
                        stack.pop_back();
                        double lower = stack.back().doubleVal;
                        stack.back().setVal<double>(upper + lower);
                        break;
                    };
                    case BC_DDIV: {
                        double upper = stack.back().doubleVal;
                        stack.pop_back();
                        double lower = stack.back().doubleVal;
                        stack.back().setVal<double>(upper / lower);
                        break;
                    };
                    case BC_DMUL: {
                        double upper = stack.back().doubleVal;
                        stack.pop_back();
                        double lower = stack.back().doubleVal;
                        stack.back().setVal<double>(upper * lower);
                        break;
                    };

                    case BC_DSUB: {
                        double upper = stack.back().doubleVal;
                        stack.pop_back();
                        double lower = stack.back().doubleVal;
                        stack.back().setVal<double>(upper - lower);
                        break;
                    };
                    case BC_IADD: {
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.back().setVal<int64_t>(upper + lower);
                        break;
                    };
                    case BC_IMUL: {
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.back().setVal<int64_t>(upper * lower);
                        break;
                    };
                    case BC_ISUB: {
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.back().setVal<int64_t>(upper - lower);
                        break;
                    };
                    case BC_IDIV: {
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.back().setVal<int64_t>(upper / lower);
                        break;
                    };

                    case BC_IAAND: {
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.back().setVal<int64_t>(upper & lower);
                        break;
                    };

                    case BC_IAOR: {
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.back().setVal<int64_t>(upper | lower);
                        break;
                    };
                    case BC_IAXOR: {
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.back().setVal<int64_t>(upper ^ lower);
                        break;
                    };

                    case BC_IMOD: {
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.back().setVal<int64_t>(upper % lower);
                        break;
                    };


                    case BC_ICMP: {
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.back().setVal<int64_t>((int64_t) ((upper > lower) - (lower > upper)));
                        break;
                    };
                    case BC_DCMP: {
                        double upper = stack.back().doubleVal;
                        stack.pop_back();
                        double lower = stack.back().doubleVal;
                        stack.back().setVal<double>(lower > upper - upper > lower);
                        break;
                    };


                    case BC_SWAP: {
                        std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
                        break;
                    };
                    case BC_IFICMPNE: {
                        Ofset ofset = currentScope->getTyped<Ofset>();
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.pop_back();
                        if (upper != lower) {
                            currentScope->currentPosition += ofset - 2;
                        }
                        break;
                    };
                    case BC_IFICMPE: {
                        Ofset ofset = currentScope->getTyped<Ofset>();
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.pop_back();
                        if (upper == lower) {
                            currentScope->currentPosition += ofset - 2;
                        }
                        break;
                    };
                    case BC_IFICMPG: {
                        Ofset ofset = currentScope->getTyped<Ofset>();
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.pop_back();
                        if (upper > lower) {
                            currentScope->currentPosition += ofset - 2;
                        }
                        break;
                    };
                    case BC_IFICMPGE: {
                        Ofset ofset = currentScope->getTyped<Ofset>();
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.pop_back();
                        if (upper >= lower) {
                            currentScope->currentPosition += ofset - 2;
                        }
                        break;
                    };
                    case BC_IFICMPL: {
                        Ofset ofset = currentScope->getTyped<Ofset>();
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.pop_back();
                        if (upper < lower) {
                            currentScope->currentPosition += ofset - 2;
                        }
                        break;
                    };
                    case BC_IFICMPLE: {
                        Ofset ofset = currentScope->getTyped<Ofset>();
                        int64_t upper = stack.back().intVal;
                        stack.pop_back();
                        int64_t lower = stack.back().intVal;
                        stack.pop_back();
                        if (upper <= lower) {
                            currentScope->currentPosition += ofset - 2;
                        }
                        break;
                    };
                    case BC_JA: {
                        Ofset ofset = currentScope->getTyped<Ofset>();
                        currentScope->currentPosition += ofset - 2;
                        break;
                    };
                    case BC_POP: {
                        stack.pop_back();
                        break;
                    };


                    case BC_STOP: {
                        cout.flush();
                        return Status::Ok();
                    }
                    default:
                        currentScope->error("unhandled operation %s", bytecodeName(instruction, 0));
                }
            }
        }
    };

    Status *InterpreterCodeImpl::execute(vector<Var *> &vars) {
        Interpreter interpreter;
        try {
            interpreter.execute(this);
        } catch (ErrorInfoHolder *error) {
            return Status::Error(error->getMessage(), error->getPosition());

        }
        return Status::Ok();
    }
}