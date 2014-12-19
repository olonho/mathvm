#include "bytecode_interpreter.hpp"

#include <stdexcept>

using namespace mathvm;

Status* InterpreterCode::execute(std::vector<Var *> &vars) {
    try {
        BytecodeFunction* top = (BytecodeFunction*) functionById(0);
        context_ = new InterpreterScopeContext(top, NULL);
        stack_.resize(0);
        while (context_->bci() < bc()->length()) {
            if (executeInstruction(context_->getInstruction())) {
                break;
            }
        }
    } catch(std::runtime_error* e) {
        return Status::Error(e->what(),context_->bci());
    } catch(...) {
        return Status::Error("Interpreter error", context_->bci());
    }
    return Status::Ok();
}

bool InterpreterCode::executeInstruction(Instruction ins) {

    switch (ins) {
        case BC_ILOAD:
            stack_.push_back(StackObj::create(context_->getInt()));
            break;
        case BC_DLOAD:
            stack_.push_back(StackObj::create(context_->getDouble()));
            break;
        case BC_SLOAD:
            stack_.push_back(StackObj::create(context_->getUInt16()));
            break;
        case BC_ILOAD0:
            stack_.push_back(StackObj::create((int64_t)0));
            break;
        case BC_DLOAD0:
            stack_.push_back(StackObj::create((double)0.0));
            break;
        case BC_SLOAD0:
            stack_.push_back(StackObj::create(makeStringConstant("")));
            break;
        case BC_ILOAD1:
            stack_.push_back(StackObj::create((int64_t)1));
            break;
        case BC_DLOAD1:
            stack_.push_back(StackObj::create((double)1.0));
            break;
        case BC_ILOADM1:
            stack_.push_back(StackObj::create((int64_t)-1));
            break;
        case BC_DLOADM1:
            stack_.push_back(StackObj::create((double)-1.0));
            break;
        case BC_IADD: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            stack_.push_back(StackObj::create(data.first.getAsInt() + data.second.getAsInt()));
            break;
        };
        case BC_DADD: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            stack_.push_back(StackObj::create(data.first.getAsDouble() + data.second.getAsDouble()));
            break;
        };
        case BC_ISUB: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            stack_.push_back(StackObj::create(data.first.getAsInt() - data.second.getAsInt()));
            break;
        };
        case BC_DSUB: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            stack_.push_back(StackObj::create(data.first.getAsDouble() - data.second.getAsDouble()));
            break;
        };
        case BC_IMUL: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            stack_.push_back(StackObj::create(data.first.getAsInt() * data.second.getAsInt()));
            break;
        };
        case BC_DMUL: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            stack_.push_back(StackObj::create(data.first.getAsDouble() * data.second.getAsDouble()));
            break;
        };
        case BC_IDIV: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.second.getAsInt() == (int64_t) 0) {
                throw new std::runtime_error("Division by zero");
            }
            stack_.push_back(StackObj::create(data.first.getAsInt() / data.second.getAsInt()));
            break;
        };
        case BC_DDIV: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.second.getAsDouble() == (double) 0.0) {
                throw new std::runtime_error("Division by zero");
            }
            stack_.push_back(StackObj::create(data.first.getAsDouble() / data.second.getAsDouble()));
            break;
        };

        case BC_IMOD: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.second.getAsInt() == (int64_t) 0) {
                throw new std::runtime_error("Division by zero");
            }
            stack_.push_back(StackObj::create(data.first.getAsInt() % data.second.getAsInt()));
            break;
        };
        case BC_INEG: {
            auto data = stack_.back().getAsInt();
            stack_.pop_back();
            stack_.push_back(StackObj::create(- data));
            break;
        };
        case BC_DNEG: {
            auto data = stack_.back().getAsDouble();
            stack_.pop_back();
            stack_.push_back(StackObj::create(- data));
            break;
        };
        case BC_IAOR: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            stack_.push_back(StackObj::create(data.first.getAsInt() | data.second.getAsInt()));
            break;
        };
        case BC_IAAND: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            stack_.push_back(StackObj::create(data.first.getAsInt() & data.second.getAsInt()));
            break;
        };
        case BC_IAXOR: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            stack_.push_back(StackObj::create(data.first.getAsInt() ^ data.second.getAsInt()));
            break;
        };
        case BC_IPRINT: {
            auto i = stack_.back().getAsInt();
            std::cout<< i;
            stack_.pop_back();
            break;
        };

        case BC_DPRINT: {
            auto d = stack_.back().getAsDouble();
            std::cout<< d;
            stack_.pop_back();
            break;
        };

        case BC_SPRINT: {
            auto x = constantById(stack_.back().getAsUInt16());
            std::cout<< x;
            stack_.pop_back();
            break;
        };

        case BC_I2D: {
            auto data = stack_.back().getAsInt();
            stack_.pop_back();
            stack_.push_back(StackObj::create((double)data));
            break;
        };
        case BC_D2I: {
            auto data = stack_.back().getAsDouble();
            stack_.pop_back();
            stack_.push_back(StackObj::create((int64_t)data));
            break;
        };
        case BC_S2I: {
            auto data = stack_.back().getAsUInt16();
            stack_.pop_back();
            stack_.push_back(StackObj::create((int64_t)data));
            break;
        };
        case BC_SWAP:
            std::swap(stack_.back(), stack_.at(stack_.size() - 2));
            break;
        case BC_POP:
            stack_.pop_back();
            break;
        case BC_LOADIVAR0:
        case BC_LOADDVAR0:
        case BC_LOADSVAR0:
            stack_.push_back(context_->getVarById(0));
            break;
        case BC_LOADIVAR1:
        case BC_LOADDVAR1:
        case BC_LOADSVAR1:
            stack_.push_back(context_->getVarById(1));
            break;
        case BC_LOADIVAR2:
        case BC_LOADDVAR2:
        case BC_LOADSVAR2:
            stack_.push_back(context_->getVarById(2));
            break;
        case BC_LOADIVAR3:
        case BC_LOADDVAR3:
        case BC_LOADSVAR3:
            stack_.push_back(context_->getVarById(3));
            break;

        case BC_LOADIVAR:
        case BC_LOADDVAR:
        case BC_LOADSVAR:
            stack_.push_back(context_->getVar());
            break;

        case BC_STOREIVAR0:
        case BC_STOREDVAR0:
        case BC_STORESVAR0:
            context_->storeVarById(stack_.back(), 0);
            stack_.pop_back();
            break;
        case BC_STOREIVAR1:
        case BC_STOREDVAR1:
        case BC_STORESVAR1:
            context_->storeVarById(stack_.back(), 1);
            stack_.pop_back();
            break;
        case BC_STOREIVAR2:
        case BC_STOREDVAR2:
        case BC_STORESVAR2:
            context_->storeVarById(stack_.back(), 2);
            stack_.pop_back();
            break;
        case BC_STOREIVAR3:
        case BC_STOREDVAR3:
        case BC_STORESVAR3:
            context_->storeVarById(stack_.back(), 3);
            stack_.pop_back();
            break;
        case BC_STOREIVAR:
        case BC_STOREDVAR:
        case BC_STORESVAR:
            context_->storeVar(stack_.back());
            stack_.pop_back();
            break;

        case BC_LOADCTXIVAR:
        case BC_LOADCTXDVAR:
        case BC_LOADCTXSVAR:
            stack_.push_back(context_->getContextVar());
            break;
        case BC_STORECTXIVAR:
        case BC_STORECTXDVAR:
        case BC_STORECTXSVAR:
            context_->storeContextVar(stack_.back());
            stack_.pop_back();
            break;
        case BC_ICMP: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.first.getAsInt() < data.second.getAsInt()) {
                stack_.push_back(StackObj::create((int64_t)-1));
            } else if (data.first.getAsInt() == data.second.getAsInt()) {
                stack_.push_back(StackObj::create((int64_t)0));
            } else {
                stack_.push_back(StackObj::create((int64_t)1));
            }
            break;
        };
        case BC_DCMP: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.first.getAsDouble() < data.second.getAsDouble()) {
                stack_.push_back(StackObj::create((int64_t)-1));
            } else if (data.first.getAsDouble() == data.second.getAsDouble()) {
                stack_.push_back(StackObj::create((int64_t)0));
            } else {
                stack_.push_back(StackObj::create((int64_t)1));
            }
            break;
        };
        case BC_JA:
            context_->jump();
            break;
        case BC_IFICMPNE: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.first.getAsInt() != data.second.getAsInt()) {
                context_->jump();
            } else {
                context_->getInt16();
            }
            break;
        };
        case BC_IFICMPE: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.first.getAsInt() == data.second.getAsInt()) {
                context_->jump();
            } else {
                context_->getInt16();
            }
            break;
        };
        case BC_IFICMPG: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.first.getAsInt() > data.second.getAsInt()) {
                context_->jump();
            } else {
                context_->getInt16();
            }
            break;
        };
        case BC_IFICMPGE: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.first.getAsInt() >= data.second.getAsInt()) {
                context_->jump();
            } else {
                context_->getInt16();
            }
            break;
        };
        case BC_IFICMPL: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.first.getAsInt() < data.second.getAsInt()) {
                context_->jump();
            } else {
                context_->getInt16();
            }
            break;
        };
        case BC_IFICMPLE: {
            std::pair<StackObj, StackObj> data(getOperandsForBinOp());
            if (data.first.getAsInt() <= data.second.getAsInt()) {
                context_->jump();
            } else {
                context_->getInt16();
            }
            break;
        };
        case BC_STOP:
            return true;

        case BC_CALL: {
            uint16_t bf_id = context_->getUInt16();
            BytecodeFunction* bf = (BytecodeFunction*)functionById(bf_id);
            context_ = new InterpreterScopeContext(bf, context_);
            break;
        };


        case BC_CALLNATIVE:
            throw new std::runtime_error("Native function does not support");

        case BC_RETURN: {
            InterpreterScopeContext* context_for_change = context_;
            context_ = context_->getParent();
            delete context_for_change;
            break;
        };

        default:
            throw new std::runtime_error("Uncorrect instruction");
    }
    return false;
}

std::pair<StackObj, StackObj> InterpreterCode::getOperandsForBinOp() {

    auto lhs = stack_.back();
    stack_.pop_back();
    auto rhs = stack_.back();
    stack_.pop_back();
    return std::make_pair(lhs, rhs);
}
