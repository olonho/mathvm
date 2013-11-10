#ifndef VMRUNTIME_H
#define	VMRUNTIME_H
#include <stack>
#include <vector>
#include "mathvm.h"
#include "Utilities.h"
using std::stack;
using std::vector;
  

namespace mathvm {
    
    union StackValue {
        int64_t iVal;
        double dVal;
        uint16_t sVal;
    };
    
    class RuntimeException : public ExceptionInfo  {
    public:
        RuntimeException() : ExceptionInfo() {}
        RuntimeException(string message) : ExceptionInfo(message) {}
    };

    class VMStack {
    public:
        StackValue top() {
            return myStack.top();
        }
        int64_t topInt64() {
            return top().iVal;
        }    
        uint16_t topStringId() {
            return top().sVal;
        }
        double topDouble() {
            return top().dVal;
        }
        
        StackValue pop();
        int64_t popInt64();
        double popDouble();
        int16_t popStringId();
        
        void push(StackValue value) { myStack.push(value);}
        void pushInt64(int64_t value);
        void pushDouble(double value);
        void pushStringId(int16_t value);
        
        void dAdd() {
            sizeCheck(2);
            pushDouble(popDouble() + popDouble());
        }
        void iAdd() {
            sizeCheck(2);
            pushInt64(popInt64() + popInt64());
        }
        
        void iSub() {
            sizeCheck(2);
            int64_t a = popInt64();
            int64_t b = popInt64();
            pushInt64(a - b);
        }
        void dSub() {
            sizeCheck(2);
            double a = popDouble();
            double b = popDouble();
            pushDouble(a - b);
        }
        
        void iMul() {
            sizeCheck(2);
            int64_t a = popInt64();
            int64_t b = popInt64();
            pushInt64(a * b);
        }
        void dMul() {
            sizeCheck(2);
            double a = popDouble();
            double b = popDouble();
            pushDouble(a * b);
        }
        
        void iDiv() {
            sizeCheck(2);
            //cout << "---START aDIV---" << endl;
            int64_t a = popInt64();
            int64_t b = popInt64();
            //cout << "a =" << a << endl;
            //cout << "b =" << b << endl;
            //cout << "a / b = " << a / b << endl;
            pushInt64(a / b);
            //cout << "---END aDIV---" << endl;
        }
        void dDiv() {
            //cout << "---START dDIV---" << endl;
            sizeCheck(2);
            double a = popDouble();
            double b = popDouble();
            //cout << "a = " << a << endl;
            //cout << "b = " << b << endl;
            //cout << "a / b = " << (double)a / (double)b << endl;
            pushDouble((double)a / (double)b);
            //cout << "---END dDIV---" << endl;
        }
        
        void iMod() {
            sizeCheck(2);
            int64_t a = popInt64();
            int64_t b = popInt64();
            pushInt64(a % b);
        }
        
        void iNeg() {
            sizeCheck(1);
            pushInt64(-popInt64());
        }
        void dNeg() {
            sizeCheck(1);
            pushDouble(-popDouble());
        }
        
    private:
        stack<StackValue> myStack;
    private:
        void sizeCheck(size_t i) {
            if(myStack.size() < i) 
                throw RuntimeException("trying to pop from emptyStack");
        }
    };
    
    class VMFunctionContext {
    private:
        uint32_t myPos;
        BytecodeFunction* myFunction;
        map<int16_t, StackValue> myStorage;
    public:
        VMFunctionContext(BytecodeFunction* function) {
            myFunction = function;
            myPos = 0;
            myStorage.clear();
        }
    public: 
        int64_t getInt64(int16_t id);
        int16_t getStringId(int16_t id);
        double getDouble(int16_t id);
        StackValue get(int16_t id);
        
        void storeStringId(int16_t id, int16_t value);
        void storeInt64(int16_t id, int64_t value);
        void storeDouble(int16_t id, double value);
        void store(int16_t id, StackValue value);
        
        bool contains(int16_t id) {
            return myStorage.count(id) > 0;
        }
        
        uint32_t getPos() const {
            return myPos;
        }
        void moveTo(uint32_t newPos) {
            myPos = newPos;
        }
        
        Bytecode* bytecode() {
            return myFunction->bytecode();
        }
        
        uint16_t getId() {
            return myFunction->id();
        }
        
        string const & getName() {
            return myFunction->name();
        }

    };

}
#endif	/* VMSTACK_H */

