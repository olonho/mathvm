#pragma once

#include <mathvm.h>
#include "context.hpp"

namespace mathvm {

    struct VectorStack {
        void pushInt(int64_t args);
        void pushDouble(double d);
        void pushUInt16(uint16_t id);
        void pushVal(Val const& top) ;
        Val popVal();
        int64_t popInt();
        double popDouble();
        uint16_t popUInt16();
    private:
        std::vector<Val> stack_;
    };

    struct ExecutionContextHolder {
        ExecutionContext* topContext();
        void enterContext(BytecodeFunction* pFunction);
        void leaveContext();

        ExecutionContext* findContext(uint16_t scopeId);

    private:
        std::vector<ExecutionContext> contexts_;
        std::vector<std::vector<size_t>> scopeIdToContextMap_ {100}; // no more than 100 functions yet
    };

    class InterpreterCodeImpl : public Code {
        ExecutionContextHolder contextHolder_;
        VectorStack stack_;

        void execute(Instruction instruction);

    public:
        mathvm::Status* execute(std::vector<mathvm::Var*>& vars) override;
    };

}