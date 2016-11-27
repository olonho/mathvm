#ifndef INTERPRETERCONTEXT_H
#define INTERPRETERCONTEXT_H

#include "ast.h"
#include "mathvm.h"
namespace mathvm 
{
    class StackElement
    {
    public:
        StackElement();
        StackElement(int64_t value);
        StackElement(double value);
        StackElement(uint16_t value);

        // Getters
        int64_t  getInt64()  const;
        double   getDouble() const;
        uint16_t getUint16() const;
    private:
        void testType(VarType type) const;
        
    private:
        VarType _type;
        union 
        {
            int64_t  int64Value;
            double   doubleValue;
            uint16_t uint16Value;
        } _value;
    };

    class InterpreterContext 
    {
    public:
        InterpreterContext(BytecodeFunction* func, InterpreterContext* parent = nullptr);
        InterpreterContext(const InterpreterContext& other);
        virtual ~InterpreterContext();
    
    public:
        Bytecode* getBytecode();
        uint32_t getPosition() const;
        Instruction getInstruction();
        bool hasNextInstruction();
    
    private:
        BytecodeFunction* _func;
        InterpreterContext* _parent;
        vector<StackElement> _vars;
        uint32_t _pos;
    };   
}



#endif /* INTERPRETERCONTEXT_H */

