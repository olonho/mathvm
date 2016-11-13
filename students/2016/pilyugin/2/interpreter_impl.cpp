#include "interpreter_impl.h"

using namespace mathvm;

bool InterpreterCodeImpl::execute(Instruction insn) {
    switch (insn) {
        case BC_DLOAD:
            stack_.push(scope_->getDouble());
            break;
        case BC_ILOAD:
            stack_.push(scope_->getInt());
            break;
        case BC_SLOAD:
            stack_.push(scope_->getUInt16());
            break;
        case BC_DLOAD0:
            stack_.push((double) 0.0);
            break;
        case BC_ILOAD0:
            stack_.push((int64_t) 0);
            break;
        case BC_SLOAD0:
            stack_.push(makeStringConstant(""));
            break;
        case BC_DLOAD1:
            stack_.push((double) 1.0);
            break;
        case BC_ILOAD1:
            stack_.push((int64_t) 1);
            break;
        case BC_DLOADM1:
            stack_.push((double) -1.0);
            break;
        case BC_ILOADM1:
            stack_.push((int64_t) -1);
            break;

        case BC_DADD:
            stack_.push(loadTos().getDouble() + loadTos().getDouble());
            break;
        case BC_IADD:
            stack_.push(loadTos().getInt() + loadTos().getInt());
            break;
        case BC_DSUB:
            stack_.push(loadTos().getDouble() - loadTos().getDouble());
            break;
        case BC_ISUB: {
            stack_.push(loadTos().getInt() - loadTos().getInt());
            break;
        }
        case BC_DMUL:
            stack_.push(loadTos().getDouble() * loadTos().getDouble());
            break;
        case BC_IMUL:
            stack_.push(loadTos().getInt() * loadTos().getInt());
            break;
        case BC_DDIV:
            stack_.push(loadTos().getDouble() / loadTos().getDouble());
            break;
        case BC_IDIV:
            stack_.push(loadTos().getInt() / loadTos().getInt());
            break;
        case BC_IMOD:
            stack_.push(loadTos().getInt() % loadTos().getInt());
            break;

        case BC_IAOR:
            stack_.push(loadTos().getInt() | loadTos().getInt());
            break;
        case BC_IAAND:
            stack_.push(loadTos().getInt() & loadTos().getInt());
            break;
        case BC_IAXOR:
            stack_.push(loadTos().getInt() ^ loadTos().getInt());
            break;

        case BC_DNEG: {
            double value = loadTos().getDouble();
            stack_.push(-value);
            break;
        };
        case BC_INEG: {
            int64_t value = loadTos().getInt();
            stack_.push(-value);
            break;
        };

        case BC_DPRINT:
            cout << loadTos().getDouble();
            break;
        case BC_IPRINT:
            cout << loadTos().getInt();
            break;
        case BC_SPRINT:
            cout << constantById(loadTos().getUint16());
            break;

        case BC_I2D: {
            int64_t data = loadTos().getInt();
            stack_.push((double) data);
            break;
        };
        case BC_D2I: {
            double value = loadTos().getDouble();
            stack_.push((int64_t) value);
            break;
        };
        case BC_S2I: {
            uint16_t value = loadTos().getUint16();
            stack_.push((int64_t) value);
            break;
        };

        case BC_SWAP:
            swapTos();
            break;
        case BC_POP:
            stack_.pop();
            break;
        case BC_LOADIVAR0:
        case BC_LOADDVAR0:
        case BC_LOADSVAR0:
            stack_.push(scope_->getVarById(0));
            break;
        case BC_LOADIVAR1:
        case BC_LOADDVAR1:
        case BC_LOADSVAR1:
            stack_.push(scope_->getVarById(1));
            break;
        case BC_LOADIVAR2:
        case BC_LOADDVAR2:
        case BC_LOADSVAR2:
            stack_.push(scope_->getVarById(2));
            break;
        case BC_LOADIVAR3:
        case BC_LOADDVAR3:
        case BC_LOADSVAR3:
            stack_.push(scope_->getVarById(3));
            break;

        case BC_LOADIVAR:
        case BC_LOADDVAR:
        case BC_LOADSVAR:
            stack_.push(scope_->getVar());
            break;

        case BC_STOREIVAR0:
        case BC_STOREDVAR0:
        case BC_STORESVAR0:
            scope_->storeVarById(loadTos(), 0);
            break;
        case BC_STOREIVAR1:
        case BC_STOREDVAR1:
        case BC_STORESVAR1:
            scope_->storeVarById(loadTos(), 1);
            break;
        case BC_STOREIVAR2:
        case BC_STOREDVAR2:
        case BC_STORESVAR2:
            scope_->storeVarById(loadTos(), 2);
            break;
        case BC_STOREIVAR3:
        case BC_STOREDVAR3:
        case BC_STORESVAR3:
            scope_->storeVarById(loadTos(), 3);
            break;
        case BC_STOREIVAR:
        case BC_STOREDVAR:
        case BC_STORESVAR: {
            scope_->storeVar(loadTos());
            break;
        }

        case BC_LOADCTXIVAR:
        case BC_LOADCTXDVAR:
        case BC_LOADCTXSVAR:
            stack_.push(scope_->getContextVar());
            break;
        case BC_STORECTXIVAR:
        case BC_STORECTXDVAR:
        case BC_STORECTXSVAR: {
            scope_->storeContextVar(loadTos());
            break;
        }
        case BC_ICMP: {
            int64_t first = loadTos().getInt();
            int64_t second = loadTos().getInt();
            if (first < second) {
                stack_.push((int64_t) -1);
            } else if (first == second) {
                stack_.push((int64_t) 0);
            } else {
                stack_.push((int64_t) 1);
            }
            break;
        };
        case BC_DCMP: {
            double first = loadTos().getDouble();
            double second = loadTos().getDouble();
            if (first < second) {
                stack_.push((int64_t) -1);
            } else if (first == second) {
                stack_.push((int64_t) 0);
            } else {
                stack_.push((int64_t) 1);
            }
            break;
        };

        case BC_JA:
            scope_->jump(true);
            break;
        case BC_IFICMPNE:
            scope_->jump(loadTos().getInt() != loadTos().getInt());
            break;
        case BC_IFICMPE:
            scope_->jump(loadTos().getInt() == loadTos().getInt());
            break;
        case BC_IFICMPG:
            scope_->jump(loadTos().getInt() > loadTos().getInt());
            break;
        case BC_IFICMPGE:
            scope_->jump(loadTos().getInt() >= loadTos().getInt());
            break;
        case BC_IFICMPL:
            scope_->jump(loadTos().getInt() < loadTos().getInt());
            break;
        case BC_IFICMPLE:
            scope_->jump(loadTos().getInt() <= loadTos().getInt());
            break;
        case BC_STOP:
            return true;

        case BC_CALL: {
            uint16_t id = scope_->getUInt16();
            BytecodeFunction* function = (BytecodeFunction*) functionById(id);
            scope_ = new InterpreterScope(function, scope_);
            break;
        };
        case BC_RETURN: {
            InterpreterScope* innerScope = scope_;
            scope_ = scope_->getParent();
            delete innerScope;
            break;
        };

        default:
            throw InterpreterException("Invalid instruction");
    }
    return false;
}

StackValue InterpreterCodeImpl::loadTos() {
    auto value = stack_.top();
    stack_.pop();
    return value;
}

void InterpreterCodeImpl::swapTos() {
    StackValue top = stack_.top();
    stack_.pop();
    std::swap(top, stack_.top());
    stack_.push(top);
}
