#pragma once

#include <stack>
#include <memory>
#include "mathvm.h"
#include "context.h"
#include "utils.h"

namespace mathvm {
    class InterpreterCodeImpl : public Code {
    public:
        InterpreterCodeImpl(ostream& os)
                : os(os)
        {}

    private:
        ostream& os;
        friend class BytecodeVisitor;

        std::unique_ptr<Context> context;
        stack<uint32_t> insnPtrs;
        stack<BytecodeFunction*> funcs;

        uint32_t iptr;
        details::StackMem stackMem;

        template <class T>
        int64_t compare(T val1, T val2) {
            return (val1 > val2) ? 1 : (val1 == val2) ? 0 : -1;
        }

        void execute(BytecodeFunction* function) {
            iptr = 0;

            const Bytecode* bc = function->bytecode();

            while (iptr < bc->length()) {
                Instruction curInsn = bc->getInsn(iptr);
                iptr++;
                switch (curInsn) {
                    case BC_DLOAD:
                        stackMem.push(bc->getDouble(iptr));
                        iptr += sizeof(double);
                        break;
                    case BC_ILOAD:
                        stackMem.push(bc->getInt64(iptr));
                        iptr += sizeof(int64_t);
                        break;
                    case BC_SLOAD:
                        stackMem.push(bc->getUInt16(iptr));
                        iptr += sizeof(uint16_t);
                        break;
                    case BC_DLOAD0:
                        stackMem.push(0.);
                        break;
                    case BC_ILOAD0:
                        stackMem.push<int64_t>(0);
                        break;
                    case BC_SLOAD0:
                        stackMem.push<uint16_t>(0);
                        break;
                    case BC_DLOAD1:
                        stackMem.push(1.);
                        break;
                    case BC_ILOAD1:
                        stackMem.push<int64_t>(1);
                        break;
                    case BC_DLOADM1:
                        stackMem.push(-1.);
                        break;
                    case BC_ILOADM1:
                        stackMem.push<int64_t>(-1);
                        break;
                    case BC_DADD: {
                        double val1 = stackMem.getDouble();
                        double val2 = stackMem.getDouble();
                        stackMem.push(val1 + val2);
                        break;
                    }
                    case BC_IADD: {
                        int64_t val1 = stackMem.getInt();
                        int64_t val2 = stackMem.getInt();
                        stackMem.push(val1 + val2);
                        break;
                    }
                    case BC_DSUB: {
                        double val1 = stackMem.getDouble();
                        double val2 = stackMem.getDouble();
                        stackMem.push(val2 - val1);
                        break;
                    }
                    case BC_ISUB: {
                        int64_t val1 = stackMem.getInt();
                        int64_t val2 = stackMem.getInt();
                        stackMem.push(val2 - val1);
                        break;
                    }
                    case BC_DMUL: {
                        double val1 = stackMem.getDouble();
                        double val2 = stackMem.getDouble();
                        stackMem.push(val1 * val2);
                        break;
                    }
                    case BC_IMUL: {
                        int64_t val1 = stackMem.getInt();
                        int64_t val2 = stackMem.getInt();
                        stackMem.push(val1 * val2);
                        break;
                    }
                    case BC_DDIV: {
                        double val1 = stackMem.getDouble();
                        double val2 = stackMem.getDouble();
                        stackMem.push(val2 / val1);
                        break;
                    }
                    case BC_IDIV: {
                        int64_t val1 = stackMem.getInt();
                        int64_t val2 = stackMem.getInt();
                        stackMem.push(val2 / val1);
                        break;
                    }
                    case BC_IMOD: {
                        int64_t val1 = stackMem.getInt();
                        int64_t val2 = stackMem.getInt();
                        stackMem.push(val2 % val1);
                        break;
                    }
                    case BC_DNEG: {
                        double value = stackMem.getDouble();
                        stackMem.push(-value);
                        break;
                    }
                    case BC_INEG: {
                        int64_t value = stackMem.getInt();
                        stackMem.push(-value);
                        break;
                    }
                    case BC_IAOR: {
                        int64_t val1 = stackMem.getInt();
                        int64_t val2 = stackMem.getInt();
                        stackMem.push(val1 | val2);
                        break;
                    }
                    case BC_IAAND: {
                        int64_t val1 = stackMem.getInt();
                        int64_t val2 = stackMem.getInt();
                        stackMem.push(val1 & val2);
                        break;
                    }
                    case BC_IAXOR: {
                        int64_t val1 = stackMem.getInt();
                        int64_t val2 = stackMem.getInt();
                        stackMem.push(val1 ^ val2);
                        break;
                    }
                    case BC_IPRINT:
                        os << stackMem.getInt();
                        break;
                    case BC_DPRINT:
                        os << stackMem.getDouble();
                        break;
                    case BC_SPRINT:
                        os << constantById(stackMem.getUInt16());
                        break;
                    case BC_I2D:
                        stackMem.push(static_cast<double>(stackMem.getInt()));
                        break;
                    case BC_D2I:
                        stackMem.push(static_cast<int64_t>(stackMem.getDouble()));
                        break;
                    case BC_S2I:
                        stackMem.push(static_cast<int64_t>(stackMem.getUInt16()));
                        break;
                    case BC_SWAP:
                        stackMem.swap();
                        break;
                    case BC_POP:
                        stackMem.pop();
                        break;
                    case BC_LOADCTXDVAR: {
                        uint16_t scopeId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);
                        uint16_t varId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);

                        details::StackVar &var = context->getVar(scopeId, varId);
                        stackMem.push(var.asDouble());
                        break;
                    }
                    case BC_LOADCTXIVAR: {
                        uint16_t scopeId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);
                        uint16_t varId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);

