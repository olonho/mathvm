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
    public:
        VariableStorage()
        {
        }

        template<class T>
        void store(T value, uint16_t ctx, uint16_t id)
        {
            store(Value(value), ctx, id);
        }

        void store(Value const &value, uint16_t ctx, uint16_t id)
        {
            //cerr << "store " << ctx << " " << id << "\n";
            (*findDict(ctx))[id] = value;
        }

        Value const load(uint16_t ctx, uint16_t id)
        {
            //cerr << "load " << ctx << " " << id << "\n";
            std::map<uint16_t, Value> *dict = findDict(ctx);
            std::map<uint16_t, Value>::iterator const it(dict->find(id));
            if (it == dict->end()) {
                //for (int i = 0; i < (int) m_storage.size(); ++i) {
                //    cerr << "func " << i << "\n";
                //    for (int j = 0; j < (int) m_storage[i].size(); ++j) {
                //        cerr << "\tlvl " << j << "\n";
                //        for (std::map<uint16_t, Value>::iterator it = m_storage[i][j].begin(); it != m_storage[i][j].end(); ++it) {
                //            cerr << "\t\tvar " << it->first << " = " << it->second.intValue() << "\n";
                //        }
                //    }
                //}
                throw BytecodeException("Undefined variable");
            }
            return it->second;
        }

        std::map<uint16_t, Value> *findDict(uint16_t ctx)
        {
            if (ctx == 0)
                return &m_tmp;
            else
                return &m_storage[ctx].back();
            return 0;
        }

        void push(uint16_t fid)
        {
            //cerr << "push " << fid << "\n";
            m_storage[fid + 1].push_back(m_tmp);
            m_fids.push_back(fid);
        }

        void pop()
        {
            //cerr << "pop\n";
            m_tmp = m_storage[m_fids.back() + 1].back();
            m_storage[m_fids.back() + 1].pop_back();
            m_fids.pop_back();
        }

    private:
        std::map<uint16_t, Value> m_tmp;
        std::map< uint16_t,
                  std::vector< std::map<uint16_t, Value> > > m_storage;
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
