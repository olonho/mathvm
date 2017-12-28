#include <cmath>
#include <limits>
#include "ast.h"
#include "interpreter_code.h"

namespace mathvm {

    InterpreterCodeImpl::InterpreterCodeImpl() : _currentContext(nullptr) {
    }

    InterpreterCodeImpl::~InterpreterCodeImpl() {
        while (_currentContext) {
            clearContext();
        }
    }

    Status* InterpreterCodeImpl::execute(vector<Var*>&) {
        try {
            auto* top = (BytecodeFunction*) functionByName(AstFunction::top_name);
            prepareContext(top);
//            getContext()->getFunction()->bytecode()->dump(cout);

            while (hasInstructions()) {
                while (hasInstructions() &&
                       executeInstruction(getContext()->readInsn()));
                // debug
                break;
            }
        } catch (const std::exception& e) {
            return Status::Error(e.what());
        }

        return Status::Ok();
    }

    bool InterpreterCodeImpl::executeInstruction(Instruction insn) {
//        getContext()->getFunction()->bytecode()->dump(cout);
        switch (insn) {
            case BC_DLOAD: {
                dpush(getContext()->readDouble());
                break;
            }
            case BC_ILOAD: {
                ipush(getContext()->readInt());
                break;
            }
            case BC_SLOAD: {
                spush(getContext()->readId());
                break;
            }
            case BC_DLOAD0: {
                dpush(0.0);
                break;
            }
            case BC_ILOAD0: {
                ipush(0L);
                break;
            }
            case BC_SLOAD0: {
                spush(makeStringConstant(""));
                break;
            }
            case BC_DLOAD1: {
                dpush(1.0);
                break;
            }
            case BC_ILOAD1: {
                ipush(1L);
                break;
            }
            case BC_DLOADM1: {
                dpush(-1.0);
                break;
            }
            case BC_ILOADM1: {
                ipush(-1L);
                break;
            }
            case BC_DADD: {
                dpush(pop().d + pop().d);
                break;
            }
            case BC_IADD: {
                ipush(pop().i + pop().i);
                break;
            }
            case BC_DSUB: {
                dpush(pop().d - pop().d);
                break;
            }
            case BC_ISUB: {
                ipush(pop().i - pop().i);
                break;
            }
            case BC_DMUL: {
                dpush(pop().d * pop().d);
                break;
            }
            case BC_IMUL: {
                ipush(pop().i * pop().i);
                break;
            }
            case BC_DDIV: {
                dpush(pop().d / pop().d);
                break;
            }
            case BC_IDIV: {
                ipush(pop().i / pop().i);
                break;
            }
            case BC_IMOD: {
                ipush(pop().i % pop().i);
                break;
            }
            case BC_DNEG: {
                dpush(-pop().d);
                break;
            }
            case BC_INEG: {
                ipush(-pop().i);
                break;
            }
            case BC_IAOR: {
                ipush(pop().i | pop().i);
                break;
            }
            case BC_IAAND: {
                ipush(pop().i & pop().i);
                break;
            }
            case BC_IAXOR: {
                ipush(pop().i ^ pop().i);
                break;
            }
            case BC_IPRINT: {
                std::cout << pop().i;
                break;
            }
            case BC_DPRINT: {
                std::cout << pop().d;
                break;
            }
            case BC_SPRINT: {
                std::cout << constantById(pop().s);
                break;
            }
            case BC_I2D: {
                dpush(static_cast<double>(pop().i));
                break;
            }
            case BC_D2I: {
                dpush(static_cast<int64_t>(pop().d));
                break;
            }
            case BC_S2I: {
                ipush(static_cast<int64_t>(stol(constantById(pop().s))));
                break;
            }
            case BC_SWAP: {
                Val lhs = pop();
                Val rhs = pop();
                push(lhs);
                push(rhs);
                break;
            }
            case BC_POP: {
                pop();
                break;
            }
            case BC_LOADDVAR0:
            case BC_LOADIVAR0:
            case BC_LOADSVAR0: {
                push(getContext()->getVar(0));
                break;
            }
            case BC_LOADDVAR1:
            case BC_LOADIVAR1:
            case BC_LOADSVAR1: {
                push(getContext()->getVar(1));
                break;
            }
            case BC_LOADDVAR2:
            case BC_LOADIVAR2:
            case BC_LOADSVAR2: {
                push(getContext()->getVar(2));
                break;
            }
            case BC_LOADDVAR3:
            case BC_LOADIVAR3:
            case BC_LOADSVAR3: {
                push(getContext()->getVar(3));
                break;
            }
            case BC_STOREDVAR0:
            case BC_STOREIVAR0:
            case BC_STORESVAR0: {
                getContext()->setVar(0, pop());
                break;
            }
            case BC_STOREDVAR1:
            case BC_STOREIVAR1:
            case BC_STORESVAR1: {
                getContext()->setVar(1, pop());
                break;
            }
            case BC_STOREDVAR2:
            case BC_STOREIVAR2:
            case BC_STORESVAR2: {
                getContext()->setVar(2, pop());
                break;
            }
            case BC_STOREDVAR3:
            case BC_STOREIVAR3:
            case BC_STORESVAR3: {
                getContext()->setVar(3, pop());
                break;
            }
            case BC_LOADDVAR:
            case BC_LOADIVAR:
            case BC_LOADSVAR: {
                push(getContext()->getVar(getContext()->readId()));
                break;
            }
            case BC_STOREDVAR:
            case BC_STOREIVAR:
            case BC_STORESVAR: {
                getContext()->setVar(getContext()->readId(), pop());
                break;
            }
            case BC_LOADCTXDVAR:
            case BC_LOADCTXIVAR:
            case BC_LOADCTXSVAR: {
                id_t varId = getContext()->readId();
                id_t ctxId = getContext()->readId();
                push(getContext(ctxId)->getVar(varId));
                break;
            }
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR: {
                id_t varId = getContext()->readId();
                id_t ctxId = getContext()->readId();
                getContext(ctxId)->setVar(varId, pop());
                break;
            }
            case BC_DCMP: {
                double lhs = pop().d;
                double rhs = pop().d;
                if (lhs - rhs > std::numeric_limits<double>::epsilon()) {
                    ipush(1L);
                } else if (rhs - lhs > std::numeric_limits<double>::epsilon()) {
                    ipush(-1L);
                } else {
                    ipush(0L);
                }
                break;
            }
            case BC_ICMP: {
                int64_t lhs = pop().i;
                int64_t rhs = pop().i;
                if (lhs > rhs) {
                    ipush(1L);
                } else if (lhs < rhs) {
                    ipush(-1L);
                } else {
                    ipush(0L);
                }
                break;
            }
            case BC_JA: {
                auto offset = getContext()->readOffset();
                getContext()->jump(offset);
                break;
            }
            case BC_IFICMPNE: {
                auto offset = getContext()->readOffset();
                if (pop().i != pop().i) {
                    getContext()->jump(offset);
                }
                break;
            }
            case BC_IFICMPE: {
                auto offset = getContext()->readOffset();
                if (pop().i == pop().i) {
                    getContext()->jump(offset);
                }
                break;
            }
            case BC_IFICMPG: {
                auto offset = getContext()->readOffset();
                if (pop().i > pop().i) {
                    getContext()->jump(offset);
                }
                break;
            }
            case BC_IFICMPGE: {
                auto offset = getContext()->readOffset();
                if (pop().i >= pop().i) {
                    getContext()->jump(offset);
                }
                break;
            }
            case BC_IFICMPL: {
                auto offset = getContext()->readOffset();
                if (pop().i < pop().i) {
                    getContext()->jump(offset);
                }
                break;
            }
            case BC_IFICMPLE: {
                auto offset = getContext()->readOffset();
                if (pop().i <= pop().i) {
                    getContext()->jump(offset);
                }
                break;
            }
            case BC_DUMP: {
                Val val = top();
                std::cout << "--- dump ---" << std::endl;
                std::cout << "int: " << val.i << std::endl;
                std::cout << "double: " << val.d << std::endl;
                std::cout << "string id: " << val.s << std::endl;
                std::cout << "--- end dump ---" << std::endl;
                break;
            }
            case BC_STOP: {
                return false;
            }
            case BC_CALL: {
                id_t funcId = getContext()->readId();
                auto* func = dynamic_cast<BytecodeFunction*>(functionById(funcId));
                prepareContext(func);
                break;
            }
            case BC_CALLNATIVE: {
                assert("TODO: call_native" && false);
                break;
            }
            case BC_RETURN: {
                clearContext();
                break;
            }
            case BC_BREAK: {
                return false;
            }
            default: {
                throw std::runtime_error("Unknown instruction");
            }
        }

        return true;
    }

