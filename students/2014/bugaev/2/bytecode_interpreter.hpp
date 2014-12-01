#ifndef BYTECODE_INTERPRETER_HPP
#define BYTECODE_INTERPRETER_HPP

#include "mathvm.h"
#include "exceptions.hpp"
#include "interpreter_code.hpp"

#include <vector>


namespace mathvm
{

class BytecodeInterpreter
{
public:
    BytecodeInterpreter(InterpreterCodeImpl *code);
    void interpret();

private:
    class Value
    {
    public:
        Value(): m_type(VT_INVALID), m_int(0) {}
        Value(int64_t value): m_type(VT_INT), m_int(value) {}
        Value(double value): m_type(VT_DOUBLE), m_double(value) {}
        Value(size_t value): m_type(VT_STRING), m_constId(value) {}
        Value(uint16_t value): m_type(VT_STRING), m_constId(value) {}

        VarType type() const
        {
            return m_type;
        }

        int64_t intValue() const
        {
            if (m_type != VT_INT)
                throw BytecodeException("Type mismatch");
            return m_int;
        }

        double doubleValue() const
        {
            if (m_type != VT_DOUBLE)
                throw BytecodeException("Type mismatch");
            return m_double;
        }

        uint16_t constId() const
        {
            if (m_type != VT_STRING)
                throw BytecodeException("Type mismatch");
            return m_constId;
        }

    private:
        VarType m_type;
        union {
            int64_t m_int;
            double m_double;
            size_t m_constId;
        };
    };

    class VariableStorage
    {
        typedef std::vector<Value> VarDict;

    public:
        VariableStorage(InterpreterCodeImpl *code):
            m_tmp(MIN_SIZE)
        {
            size_t funcCount = 0;
            Code::FunctionIterator fi(code);
            while (fi.hasNext()) {
                ++funcCount;
                fi.next();
            }
            m_storage.resize(funcCount + 1);
        }

        template<class T>
        void store(T value, uint16_t ctx, uint16_t id)
        {
            store(Value(value), ctx, id);
        }

        void store(Value const &value, uint16_t ctx, uint16_t id)
        {
            //cerr << "store " << ctx << " " << id << "\n";
            VarDict *dict = findDict(ctx);
            if (id >= dict->size())
                dict->resize(id + 1);
            (*dict)[id] = value;
        }

        Value const load(uint16_t ctx, uint16_t id)
        {
            //cerr << "load " << ctx << " " << id << "\n";
            VarDict *dict = findDict(ctx);
            if (id >= dict->size() || (*dict)[id].type() == VT_INVALID)
                throw BytecodeException("Undefined variable");
            return (*dict)[id];
        }

        VarDict *findDict(uint16_t ctx)
        {
            if (ctx >= m_storage.size())
                throw BytecodeException("Illegal context");

            if (ctx == 0)
                return &m_tmp;
            else
                return &m_storage[ctx].back();
            return 0;
        }

        void push(uint16_t fid)
        {
            //cerr << "push " << fid << "\n";
            assert(fid < m_storage.size());
            m_storage[fid].push_back(m_tmp);
            m_fids.push_back(fid);
        }

        void pop()
        {
            //cerr << "pop " << m_fids.back() << "\n";
            m_storage[m_fids.back()].pop_back();
            m_fids.pop_back();
        }

    private:
        static size_t const MIN_SIZE = 4;

        VarDict m_tmp;
        std::vector< std::vector< VarDict > > m_storage;
        std::vector<uint16_t> m_fids;
    };

    void moveBci(bool ignoring = true);
    void writeValue(std::ostream &out, Value const &value, VarType type);

    Bytecode *bc()
    {
        return m_funcs.back()->bytecode();
    }

    void pushFunc(uint16_t id)
    {
        BytecodeFunction *bf = dynamic_cast<BytecodeFunction *>(
                m_code->functionById(id));
        //cerr << "push func " << id << " " << bf << "\n";
        assert(bf);
        m_funcs.push_back(bf);
    }

    void pushFunc(std::string const &name)
    {
        BytecodeFunction *bf = dynamic_cast<BytecodeFunction *>(
                m_code->functionByName(name));
        assert(bf);
        m_funcs.push_back(bf);
    }

    void popFunc()
    {
        m_funcs.pop_back();
    }

    size_t &bci()
    {
        return m_bcis.back();
    }

    void pushBci(size_t i = 0)
    {
        m_bcis.push_back(i);
    }

    void popBci()
    {
        m_bcis.pop_back();
    }

    template<class T>
    void pushValue(T value)
    {
        m_stack.push_back(Value(value));
    }

    void pushValue(Value const &value)
    {
        m_stack.push_back(value);
    }

    Value const popValue()
    {
        if (m_stack.empty())
            throw BytecodeException("Incorrect bytecode");

        Value const value(m_stack.back());
        m_stack.pop_back();
        return value;
    }

private:
    InterpreterCodeImpl *m_code;

    VariableStorage m_locals;
    std::vector<Value> m_stack;
    std::vector<BytecodeFunction *> m_funcs;
    std::vector<size_t> m_bcis;
};

}


#endif // BYTECODE_INTERPRETER_HPP
