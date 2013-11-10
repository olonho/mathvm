#ifndef INTERPRETER_CODE_IMPL_H
#define	INTERPRETER_CODE_IMPL_H

#include "mathvm.h"
#include "ast.h"
#include "parser.h"
#include "VMRuntime.h"

namespace mathvm {
    class InterpreterCodeImpl : public Code {
    private:
        VMStack myStack;
        vector<VMFunctionContext*> myContexts;  
        Bytecode* bytecode;
        uint32_t myPos;
        ostream& myOut; 
        
    public:
        InterpreterCodeImpl(ostream& output);
        virtual ~InterpreterCodeImpl();
    
        virtual Status* execute(std::vector<Var*>&);
        
    private:
        VMFunctionContext* context() {
           return myContexts.empty() ? 0 : myContexts.back();
        }
        
        uint16_t nextStringId();
        uint16_t nextUInt16();
        int64_t nextInt64();
        double nextDouble();
        
        template<class C>
        void jumpIf(C comparator) {
            int64_t left = myStack.popInt64();
            int64_t right = myStack.popInt64();
            bool result = comparator(left, right);
            myPos += result ? bytecode->getInt16(myPos) : sizeof(uint16_t);
        }
        
        void loadFromContext(uint16_t context_id) {
            
            for(int i = myContexts.size() - 1; i >= 0; --i) {
                if(context_id == myContexts[i]->getId()){
                     myStack.push(myContexts[i]->get(nextUInt16()));
                     return;
                }
            }
            throw RuntimeException() << "try to load uninitialized variable";
        }
        
        void storeToContext(uint16_t context_id) {
            for(int i = myContexts.size() - 1; i >= 0; --i) {
                if(context_id == myContexts[i]->getId()){
                    myContexts[i]->store(nextUInt16(),myStack.pop());
                    return;
                }
            }
            
            throw RuntimeException() 
                    << "there is no context with id =" << context_id;
        }
       
        void enterFunction(uint16_t id) {
            if (context() != 0) {
                context()->moveTo(myPos);
            }
      
            BytecodeFunction* function = (BytecodeFunction*)functionById(id);
            myContexts.push_back(new VMFunctionContext(function));
            
            myPos = 0;
            bytecode = function->bytecode();
            
        }
        
        void exitFunction() {
        	VMFunctionContext* tmp = context();
            myContexts.pop_back();
            delete tmp;
            
            myPos = context()->getPos();
            bytecode = context()->bytecode();
        }        
             
    };
}
#endif	/* INTERPRETERCODEIMPL_H */

