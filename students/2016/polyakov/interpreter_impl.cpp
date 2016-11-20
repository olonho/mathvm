#include "mathvm.h"
#include "interpreter_impl.h"

namespace mathvm { 

struct BinOperands {
    StackVar left, right;
    BinOperands(StackVar& left, StackVar& right) :
        left(left), right(right)
    {}

    static BinOperands get(std::vector<StackVar>& stack) {
        auto lhs = stack.back();
        stack.pop_back();
        auto rhs = stack.back();
        stack.pop_back();
        return BinOperands(lhs, rhs);        
    }
};

Status* InterpreterCodeImpl::execute(std::vector<Var *> &vars) {
    try {
        BytecodeFunction* top = (BytecodeFunction*) functionById(0);
        context = new InterprContext(top, NULL);
        stack.resize(0);
        while (context->bci() < bc()->length()) {
            if (executeInsn(context->nextInsn())) {
                break;
            }
        }
    } catch(std::runtime_error* e) {
        return Status::Error(e->what(), context->bci());
    } catch(...) {
        return Status::Error("Interpreter error", context->bci());
    }
    return Status::Ok();
}

bool InterpreterCodeImpl::executeInsn(Instruction ins) {
    switch (ins) {
        case BC_ILOAD:
            stack.push_back(StackVar::getFrom(context->getInt()));
            break;
        case BC_DLOAD:
            stack.push_back(StackVar::getFrom(context->getDouble()));
            break;
        case BC_SLOAD: {
            const char * strVal = constantById(context->getUInt16()).c_str();
            stack.push_back(StackVar::getFrom(strVal));
            break;
        };
        case BC_ILOAD0:
            stack.push_back(StackVar::getFrom((int64_t)0));
            break;
        case BC_DLOAD0:
            stack.push_back(StackVar::getFrom((double)0.0));
            break;
        case BC_SLOAD0:
            stack.push_back(StackVar::getFrom(""));
            break;
        case BC_ILOAD1:
            stack.push_back(StackVar::getFrom((int64_t)1));
            break;
        case BC_DLOAD1:
            stack.push_back(StackVar::getFrom((double)1.0));
            break;
        case BC_ILOADM1:
            stack.push_back(StackVar::getFrom((int64_t)-1));
            break;
        case BC_DLOADM1:
            stack.push_back(StackVar::getFrom((double)-1.0));
            break;
        case BC_IADD: {
            BinOperands data = BinOperands::get(stack);
            stack.push_back(StackVar::getFrom(data.left.getIntValue() + data.right.getIntValue()));
            break;
        };
        case BC_DADD: {
            BinOperands data = BinOperands::get(stack);
            stack.push_back(StackVar::getFrom(data.left.getDoubleValue() + data.right.getDoubleValue()));
            break;
        };
        case BC_ISUB: {
            BinOperands data = BinOperands::get(stack);
            stack.push_back(StackVar::getFrom(data.left.getIntValue() - data.right.getIntValue()));
            break;
        };
        case BC_DSUB: {
            BinOperands data = BinOperands::get(stack);
            stack.push_back(StackVar::getFrom(data.left.getDoubleValue() - data.right.getDoubleValue()));
            break;
        };
        case BC_IMUL: {
            BinOperands data = BinOperands::get(stack);
            stack.push_back(StackVar::getFrom(data.left.getIntValue() * data.right.getIntValue()));
            break;
        };
        case BC_DMUL: {
            BinOperands data = BinOperands::get(stack);
            stack.push_back(StackVar::getFrom(data.left.getDoubleValue() * data.right.getDoubleValue()));
            break;
        };
        case BC_IDIV: {
            BinOperands data = BinOperands::get(stack);
            if (data.right.getIntValue() == 0) {
                throw new std::runtime_error("Division by zero");
            }
            stack.push_back(StackVar::getFrom(data.left.getIntValue() / data.right.getIntValue()));
            break;
        };
        case BC_DDIV: {
            BinOperands data = BinOperands::get(stack);
            if (data.right.getDoubleValue() == 0.0) {
                throw new std::runtime_error("Division by zero");
            }
            stack.push_back(StackVar::getFrom(data.left.getDoubleValue() / data.right.getDoubleValue()));
            break;
        };

        case BC_IMOD: {
            BinOperands data = BinOperands::get(stack);
            if (data.right.getIntValue() == 0) {
                throw new std::runtime_error("Division by zero");
            }
            stack.push_back(StackVar::getFrom(data.left.getIntValue() % data.right.getIntValue()));
            break;
        };
        case BC_INEG: {
            auto data = stack.back().getIntValue();
            stack.pop_back();
            stack.push_back(StackVar::getFrom(- data));
            break;
        };
        case BC_DNEG: {
            auto data = stack.back().getDoubleValue();
            stack.pop_back();
            stack.push_back(StackVar::getFrom(- data));
            break;
        };
        case BC_IAOR: {
            BinOperands data = BinOperands::get(stack);
            stack.push_back(StackVar::getFrom(data.left.getIntValue() | data.right.getIntValue()));
            break;
        };
        case BC_IAAND: {
            BinOperands data = BinOperands::get(stack);
            stack.push_back(StackVar::getFrom(data.left.getIntValue() & data.right.getIntValue()));
            break;
        };
        case BC_IAXOR: {
            BinOperands data = BinOperands::get(stack);
            stack.push_back(StackVar::getFrom(data.left.getIntValue() ^ data.right.getIntValue()));
            break;
        };
        case BC_IPRINT: {
            auto i = stack.back().getIntValue();
            std::cout<< i;
            stack.pop_back();
            break;
        };

        case BC_DPRINT: {
            auto d = stack.back().getDoubleValue();
            std::cout<< d;
            stack.pop_back();
            break;
        };

        case BC_SPRINT: {
            std::cout << stack.back().getStringValue();
            stack.pop_back();
            break;
        };

        case BC_I2D: {
            auto data = stack.back().getIntValue();
            stack.pop_back();
            stack.push_back(StackVar::getFrom((double)data));
            break;
        };
        case BC_D2I: {
            auto data = stack.back().getDoubleValue();
            stack.pop_back();
            stack.push_back(StackVar::getFrom((int64_t)data));
            break;
        };
        case BC_S2I: {
            auto data = stack.back().getStringValue();
            stack.pop_back();
            stack.push_back(StackVar::getFrom((int64_t)data));
            break;
        };
        case BC_SWAP:
            std::swap(stack.back(), stack.at(stack.size() - 2));
            break;
        case BC_POP:
            stack.pop_back();
            break;
        case BC_LOADIVAR0:
        case BC_LOADDVAR0:
        case BC_LOADSVAR0:
            stack.push_back(context->getVarById(0));
            break;
        case BC_LOADIVAR1:
        case BC_LOADDVAR1:
        case BC_LOADSVAR1:
            stack.push_back(context->getVarById(1));
            break;
        case BC_LOADIVAR2:
        case BC_LOADDVAR2:
        case BC_LOADSVAR2:
            stack.push_back(context->getVarById(2));
            break;
        case BC_LOADIVAR3:
        case BC_LOADDVAR3:
        case BC_LOADSVAR3:
            stack.push_back(context->getVarById(3));
            break;

        case BC_LOADIVAR:
        case BC_LOADDVAR:
        case BC_LOADSVAR:
            stack.push_back(context->getVar());
            break;

        case BC_STOREIVAR0:
        case BC_STOREDVAR0:
        case BC_STORESVAR0:
            context->storeVarById(stack.back(), 0);
            stack.pop_back();
            break;
        case BC_STOREIVAR1:
        case BC_STOREDVAR1:
        case BC_STORESVAR1:
            context->storeVarById(stack.back(), 1);
            stack.pop_back();
            break;
        case BC_STOREIVAR2:
        case BC_STOREDVAR2:
        case BC_STORESVAR2:
            context->storeVarById(stack.back(), 2);
            stack.pop_back();
            break;
        case BC_STOREIVAR3:
        case BC_STOREDVAR3:
        case BC_STORESVAR3:
            context->storeVarById(stack.back(), 3);
            stack.pop_back();
            break;
        case BC_STOREIVAR:
        case BC_STOREDVAR:
        case BC_STORESVAR:
            context->storeVar(stack.back());
            stack.pop_back();
            break;

        case BC_LOADCTXIVAR:
        case BC_LOADCTXDVAR:
        case BC_LOADCTXSVAR:
            stack.push_back(context->getCTXVar());
            break;
        case BC_STORECTXIVAR:
        case BC_STORECTXDVAR:
        case BC_STORECTXSVAR:
            context->storeCTXVar(stack.back());
            stack.pop_back();
            break;
        case BC_ICMP: {
            BinOperands data = BinOperands::get(stack);
            if (data.left.getIntValue() < data.right.getIntValue()) {
                stack.push_back(StackVar::getFrom((int64_t)-1));
            } else if (data.left.getIntValue() == data.right.getIntValue()) {
                stack.push_back(StackVar::getFrom((int64_t)0));
            } else {
                stack.push_back(StackVar::getFrom((int64_t)1));
            }
            break;
        };
        case BC_DCMP: {
            BinOperands data = BinOperands::get(stack);
            if (data.left.getDoubleValue() < data.right.getDoubleValue()) {
                stack.push_back(StackVar::getFrom((int64_t)-1));
            } else if (data.left.getDoubleValue() == data.right.getDoubleValue()) {
                stack.push_back(StackVar::getFrom((int64_t)0));
            } else {
                stack.push_back(StackVar::getFrom((int64_t)1));
            }
            break;
        };
        case BC_JA:
            context->jump();
            break;
        case BC_IFICMPNE: {
            BinOperands data = BinOperands::get(stack);
            if (data.left.getIntValue() != data.right.getIntValue()) {
                context->jump();
            } else {
                context->getInt16();
            }
            break;
        };
        case BC_IFICMPE: {
            BinOperands data = BinOperands::get(stack);
            if (data.left.getIntValue() == data.right.getIntValue()) {
                context->jump();
            } else {
                context->getInt16();
            }
            break;
        };
        case BC_IFICMPG: {
            BinOperands data = BinOperands::get(stack);
            if (data.left.getIntValue() > data.right.getIntValue()) {
                context->jump();
            } else {
                context->getInt16();
            }
            break;
        };
        case BC_IFICMPGE: {
            BinOperands data = BinOperands::get(stack);
            if (data.left.getIntValue() >= data.right.getIntValue()) {
                context->jump();
            } else {
                context->getInt16();
            }
            break;
        };
        case BC_IFICMPL: {
            BinOperands data = BinOperands::get(stack);
            if (data.left.getIntValue() < data.right.getIntValue()) {
                context->jump();
            } else {
                context->getInt16();
            }
            break;
        };
        case BC_IFICMPLE: {
            BinOperands data = BinOperands::get(stack);
            if (data.left.getIntValue() <= data.right.getIntValue()) {
                context->jump();
            } else {
                context->getInt16();
            }
            break;
        };
        case BC_STOP:
            return true;

        case BC_CALL: {
            uint16_t bFuncid = context->getUInt16();
            BytecodeFunction* bFunc = (BytecodeFunction*) functionById(bFuncid);
            context = new InterprContext(bFunc, context);
            break;
        };
        case BC_CALLNATIVE:
            throw new runtime_error("Native calls doesn't supported");
        case BC_RETURN: {
            InterprContext* contextfor_change = context;
            context = context->getParent();
            delete contextfor_change;
            break;
        };
        default:
            throw new std::runtime_error("Incorrect instruction");
    }
    return false;
}

}
