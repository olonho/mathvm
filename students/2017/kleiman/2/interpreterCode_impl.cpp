#include "context.h"
#include <vector>
#include <stack>
#include <iostream>
#include <cmath>

namespace mathvm
{
    InterpreterCodeImpl::InterpreterCodeImpl()
            : _currentContext(START_CONTEXT)
    {
        _map[BC_INVALID] = std::bind(&InterpreterCodeImpl::bc_invalid, this);
        _map[BC_DLOAD] = std::bind(&InterpreterCodeImpl::bc_dload, this);
        _map[BC_ILOAD] = std::bind(&InterpreterCodeImpl::bc_iload, this);
        _map[BC_SLOAD] = std::bind(&InterpreterCodeImpl::bc_sload, this);
        _map[BC_DLOAD0] = std::bind(&InterpreterCodeImpl::bc_dload0, this);
        _map[BC_ILOAD0] = std::bind(&InterpreterCodeImpl::bc_iload0, this);
        _map[BC_SLOAD0] = std::bind(&InterpreterCodeImpl::bc_sload0, this);
        _map[BC_DLOAD1] = std::bind(&InterpreterCodeImpl::bc_dload1, this);
        _map[BC_ILOAD1] = std::bind(&InterpreterCodeImpl::bc_iload1, this);
        _map[BC_DLOADM1] = std::bind(&InterpreterCodeImpl::bc_dloadm1, this);
        _map[BC_ILOADM1] = std::bind(&InterpreterCodeImpl::bc_iloadm1, this);
        _map[BC_DADD] = std::bind(&InterpreterCodeImpl::bc_dadd, this);
        _map[BC_IADD] = std::bind(&InterpreterCodeImpl::bc_iadd, this);
        _map[BC_DSUB] = std::bind(&InterpreterCodeImpl::bc_dsub, this);
        _map[BC_ISUB] = std::bind(&InterpreterCodeImpl::bc_isub, this);
        _map[BC_DMUL] = std::bind(&InterpreterCodeImpl::bc_dmul, this);
        _map[BC_IMUL] = std::bind(&InterpreterCodeImpl::bc_imul, this);
        _map[BC_DDIV] = std::bind(&InterpreterCodeImpl::bc_ddiv, this);
        _map[BC_IDIV] = std::bind(&InterpreterCodeImpl::bc_idiv, this);
        _map[BC_IMOD] = std::bind(&InterpreterCodeImpl::bc_imod, this);
        _map[BC_DNEG] = std::bind(&InterpreterCodeImpl::bc_dneg, this);
        _map[BC_INEG] = std::bind(&InterpreterCodeImpl::bc_ineg, this);
        _map[BC_IAOR] = std::bind(&InterpreterCodeImpl::bc_iaor, this);
        _map[BC_IAAND] = std::bind(&InterpreterCodeImpl::bc_iaand, this);
        _map[BC_IAXOR] = std::bind(&InterpreterCodeImpl::bc_iaxor, this);
        _map[BC_IPRINT] = std::bind(&InterpreterCodeImpl::bc_iprint, this);
        _map[BC_DPRINT] = std::bind(&InterpreterCodeImpl::bc_dprint, this);
        _map[BC_SPRINT] = std::bind(&InterpreterCodeImpl::bc_sprint, this);
        _map[BC_I2D] = std::bind(&InterpreterCodeImpl::bc_i2d, this);
        _map[BC_D2I] = std::bind(&InterpreterCodeImpl::bc_d2i, this);
        _map[BC_S2I] = std::bind(&InterpreterCodeImpl::bc_s2i, this);
        _map[BC_SWAP] = std::bind(&InterpreterCodeImpl::bc_swap, this);
        _map[BC_POP] = std::bind(&InterpreterCodeImpl::bc_pop, this);
        _map[BC_LOADDVAR] = std::bind(&InterpreterCodeImpl::bc_loaddvar, this);
        _map[BC_LOADIVAR] = std::bind(&InterpreterCodeImpl::bc_loadivar, this);
        _map[BC_LOADSVAR] = std::bind(&InterpreterCodeImpl::bc_loadsvar, this);
        _map[BC_STOREDVAR] = std::bind(&InterpreterCodeImpl::bc_storedvar, this);
        _map[BC_STOREIVAR] = std::bind(&InterpreterCodeImpl::bc_storeivar, this);
        _map[BC_STORESVAR] = std::bind(&InterpreterCodeImpl::bc_storesvar, this);
        _map[BC_LOADCTXDVAR] = std::bind(&InterpreterCodeImpl::bc_loadctxdvar, this);
        _map[BC_LOADCTXIVAR] = std::bind(&InterpreterCodeImpl::bc_loadctxivar, this);
        _map[BC_LOADCTXSVAR] = std::bind(&InterpreterCodeImpl::bc_loadctxsvar, this);
        _map[BC_STORECTXDVAR] = std::bind(&InterpreterCodeImpl::bc_storectxdvar, this);
        _map[BC_STORECTXIVAR] = std::bind(&InterpreterCodeImpl::bc_storectxivar, this);
        _map[BC_STORECTXSVAR] = std::bind(&InterpreterCodeImpl::bc_storectxsvar, this);
        _map[BC_DCMP] = std::bind(&InterpreterCodeImpl::bc_dcmp, this);
        _map[BC_ICMP] = std::bind(&InterpreterCodeImpl::bc_icmp, this);
        _map[BC_JA] = std::bind(&InterpreterCodeImpl::bc_ja, this);
        _map[BC_IFICMPNE] = std::bind(&InterpreterCodeImpl::bc_ificmpne, this);
        _map[BC_IFICMPE] = std::bind(&InterpreterCodeImpl::bc_ificmpe, this);
        _map[BC_IFICMPG] = std::bind(&InterpreterCodeImpl::bc_ificmpg, this);
        _map[BC_IFICMPGE] = std::bind(&InterpreterCodeImpl::bc_ificmpge, this);
        _map[BC_IFICMPL] = std::bind(&InterpreterCodeImpl::bc_ificmpl, this);
        _map[BC_IFICMPLE] = std::bind(&InterpreterCodeImpl::bc_ificmple, this);
        _map[BC_CALL] = std::bind(&InterpreterCodeImpl::bc_call, this);
        _map[BC_RETURN] = std::bind(&InterpreterCodeImpl::bc_return, this);
    }

