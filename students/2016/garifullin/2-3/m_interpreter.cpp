#include <stdexcept>

#include "m_interpreter.h"

using namespace mathvm;

Status* InterpreterCode::execute(std::vector<Var *> &) {
    try {
        BytecodeFunction* top = (BytecodeFunction*) functionById(0);
        _context = new InterpreterScopeContext(top, nullptr);
        _stack.resize(0);
        while (_context->bci() < bc()->length()) {
            if (executeInstruction(_context->getInsn())) {
                break;
            }
        }
    } catch(InterpreterError const &e) {
        return Status::Error(e.what());
    }
    return Status::Ok();
}

bool InterpreterCode::executeInstruction(Instruction ins) {

    switch (ins) {
    case BC_ILOAD:
        _stack.push_back(stack_var::create(_context->getInt()));
        break;
    case BC_DLOAD:
        _stack.push_back(stack_var::create(_context->getDouble()));
        break;
    case BC_SLOAD:
        _stack.push_back(stack_var::create(_context->getUInt16()));
        break;
    case BC_ILOAD0:
        _stack.push_back(stack_var::create((int64_t)0));
        break;
    case BC_DLOAD0:
        _stack.push_back(stack_var::create((double)0.0));
        break;
    case BC_SLOAD0:
        _stack.push_back(stack_var::create(makeStringConstant("")));
        break;
    case BC_ILOAD1:
        _stack.push_back(stack_var::create((int64_t)1));
        break;
    case BC_DLOAD1:
        _stack.push_back(stack_var::create((double)1.0));
        break;
    case BC_ILOADM1:
        _stack.push_back(stack_var::create((int64_t)-1));
        break;
    case BC_DLOADM1:
        _stack.push_back(stack_var::create((double)-1.0));
        break;
    case BC_IADD: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        _stack.push_back(stack_var::create(data.first.getAsInt() + data.second.getAsInt()));
        break;
    };
    case BC_DADD: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        _stack.push_back(stack_var::create(data.first.getAsDouble() + data.second.getAsDouble()));
        break;
    };
    case BC_ISUB: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        _stack.push_back(stack_var::create(data.first.getAsInt() - data.second.getAsInt()));
        break;
    };
    case BC_DSUB: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        _stack.push_back(stack_var::create(data.first.getAsDouble() - data.second.getAsDouble()));
        break;
    };
    case BC_IMUL: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        _stack.push_back(stack_var::create(data.first.getAsInt() * data.second.getAsInt()));
        break;
    };
    case BC_DMUL: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        _stack.push_back(stack_var::create(data.first.getAsDouble() * data.second.getAsDouble()));
        break;
    };
    case BC_IDIV: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.second.getAsInt() == (int64_t) 0) {
            throw InterpreterError("Division by zero");
        }
        _stack.push_back(stack_var::create(data.first.getAsInt() / data.second.getAsInt()));
        break;
    };
    case BC_DDIV: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.second.getAsDouble() == (double) .0) {
            throw InterpreterError("Division by zero");
        }
        _stack.push_back(stack_var::create(data.first.getAsDouble() / data.second.getAsDouble()));
        break;
    };

    case BC_IMOD: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.second.getAsInt() == (int64_t) 0) {
            throw InterpreterError("Division by zero");
        }
        _stack.push_back(stack_var::create(data.first.getAsInt() % data.second.getAsInt()));
        break;
    };
    case BC_INEG: {
        int64_t data = _stack.back().getAsInt();
        _stack.pop_back();
        _stack.push_back(stack_var::create(- data));
        break;
    };
    case BC_DNEG: {
        double data = _stack.back().getAsDouble();
        _stack.pop_back();
        _stack.push_back(stack_var::create(- data));
        break;
    };
    case BC_IAOR: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        _stack.push_back(stack_var::create(data.first.getAsInt() | data.second.getAsInt()));
        break;
    };
    case BC_IAAND: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        _stack.push_back(stack_var::create(data.first.getAsInt() & data.second.getAsInt()));
        break;
    };
    case BC_IAXOR: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        _stack.push_back(stack_var::create(data.first.getAsInt() ^ data.second.getAsInt()));
        break;
    };
    case BC_IPRINT: {
        int64_t i = _stack.back().getAsInt();
        std::cout<< i;
        _stack.pop_back();
        break;
    };

    case BC_DPRINT: {
        double d = _stack.back().getAsDouble();
        std::cout<< d;
        _stack.pop_back();
        break;
    };

    case BC_SPRINT: {
        const string &x = constantById(_stack.back().getAsUInt16());
        std::cout<< x;
        _stack.pop_back();
        break;
    };

    case BC_I2D: {
        int64_t data = _stack.back().getAsInt();
        _stack.pop_back();
        _stack.push_back(stack_var::create((double)data));
        break;
    };
    case BC_D2I: {
        double data = _stack.back().getAsDouble();
        _stack.pop_back();
        _stack.push_back(stack_var::create((int64_t)data));
        break;
    };
    case BC_S2I: {
        uint16_t data = _stack.back().getAsUInt16();
        _stack.pop_back();
        _stack.push_back(stack_var::create((int64_t)data));
        break;
    };
    case BC_SWAP:
        std::swap(_stack.back(), _stack.at(_stack.size() - 2));
        break;
    case BC_POP:
        _stack.pop_back();
        break;
    case BC_LOADIVAR0:
    case BC_LOADDVAR0:
    case BC_LOADSVAR0:
        _stack.push_back(_context->getVarById(0));
        break;
    case BC_LOADIVAR1:
    case BC_LOADDVAR1:
    case BC_LOADSVAR1:
        _stack.push_back(_context->getVarById(1));
        break;
    case BC_LOADIVAR2:
    case BC_LOADDVAR2:
    case BC_LOADSVAR2:
        _stack.push_back(_context->getVarById(2));
        break;
    case BC_LOADIVAR3:
    case BC_LOADDVAR3:
    case BC_LOADSVAR3:
        _stack.push_back(_context->getVarById(3));
        break;

    case BC_LOADIVAR:
    case BC_LOADDVAR:
    case BC_LOADSVAR:
        _stack.push_back(_context->getVar());
        break;

    case BC_STOREIVAR0:
    case BC_STOREDVAR0:
    case BC_STORESVAR0:
        _context->storeVarById(_stack.back(), 0);
        _stack.pop_back();
        break;
    case BC_STOREIVAR1:
    case BC_STOREDVAR1:
    case BC_STORESVAR1:
        _context->storeVarById(_stack.back(), 1);
        _stack.pop_back();
        break;
    case BC_STOREIVAR2:
    case BC_STOREDVAR2:
    case BC_STORESVAR2:
        _context->storeVarById(_stack.back(), 2);
        _stack.pop_back();
        break;
    case BC_STOREIVAR3:
    case BC_STOREDVAR3:
    case BC_STORESVAR3:
        _context->storeVarById(_stack.back(), 3);
        _stack.pop_back();
        break;
    case BC_STOREIVAR:
    case BC_STOREDVAR:
    case BC_STORESVAR:
        _context->storeVar(_stack.back());
        _stack.pop_back();
        break;

    case BC_LOADCTXIVAR:
    case BC_LOADCTXDVAR:
    case BC_LOADCTXSVAR:
        _stack.push_back(_context->getContextVar());
        break;
    case BC_STORECTXIVAR:
    case BC_STORECTXDVAR:
    case BC_STORECTXSVAR:
        _context->storeContextVar(_stack.back());
        _stack.pop_back();
        break;
    case BC_ICMP: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.first.getAsInt() < data.second.getAsInt()) {
            _stack.push_back(stack_var::create((int64_t)-1));
        } else if (data.first.getAsInt() == data.second.getAsInt()) {
            _stack.push_back(stack_var::create((int64_t)0));
        } else {
            _stack.push_back(stack_var::create((int64_t)1));
        }
        break;
    };
    case BC_DCMP: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.first.getAsDouble() < data.second.getAsDouble()) {
            _stack.push_back(stack_var::create((int64_t)-1));
        } else if (data.first.getAsDouble() == data.second.getAsDouble()) {
            _stack.push_back(stack_var::create((int64_t)0));
        } else {
            _stack.push_back(stack_var::create((int64_t)1));
        }
        break;
    };
    case BC_JA:
        _context->jmp();
        break;
    case BC_IFICMPNE: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.first.getAsInt() != data.second.getAsInt()) {
            _context->jmp();
        } else {
            _context->getInt16();
        }
        break;
    };
    case BC_IFICMPE: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.first.getAsInt() == data.second.getAsInt()) {
            _context->jmp();
        } else {
            _context->getInt16();
        }
        break;
    };
    case BC_IFICMPG: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.first.getAsInt() > data.second.getAsInt()) {
            _context->jmp();
        } else {
            _context->getInt16();
        }
        break;
    };
    case BC_IFICMPGE: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.first.getAsInt() >= data.second.getAsInt()) {
            _context->jmp();
        } else {
            _context->getInt16();
        }
        break;
    };
    case BC_IFICMPL: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.first.getAsInt() < data.second.getAsInt()) {
            _context->jmp();
        } else {
            _context->getInt16();
        }
        break;
    };
    case BC_IFICMPLE: {
        std::pair<stack_var, stack_var> data(getOperandsForBinOp());
        if (data.first.getAsInt() <= data.second.getAsInt()) {
            _context->jmp();
        } else {
            _context->getInt16();
        }
        break;
    };
    case BC_STOP:
        return true;

    case BC_CALL: {
        uint16_t bf_id = _context->getUInt16();
        BytecodeFunction* bf = (BytecodeFunction*)functionById(bf_id);
        _context = new InterpreterScopeContext(bf, _context);
        break;
    };


    case BC_CALLNATIVE:
        throw InterpreterError("Native functions are not supported");

    case BC_RETURN: {
        InterpreterScopeContext* context_for_change = _context;
        _context = _context->getParent();
        delete context_for_change;
        break;
    };

    default:
        throw InterpreterError("Invalid instruction");
    }
    return false;
}

std::pair<stack_var, stack_var> InterpreterCode::getOperandsForBinOp() {

    stack_var lhs = _stack.back();
    _stack.pop_back();
    stack_var rhs = _stack.back();
    _stack.pop_back();
    return std::make_pair(lhs, rhs);
}
