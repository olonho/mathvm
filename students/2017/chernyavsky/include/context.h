#pragma once

#include <mathvm.h>

namespace mathvm {

    using id_t = uint16_t;

    union Val {
        double d;
        int64_t i;
        id_t s;
    };

    struct Context {

        explicit Context(BytecodeFunction* func, Context* parent = nullptr, Scope* scope = nullptr);

        BytecodeFunction* getFunction();

        Context* getParent();

        Bytecode* getBytecode();

        uint32_t getBytecodePosition();

        VarType getTosType();

        void setTosType(VarType type);

        uint64_t getId();

        Val getVar(id_t varId);

        id_t getVarId(const string& name);

        Val* findVar(const string& name);

        void setVar(id_t varId, Val newVal);

        void addVar(const string& name);

        id_t getLocalsNumber();

        double readDouble();

        int64_t readInt();

        id_t readId();

        int16_t readOffset();

        Instruction readInsn();

        void jump(int16_t offset);

    private:
        BytecodeFunction* _function;
        Context* _parent;
        uint32_t _bytecodePos;
        VarType _tosType;
        std::vector<Val> _locals;
        std::map<std::string, id_t> _idByName;
        uint64_t _scopeId;
    };

}