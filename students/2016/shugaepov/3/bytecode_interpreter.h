#ifndef MATHVM_BYTECODE_INTERPRETER_H
#define MATHVM_BYTECODE_INTERPRETER_H

#include "../../../../include/mathvm.h"

#include <deque>
#include <stack>

namespace mathvm
{
    using namespace std;

    class stack_value
    {
        union
        {
            int64_t i;
            double d;
            int16_t id;
        } _value;

        VarType _v_type;

    public:

        stack_value()
                : _v_type(VT_INVALID)
        {}

        stack_value(int64_t i)
                : _v_type(VT_INT)
        {
            _value.i = i;
        }

        stack_value(double d)
                : _v_type(VT_DOUBLE)
        {
            _value.d = d;
        }

        stack_value(uint16_t id)
                : _v_type(VT_STRING)
        {
            _value.id = id;
        }

        bool check(VarType type) const
        {
            return !(_v_type == VT_INVALID || _v_type != type);
        }

        int64_t get_int() const
        {
            if (!check(VT_INT))
                throw ;
            return _value.i;
        }

        double get_double() const
        {
            if (!check(VT_DOUBLE))
                throw ;
            return _value.d;
        }

        uint16_t get_id() const
        {
            if (!check(VT_STRING))
                throw ;
            return _value.id;
        }
    };

    class interpreter_context
    {
    public:
        BytecodeFunction* _bf;
        vector<stack_value> _vars;
        uint32_t _position;

        bool valid_bounds(uint16_t id) const { return id < _vars.size(); }

        interpreter_context(BytecodeFunction* bf)
                : _bf(bf)
                , _position(0)
        {
            _vars.resize(bf->localsNumber());
        }

        ~interpreter_context()
        {}

        Instruction next()
        {
            return _bf->bytecode()->getInsn(_position++);
        }

        bool has_next()
        {
            return _position < _bf->bytecode()->length();
        }

        stack_value var_by_id(int id) { return _vars[id]; }

        uint16_t id() {
            uint16_t res = _bf->bytecode()->getUInt16(_position);
            _position += sizeof(uint16_t);
            return res;
        }

        int64_t int64()
        {
            int64_t res = _bf->bytecode()->getInt64(_position);
            _position += sizeof(int64_t);
            return res;
        }

        int16_t int16()
        {
            int16_t res = _bf->bytecode()->getInt16(_position);
            _position += sizeof(int16_t);
            return res;
        }

        double real()
        {
            double res = _bf->bytecode()->getDouble(_position);
            _position += sizeof(double);
            return res;
        }

        stack_value var() { return var_by_id(id()); }

        void store(stack_value val)
        {
            store_by_id(val, id());
        }

        void store_by_id(stack_value val, uint16_t id)
        {
            if (!valid_bounds(id))
                throw;
            _vars[id] = val;
        }

        void jump(bool condition)
        {
            int16_t offset = int16();
            if (condition)
                _position += offset - sizeof(int16_t);
        }
    };


    class bytecode_interpreter : public Code
    {
        deque<interpreter_context> _ctx;
        stack<stack_value>  _stack;

    public:

        bytecode_interpreter()
        {}

        ~bytecode_interpreter()
        {}

        Status* execute(vector<Var*>& vars) override;

    private:

        void execute(Instruction insn);

        stack_value load_context_var(uint16_t ctx_id, uint16_t var_id)
        {
            for (int i = 0; i < (int)_ctx.size(); i++)
            {
                if (_ctx[i]._bf->scopeId() == ctx_id)
                    return _ctx[i]._vars[var_id];
            }
            throw ;
        }

        stack_value load_context_var()
        {
            if (_ctx.size() == 0)
                throw ;

            uint16_t ctx_id = _ctx.front().id();
            uint16_t var_id = _ctx.front().id();
            return load_context_var(ctx_id, var_id);
        }

        void store_context_var(stack_value val, uint16_t ctx_id, uint16_t var_id)
        {
            for (int i = 0; i < (int)_ctx.size(); i++)
            {
                if (_ctx[i]._bf->scopeId() == ctx_id)
                {
                    _ctx[i]._vars[var_id] = val;
                    return;
                }
            }
            throw ;
        }

        void store_context_var(stack_value val)
        {
            if (_ctx.size() == 0)
                throw ;

            uint16_t ctx_id = _ctx.front().id();
            uint16_t var_id = _ctx.front().id();
            store_context_var(val, ctx_id, var_id);
        }

        stack_value load_tos()
        {
            stack_value val = _stack.top();
            _stack.pop();
            return val;
        }

        void swap_tos()
        {
            stack_value top = _stack.top();
            _stack.pop();
            swap(top, _stack.top());
            _stack.push(top);
        }
    };
} // mathvm

#endif //MATHVM_BYTECODE_INTERPRETER_H
