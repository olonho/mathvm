#pragma once

#include <mathvm.h>
#include <ast.h>

#include <cstdint>
#include <map>
#include <vector>
#include <string>

namespace mathvm {

    using std :: map;
    using std :: vector;
    using std :: string;

    namespace details {
        struct TranslatorVar {
            TranslatorVar(uint16_t id, uint16_t contextId);
            uint16_t id();
            uint16_t contextId();
        private:
            uint16_t _id;
            uint16_t _contextId;
        };

        struct ScopeData {
            ScopeData(BytecodeFunction* function, ScopeData* parent = nullptr);

            uint16_t getId();
            uint16_t getVarCount();

            void addAstVar(const AstVar* var);
            TranslatorVar getVar(const AstVar* var);

            BytecodeFunction* function();
            ScopeData* parent();
            map<string, uint16_t> variables();
            int localsCount();
        private:
            BytecodeFunction* _function;
            ScopeData* _parent;
            std :: map<string, uint16_t> _variables;
            int _localsCount;
        };

        struct StackValue {
            union {
                int64_t _intVal;
                double _doubleVal;
                uint16_t _stringRef;
            } _holder;

            StackValue();

            static StackValue saveInt(int64_t val);
            static StackValue saveDouble(double val);
            static StackValue saveStringRef(uint16_t val);

            int64_t intVal();
            double doubleVal();
            uint16_t stringRef();
        };

        struct Context {
            Context(BytecodeFunction* function, Context* parent = nullptr);

            StackValue getContextVal();
            void setContextVal(StackValue val);

            StackValue getVal();
            void setVal(StackValue val);

            void jump(int16_t offset);

            uint16_t getStringRef();
            int64_t getInt();
            int16_t getShort();
            double getDouble();

            Context* parent();
            Instruction getCurrentInstruction();
            Bytecode* getCurrentBytecode();
        private:
            StackValue getContextValHelper(uint16_t varId, uint16_t contextId);
            void setContextValHelper(uint16_t varId, uint16_t contextId, StackValue val);
        private:
            vector<StackValue> _env;
            BytecodeFunction* _function;
            Context* _parent;
            uint64_t _ip;
        };


        Instruction ADD(VarType type);
        Instruction SUB(VarType type);
        Instruction MUL(VarType type);
        Instruction DIV(VarType type);
        Instruction COMPARE(VarType type);

        Instruction NEGATE(VarType type);

        Instruction LOAD(VarType type);
        Instruction STORE(VarType type);

        Instruction LOAD_CTX(VarType type);
        Instruction STORE_CTX(VarType type);

        Instruction PRINT(VarType type);
    } // end namespace details
} // end namespace mathvm
