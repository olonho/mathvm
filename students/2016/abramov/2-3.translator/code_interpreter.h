#ifndef CODEINTERPRETER_H
#define CODEINTERPRETER_H

#include <stack>
#include <string>

#include "mathvm.h"
#include "interpreter_context.h"

namespace mathvm
{   
    class CodeInterpreter : public Code
    {
    public:
        CodeInterpreter();
        CodeInterpreter(const CodeInterpreter& other);
        virtual ~CodeInterpreter();
        virtual Status* execute(vector<Var*>& vars) override;
    
    private:
        void executeInstruction(Instruction instruction);
        bool tryLoad(Instruction instruction);
        bool tryStore(Instruction instruction);
        bool tryArithmetic(Instruction instruction);
        bool tryLogic(Instruction instruction);
        bool tryPrint(Instruction instruction);
        bool tryConvert(Instruction instruction);
        bool tryCompare(Instruction instruction);
        bool tryOther(Instruction Instruction);
        
        StackElement popStack();
    
    private:
        InterpreterContext* _context;
        std::stack<StackElement> _stack;
    };
}



#endif /* CODEINTERPRETER_H */