    void InterpreterCodeImpl::bc_dload()
    {
        double val = _current_code->getDouble(_instruction_pointer);
        _stack_var.push(StoreVaribale(VT_DOUBLE, val));
        _instruction_pointer += sizeof(val);
    }

    void InterpreterCodeImpl::bc_iload()
    {
        auto val = _current_code->getInt64(_instruction_pointer);
        _stack_var.push(StoreVaribale(VT_INT, val));
        _instruction_pointer += sizeof(val);
    }

    void InterpreterCodeImpl::bc_sload()
    {
        auto val = _current_code->getUInt16(_instruction_pointer);
        _stack_var.push(StoreVaribale(VT_INT, val));
        _instruction_pointer += sizeof(val);
    }

    void InterpreterCodeImpl::bc_dload0()
    {
        _stack_var.push(StoreVaribale(VT_DOUBLE, 0.0));
    }

    void InterpreterCodeImpl::bc_iload0()
    {
        _stack_var.push(StoreVaribale(VT_INT, 0));
    }

    void InterpreterCodeImpl::bc_sload0()
    {
        _stack_var.push(StoreVaribale(VT_INT, 0));
    }

    void InterpreterCodeImpl::bc_dload1()
    {
        _stack_var.push(StoreVaribale(VT_DOUBLE, 1.0));
    }

    void InterpreterCodeImpl::bc_iload1()
    {
        _stack_var.push(StoreVaribale(VT_INT, 1));
    }

    void InterpreterCodeImpl::bc_dloadm1()
    {
        _stack_var.push(StoreVaribale(VT_DOUBLE, -1.0));
    }