    bool InterpreterCodeImpl::hasInstructions() {
        return getContext()->getBytecodePosition() < getContext()->getBytecode()->length();
    }

    void InterpreterCodeImpl::push(Val v) {
        stack.push_back(v);
    }

    void InterpreterCodeImpl::dpush(double d) {
        Val v = {};
        v.d = d;
        push(v);
    }

    void InterpreterCodeImpl::ipush(int64_t i) {
        Val v = {};
        v.i = i;
        push(v);
    }

    void InterpreterCodeImpl::spush(id_t s) {
        Val v = {};
        v.s = s;
        push(v);
    }

    Val InterpreterCodeImpl::pop() {
        Val v = stack.back();
        stack.pop_back();
        return v;
    }

    Val InterpreterCodeImpl::top() {
        return stack.back();
    }

    Context* InterpreterCodeImpl::getContext() {
        return _currentContext;
    }

    Context* InterpreterCodeImpl::getContext(id_t ctxId) {
        Context* context = _currentContext;
        while (context != nullptr && context->getId() != ctxId) {
            context = context->getParent();
        }

        if (context == nullptr) {
            throw std::runtime_error("Unknown context");
        }

        return context;
    }

    void InterpreterCodeImpl::prepareContext(BytecodeFunction* func) {
        _currentContext = new Context(func, _currentContext);
    }

    void InterpreterCodeImpl::clearContext() {
        Context* parentContext = _currentContext->getParent();
        delete _currentContext;
        _currentContext = parentContext;
    }

    Bytecode* InterpreterCodeImpl::getBytecode() {
        return getContext()->getBytecode();
    }

}
