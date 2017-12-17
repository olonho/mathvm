#pragma once

#include <stdint.h>
#include <unordered_map>
#include <memory>
#include <functional>
#include <string>
#include <stack>
#include <utility>
#include "mathvm.h"

namespace mathvm
{
    class StoreVaribale {
    public:
        VarType _type;
        union {
            double _doubleValue;
            int64_t _intValue;
        };
        template <class T>
        StoreVaribale(VarType vt, T val) {
            _type = vt;
            switch (_type) {
                case VT_DOUBLE:
                    _doubleValue = val;
                    break;
                case VT_INT:
                    _intValue = val;
                    break;
                default:
                    break;
            }
        }

        int64_t getInt() const
        {
            // assert(VT_INT == _type);
            return _intValue;
        }

        double getDouble() const
        {
            // assert(VT_DOUBLE == _type);
            return _doubleValue;
        }

        void setDouble(double val)
        {
            // assert(VT_DOUBLE == _type);
            this->_doubleValue = val;
        }

        void setInt(int64_t val)
        {
            // switch(_type) {
            //     case VT_DOUBLE: std::cout << "DOUBLE" << std::endl; break;
            //     case VT_INT: std::cout << "INT" << std::endl; break; 
            //     default: break;               
            // }
            // assert(VT_INT == _type);
            this->_intValue = val;
        }

        VarType getType() const
        {
            return _type;
        }

        virtual ~StoreVaribale() {}
    };

    class InterpreterCodeImpl : public Code
    {
    public:
        typedef std::vector<std::shared_ptr<Var>> vars;

        const uint16_t START_CONTEXT = 0xFFFF;
        const uint16_t INCORRECT_ID = 0xFFFF;

        InterpreterCodeImpl();

        virtual Status* execute(vector<Var*>& vars);

        virtual void disassemble(ostream& out = cout, FunctionFilter* filter = 0)
        {

        }

        void addContext(uint16_t context)
        {
            _parent[context] = _currentContext;
            _currentContext = context;
        }

        uint16_t addVar(std::shared_ptr<Var>& pVar)
        {
            uint16_t id = _vars[_currentContext].size();
            _vars[_currentContext].push_back(pVar);
            _context2nameid[_currentContext][pVar->name()] = id;
            return id;
        }

        std::pair<uint16_t, uint16_t> ContextAndIDByName(const std::string& name)
        {
            uint16_t context = _currentContext;
            do
            {
                auto it = _context2nameid[context].find(name);
                if (it != _context2nameid[context].end())
                {
                    return std::make_pair(context, it->second);
                }
            } while ((context = _parent[context]) != START_CONTEXT);
            return std::make_pair(INCORRECT_ID, INCORRECT_ID);
        }

        void push() {
            std::vector<StoreVaribale> out;
            for (const auto &i : _vars[_currentContext]) {
                switch (i->type()) {
                    case VT_DOUBLE: {
                        out.emplace_back(VT_DOUBLE, i->getDoubleValue());
                        break;
                    }
                    case VT_INT: {
                        out.emplace_back(VT_INT, i->getIntValue());
                        break;
                    }
                    default:
                        break;
                }
            }
            _prev_context[_currentContext].push(Storage(out));
        }

        void pop() {
            _prev_context[_currentContext].pop();
        }

        int64_t getLocalIntVar(uint16_t id) {
            return _prev_context[_currentContext].top().getInt(id);
        }

        void setLocalIntVar(uint16_t id, int64_t val)
        {
            return _prev_context[_currentContext].top().setInt(id, val);
        }

        double getLocalDoubleVar(uint16_t id) {
            return _prev_context[_currentContext].top().getDouble(id);
        }

        void setLocalDoubleVar(uint16_t id, double val)
        {
            _prev_context[_currentContext].top().setDouble(id, val);
        }

        int64_t getGlobalIntVar(uint16_t context, uint16_t id) {
            return _prev_context[context].top().getInt(id);
        }

        void setGlobalIntVar(uint16_t context, uint16_t id, int64_t val) {
            _prev_context[context].top().setInt(id, val);
        }

        double getGlobalDoubleVar(uint16_t context, uint16_t id) {
            return _prev_context[context].top().getDouble(id);
        }

        void setGlobalDoubleVar(uint16_t context, uint16_t id, double val) {
            _prev_context[context].top().setDouble(id, val);
        }

        void setCurrentContext(uint16_t context)
        {
            _currentContext = context;
        }

        uint16_t getCurrentContext()
        {
            return _currentContext;
        }

        Bytecode* getFunctionCode()
        {
            return static_cast<BytecodeFunction*>(functionById(_currentContext))->bytecode();
        }

        virtual ~InterpreterCodeImpl() {}
    private:
        void bc_dload();
        void bc_iload();
        void bc_sload();
        void bc_dload0();
        void bc_iload0();
        void bc_sload0();
        void bc_dload1();
        void bc_iload1();
        void bc_dloadm1();
        void bc_iloadm1();
        void bc_dadd();
        void bc_iadd();
        void bc_dsub();
        void bc_isub();
        void bc_dmul();
        void bc_imul();
        void bc_ddiv();
        void bc_idiv();
        void bc_imod();
        void bc_dneg();
        void bc_ineg();
        void bc_iaor();
        void bc_iaand();
        void bc_iaxor();
        void bc_iprint();
        void bc_dprint();
        void bc_sprint();
        void bc_i2d();
        void bc_d2i();
        void bc_s2i();
        void bc_swap();
        void bc_pop();
        void bc_loaddvar();
        void bc_loadivar();
        void bc_loadsvar();
        void bc_storedvar();
        void bc_storeivar();
        void bc_storesvar();
        void bc_loadctxdvar();
        void bc_loadctxivar();
        void bc_loadctxsvar();
        void bc_storectxivar();
        void bc_storectxdvar();
        void bc_storectxsvar();
        void bc_dcmp();
        void bc_icmp();
        void bc_ja();
        void bc_ificmpne();
        void bc_ificmpe();
        void bc_ificmpg();
        void bc_ificmpge();
        void bc_ificmpl();
        void bc_ificmple();
        void bc_call();
        void bc_return();
        void bc_invalid();
    private:
        typedef std::unordered_map<std::string, uint16_t> name2id;

        class Storage
        {
        private:
            std::vector<StoreVaribale> _var;
        public:
            Storage(std::vector<StoreVaribale> var) : _var(std::move(var)) {}

            double getDouble(int16_t id)
            {
                return _var[id].getDouble();
            }

            int64_t getInt(int16_t id)
            {
                return _var[id].getInt();
            }

            void setInt(int16_t id, int64_t val)
            {
                _var[id].setInt(val);
            }

            void setDouble(int16_t id, double val)
            {
                _var[id].setDouble(val);
            }

            virtual ~Storage() {}

        };
        uint16_t _currentContext;
        std::unordered_map<uint16_t, vars> _vars;
        std::unordered_map<uint16_t, uint16_t> _parent;
        std::unordered_map<uint16_t, name2id> _context2nameid;
        std::unordered_map<uint16_t, std::function<void()>> _map;
        std::stack<std::pair<size_t, Bytecode*>> _context;
        std::stack<StoreVaribale> _stack_var;
        uint64_t _instruction_pointer;
        Bytecode* _current_code;
        std::stack<uint16_t> _prev;
        std::unordered_map<uint16_t, std::stack<Storage>> _prev_context;

    };
}
