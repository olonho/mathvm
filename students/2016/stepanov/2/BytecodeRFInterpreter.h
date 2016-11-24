//
// Created by user on 11/12/16.
//

#ifndef VM_BYTECODE_PRINTER_INTERPRETER_H
#define VM_BYTECODE_PRINTER_INTERPRETER_H

#include <cstring>
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include <set>
#include <stack>

namespace mathvm {
    class StackItem {
    public:
        union {
            int64_t intValue;
            double doubleValue;
            uint16_t stringIdValue;
        } value;
        StackItem() {
            value.intValue = 0;
        }

        StackItem(int64_t arg) {
            value.intValue = arg;
        }

        StackItem(double arg) {
            value.doubleValue = arg;
        }

        StackItem(uint16_t arg) {
            value.stringIdValue = arg;
        }
    };

    static StackItem STACK_EMPTY = StackItem((int64_t)0);
    extern std::vector<StackItem> variables;

    extern std::stack<size_t> scopeOffsets[UINT16_MAX];

    class InterScope {
    private:
        Bytecode *bytecode;
        BytecodeFunction *bf;
    public:
        //[TODO] REWRITE
        Status *status = nullptr;
        uint32_t IP = 0;
        size_t variableOffset;
        InterScope * parent;

        InterScope(BytecodeFunction *bf, InterScope * parent = nullptr);

        ~InterScope();

        Instruction next();

        inline uint16_t nextUint16t();

        inline void skipUint16t();

        inline int64_t nextInt();

        inline double nextDouble();

        inline void jump();

        StackItem* variableLookup(uint16_t scope, uint16_t variable){
            if (bf->scopeId() == scope){
                return &(variables[variableOffset + variable]);
            }
            else {
                return &(variables[scopeOffsets[scope].top() + variable]);
            }
        }
   };

    typedef int64_t (*nativePtr)(int64_t *);
    typedef double (*nativeDoublePtr)(int64_t *);
    typedef double (*nativeVoidPtr)(int64_t *);

    extern int64_t nativeLinks[UINT16_MAX];
    extern nativePtr nativeFunctions[UINT16_MAX];

    class BytecodeRFInterpreterCode : public Code {
    private:
        size_t variablesOffset;
        std::vector<StackItem> stack;
        InterScope *is = nullptr;

        bool evaluateThis(Instruction instr);
        uint16_t emptyString();
    public:
        ~BytecodeRFInterpreterCode() {
            delete(is);
        }
        BytecodeRFInterpreterCode() {
            stack.reserve(100);
            memset(nativeLinks, 0, sizeof (int64_t)*UINT16_MAX);
            memset(nativeFunctions, 0, sizeof (nativePtr)*UINT16_MAX);
        }

        virtual Status *execute(vector<Var *> &vars) override {
            BytecodeFunction *topFunction = (BytecodeFunction *) functionByName(AstFunction::top_name);
            is = new InterScope(topFunction);
            variablesOffset = is->variableOffset;

            Instruction nextInstruction;
            while ((nextInstruction = is->next()) != BC_STOP) {
                if (!evaluateThis(nextInstruction)) {
                    break;
                }
            }
            return is ? is->status : Status::Ok();
        }

        int64_t getStringConstantPtrById(uint16_t id);

        void registerStringConstantPtrById(uint16_t id, int64_t ptr);
    };
}

#endif //VM_BYTECODE_PRINTER_INTERPRETER_H