                        details::StackVar &var = context->getVar(scopeId, varId);
                        stackMem.push(var.asInt());
                        break;
                    }
                    case BC_LOADCTXSVAR: {
                        uint16_t scopeId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);
                        uint16_t varId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);

                        details::StackVar &var = context->getVar(scopeId, varId);
                        stackMem.push(var.asUInt16());
                        break;
                    }
                    case BC_STORECTXDVAR: {
                        uint16_t scopeId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);
                        uint16_t varId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);

                        context->setVarDouble(scopeId, varId, stackMem.getDouble());
                        break;
                    }
                    case BC_STORECTXIVAR: {
                        uint16_t scopeId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);
                        uint16_t varId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);

                        context->setVarInt(scopeId, varId, stackMem.getInt());
                        break;
                    }
                    case BC_STORECTXSVAR: {
                        uint16_t scopeId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);
                        uint16_t varId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);

                        context->setVarString(scopeId, varId, stackMem.getUInt16());
                        break;
                    }
                    case BC_DCMP: {
                        double val1 = stackMem.getDouble();
                        double val2 = stackMem.getDouble();
                        stackMem.push(compare(val2, val1));
                        break;
                    }
                    case BC_ICMP: {
                        int64_t val1 = stackMem.getInt();
                        int64_t val2 = stackMem.getInt();
                        stackMem.push(compare(val2, val1));
                        break;
                    }
                    case BC_JA:
                        iptr += bc->getInt16(iptr);
                        break;
                    case BC_IFICMPNE: {
                        int64_t value1 = stackMem.getInt();
                        int64_t value2 = stackMem.getInt();
                        int16_t offset = bc->getInt16(iptr);
                        if (value1 != value2) {
                            iptr += offset;
                        } else {
                            iptr += sizeof(int16_t);
                        }
                        break;
                    }
                    case BC_IFICMPE: {
                        int64_t value1 = stackMem.getInt();
                        int64_t value2 = stackMem.getInt();
                        int16_t offset = bc->getInt16(iptr);
                        if (value1 == value2) {
                            iptr += offset;
                        } else {
                            iptr += sizeof(int16_t);
                        }
                        break;
                    }
                    case BC_IFICMPG: {
                        int64_t value1 = stackMem.getInt();
                        int64_t value2 = stackMem.getInt();
                        int16_t offset = bc->getInt16(iptr);
                        if (value2 > value1) {
                            iptr += offset;
                        } else {
                            iptr += sizeof(int16_t);
                        }
                        break;
                    }
                    case BC_IFICMPGE: {
                        int64_t value1 = stackMem.getInt();
                        int64_t value2 = stackMem.getInt();
                        int16_t offset = bc->getInt16(iptr);
                        if (value2 >= value1) {
                            iptr += offset;
                        } else {
                            iptr += sizeof(int16_t);
                        }
                        break;
                    }
                    case BC_IFICMPL: {
                        int64_t value1 = stackMem.getInt();
                        int64_t value2 = stackMem.getInt();
                        int16_t offset = bc->getInt16(iptr);
                        if (value2 < value1) {
                            iptr += offset;
                        } else {
                            iptr += sizeof(int16_t);
                        }
                        break;
                    }
                    case BC_IFICMPLE: {
                        int64_t value1 = stackMem.getInt();
                        int64_t value2 = stackMem.getInt();
                        int16_t offset = bc->getInt16(iptr);
                        if (value2 <= value1) {
                            iptr += offset;
                        } else {
                            iptr += sizeof(int16_t);
                        }
                        break;
                    }
                    case BC_CALL: {
//                        _variables.pushScope(func->id(), func->localsNumber());
                        uint16_t funcId = bc->getUInt16(iptr);
                        iptr += sizeof(uint16_t);

                        insnPtrs.push(iptr);
                        funcs.push(function);

                        auto func = static_cast<BytecodeFunction *>(functionById(funcId));
                        context->createContext(func);
                        bc = func->bytecode();

                        iptr = 0;
                        function = func;
                        break;
                    }
                    case BC_CALLNATIVE:
                        break;
                    case BC_RETURN:
//                        iptr = bc->length();
                        context->popContext(function);

                        iptr = insnPtrs.top();
                        insnPtrs.pop();

                        function = funcs.top();
                        funcs.pop();

                        bc = function->bytecode();
                        break;
                    case BC_INVALID:
                        assert("Invalid bytecode");
                    default:
                        assert(false && "Unknown bytecode");
                }
            }
        }

        void setContext(std::vector<TranslatedScope*>& scopes) {
            this->context = std::unique_ptr<Context>(new Context(std::move(scopes)));
        }
    public:
        Status* execute(vector<Var*>& vars) override {
            BytecodeFunction* main = static_cast<BytecodeFunction*>(functionById(0));

            // TODO: load arguments
            // load arguments and start
            execute(main);

            return Status::Ok();
        }
    };
}