    void InterpreterCodeImpl::bc_iloadm1()
    {
        _stack_var.push(StoreVaribale(VT_INT, -1));
    }

    void InterpreterCodeImpl::bc_dadd()
    {
        double first = _stack_var.top().getDouble();
        _stack_var.pop();
        double second = _stack_var.top().getDouble();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_DOUBLE, first + second));
    }

    void InterpreterCodeImpl::bc_iadd()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        int64_t second = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, first + second));
    }

    void InterpreterCodeImpl::bc_dsub()
    {
        double first = _stack_var.top().getDouble();
        _stack_var.pop();
        double second = _stack_var.top().getDouble();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_DOUBLE, second - first));

    }

    void InterpreterCodeImpl::bc_isub()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        int64_t second = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, second - first));
    }

    void InterpreterCodeImpl::bc_dmul()
    {
        double first = _stack_var.top().getDouble();
        _stack_var.pop();
        double second = _stack_var.top().getDouble();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_DOUBLE, second * first));
    }

    void InterpreterCodeImpl::bc_imul()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        int64_t second = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, second * first));
    }

    void InterpreterCodeImpl::bc_ddiv()
    {
        double first = _stack_var.top().getDouble();
        _stack_var.pop();
        double second = _stack_var.top().getDouble();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_DOUBLE, second / first));
    }

    void InterpreterCodeImpl::bc_idiv()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        int64_t second = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, second / first));
    }

    void InterpreterCodeImpl::bc_imod()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        int64_t second = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, second % first));
    }

    void InterpreterCodeImpl::bc_dneg()
    {
        double first = _stack_var.top().getDouble();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_DOUBLE, -first));
    }

    void InterpreterCodeImpl::bc_ineg()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, -first));
    }

    void InterpreterCodeImpl::bc_iaor()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        int64_t second = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, second | first));
    }

    void InterpreterCodeImpl::bc_iaand()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        int64_t second = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, second & first));
    }

    void InterpreterCodeImpl::bc_iaxor()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        int64_t second = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, second ^ first));
    }

    void InterpreterCodeImpl::bc_iprint()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        std::cout << first;
    }

    void InterpreterCodeImpl::bc_dprint()
    {
        double first = _stack_var.top().getDouble();
        _stack_var.pop();
        std::cout << first;
    }

    void InterpreterCodeImpl::bc_sprint()
    {
        auto first = static_cast<uint16_t>(_stack_var.top().getInt());
        _stack_var.pop();
        std::cout << constantById(first);
    }

    void InterpreterCodeImpl::bc_i2d()
    {
        int64_t first = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_DOUBLE, static_cast<double>(first)));
    }

    void InterpreterCodeImpl::bc_d2i()
    {
        double first = _stack_var.top().getDouble();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, static_cast<int64_t >(first)));
    }

    void InterpreterCodeImpl::bc_s2i()
    {
        auto first = static_cast<uint16_t>(_stack_var.top().getInt());
        _stack_var.pop();
        auto str = constantById(first);
        std::string::size_type sz;
        int64_t val = std::stoll(str,&sz);
        _stack_var.push(StoreVaribale(VT_INT, val));
    }

    void InterpreterCodeImpl::bc_swap()
    {
        auto first = _stack_var.top();
        _stack_var.pop();
        auto second = _stack_var.top();
        _stack_var.pop();
        _stack_var.push(first);
        _stack_var.push(second);

    }

    void InterpreterCodeImpl::bc_pop()
    {
        _stack_var.pop();
    }

    void InterpreterCodeImpl::bc_loaddvar()
    {
        uint16_t id = _current_code->getUInt16(_instruction_pointer);
        auto var = getLocalDoubleVar(id);
        _stack_var.push(StoreVaribale(VT_DOUBLE, var));
        _instruction_pointer += sizeof(id);
    }

    void InterpreterCodeImpl::bc_loadivar()
    {
        uint16_t id = _current_code->getUInt16(_instruction_pointer);
        auto var = getLocalIntVar(id);
        _stack_var.push(StoreVaribale(VT_INT, var));
        _instruction_pointer += sizeof(id);
    }

    void InterpreterCodeImpl::bc_loadsvar()
    {
        uint16_t id = _current_code->getUInt16(_instruction_pointer);
        _stack_var.push(StoreVaribale(VT_INT, id));
        _instruction_pointer += sizeof(id);
    }

    void InterpreterCodeImpl::bc_storedvar()
    {
        uint16_t id = _current_code->getUInt16(_instruction_pointer);
        auto TOS_var = _stack_var.top().getDouble();
        _stack_var.pop();
        setLocalDoubleVar(id, TOS_var);
        _instruction_pointer += sizeof(id);
    }

    void InterpreterCodeImpl::bc_storeivar()
    {
        uint16_t id = _current_code->getUInt16(_instruction_pointer);
        auto TOS_var = _stack_var.top().getInt();
        _stack_var.pop();
        setLocalIntVar(id, TOS_var);
        _instruction_pointer += sizeof(id);
    }

    void InterpreterCodeImpl::bc_storesvar()
    {
        uint16_t id = _current_code->getUInt16(_instruction_pointer);
        auto TOS_var = _stack_var.top().getInt();
        _stack_var.pop();
        setLocalIntVar(id, TOS_var);
        _instruction_pointer += sizeof(id);
    }

    void InterpreterCodeImpl::bc_loadctxdvar()
    {
        auto context = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(context);
        auto id = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(id);
        auto var = getGlobalDoubleVar(context, id);
        _stack_var.push(StoreVaribale(VT_DOUBLE, var));
    }

    void InterpreterCodeImpl::bc_loadctxivar()
    {
        auto context = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(context);
        auto id = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(id);
        auto var = getGlobalIntVar(context, id);
        _stack_var.push(StoreVaribale(VT_INT, var));
    }

    void InterpreterCodeImpl::bc_loadctxsvar()
    {
        auto context = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(context);
        auto id = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(id);
        auto var = getGlobalIntVar(context, id);
        _stack_var.push(StoreVaribale(VT_INT, var));
    }

    void InterpreterCodeImpl::bc_storectxivar()
    {
        auto context = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(context);
        auto id = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(id);
        auto var = _stack_var.top().getInt();
        _stack_var.pop();
        setGlobalIntVar(context, id, var);
    }

    void InterpreterCodeImpl::bc_storectxdvar()
    {
        auto context = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(context);
        auto id = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(id);
        auto var = _stack_var.top().getDouble();
        _stack_var.pop();
        setGlobalDoubleVar(context, id, var);
    }

    void InterpreterCodeImpl::bc_storectxsvar()
    {
        auto context = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(context);
        auto id = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(id);
        auto var = _stack_var.top().getInt();
        _stack_var.pop();
        setGlobalIntVar(context, id, var);
    }

    void InterpreterCodeImpl::bc_dcmp()
    {
        auto upper = _stack_var.top().getDouble();
        _stack_var.pop();
        auto lower = _stack_var.top().getDouble();
        _stack_var.pop();
        int64_t result = fabs(lower - upper) < 0.000000001 ? 0 : (lower - upper < 0 ? -1 : 1);
        _stack_var.push(StoreVaribale(VT_DOUBLE, lower));
        _stack_var.push(StoreVaribale(VT_DOUBLE, upper));
        _stack_var.push(StoreVaribale(VT_INT, result));
    }

    void InterpreterCodeImpl::bc_icmp()
    {
        auto upper = _stack_var.top().getInt();
        _stack_var.pop();
        auto lower = _stack_var.top().getInt();
        _stack_var.pop();
        int64_t result = lower - upper;
        result /= abs(result) ? abs(result) : 1;
        _stack_var.push(StoreVaribale(VT_INT, lower));
        _stack_var.push(StoreVaribale(VT_INT, upper));
        _stack_var.push(StoreVaribale(VT_INT, result));
    }

    void InterpreterCodeImpl::bc_ja()
    {
        int16_t offset = _current_code->getInt16(_instruction_pointer);
        _instruction_pointer += offset;
    }

    void InterpreterCodeImpl::bc_ificmpne()
    {
        auto upper = _stack_var.top().getInt();
        _stack_var.pop();
        auto lower = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, lower));
        _stack_var.push(StoreVaribale(VT_INT, upper));
        _instruction_pointer += lower != upper ? static_cast<size_t>(_current_code->getInt16(_instruction_pointer)) : sizeof(uint16_t);
    }

    void InterpreterCodeImpl::bc_ificmpe()
    {
        auto upper = _stack_var.top().getInt();
        _stack_var.pop();
        auto lower = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, lower));
        _stack_var.push(StoreVaribale(VT_INT, upper));
        _instruction_pointer += lower == upper ? static_cast<size_t>(_current_code->getInt16(_instruction_pointer)) : sizeof(uint16_t);
    }

    void InterpreterCodeImpl::bc_ificmpg()
    {
        auto upper = _stack_var.top().getInt();
        _stack_var.pop();
        auto lower = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, lower));
        _stack_var.push(StoreVaribale(VT_INT, upper));
        _instruction_pointer += lower > upper ? static_cast<size_t>(_current_code->getInt16(_instruction_pointer)) : sizeof(uint16_t);
    }

    void InterpreterCodeImpl::bc_ificmpge()
    {
        auto upper = _stack_var.top().getInt();
        _stack_var.pop();
        auto lower = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, lower));
        _stack_var.push(StoreVaribale(VT_INT, upper));
        _instruction_pointer += lower >= upper ? static_cast<size_t>(_current_code->getInt16(_instruction_pointer)) : sizeof(uint16_t);
    }

    void InterpreterCodeImpl::bc_ificmpl()
    {
        auto upper = _stack_var.top().getInt();
        _stack_var.pop();
        auto lower = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, lower));
        _stack_var.push(StoreVaribale(VT_INT, upper));
        _instruction_pointer += lower < upper ? static_cast<size_t>(_current_code->getInt16(_instruction_pointer)) : sizeof(uint16_t);
    }

    void InterpreterCodeImpl::bc_ificmple()
    {
        auto upper = _stack_var.top().getInt();
        _stack_var.pop();
        auto lower = _stack_var.top().getInt();
        _stack_var.pop();
        _stack_var.push(StoreVaribale(VT_INT, lower));
        _stack_var.push(StoreVaribale(VT_INT, upper));
        _instruction_pointer += lower <= upper ? static_cast<size_t>(_current_code->getInt16(_instruction_pointer)) : sizeof(uint16_t);
    }

    void InterpreterCodeImpl::bc_call()
    {
        uint16_t func = _current_code->getUInt16(_instruction_pointer);
        _instruction_pointer += sizeof(uint16_t);
        _context.push(std::make_pair(_instruction_pointer, _current_code));
        _instruction_pointer = 0;
        _prev.push(getCurrentContext());
        setCurrentContext(func);
        push();
        _current_code = getFunctionCode();
    }

    void InterpreterCodeImpl::bc_return()
    {
        pop();
        setCurrentContext(_prev.top());
        _prev.pop();
        _instruction_pointer = static_cast<uint16_t>(_current_code->length());
    }

    void InterpreterCodeImpl::bc_invalid()
    {
        throw std::logic_error("unexpected instruction");
    }

    Status* InterpreterCodeImpl::execute(std::vector<Var*> & vars)
    {
        _prev_context.reserve(10000000);
        setCurrentContext(0);
        _context.push(std::make_pair(0, getFunctionCode()));
        push();
        while (!_context.empty()) {
            auto p = _context.top();
            _context.pop();
            _current_code = p.second;
            _instruction_pointer = static_cast<uint16_t>(p.first);
            while (_instruction_pointer < _current_code->length()) {
                Instruction insn = _current_code->getInsn(_instruction_pointer);
                ++_instruction_pointer;
                _map[insn]();
            }
        }
        return Status::Ok();
    }

}