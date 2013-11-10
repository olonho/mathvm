#include "VMRuntime.h"

namespace mathvm {
    //VMStack
    StackValue VMStack::pop() {
        StackValue val = myStack.top();
        myStack.pop();
        return val;
    }
    int64_t VMStack::popInt64() {
        int64_t val = myStack.top().iVal;
        myStack.pop();
        return val;
    }
    double VMStack::popDouble() {
        double val = myStack.top().dVal;
        myStack.pop();
        return val;
    }
    int16_t VMStack::popStringId() {
        int16_t val = myStack.top().sVal;
        myStack.pop();
        return val;
    }
    
    
    void VMStack::pushInt64(int64_t value) {
        StackValue val;
        val.iVal = value;
        myStack.push(val);
    }  
    void VMStack::pushDouble(double value) {
        StackValue val;
        val.dVal = value;
        myStack.push(val);
    }   
    void VMStack::pushStringId(int16_t value) {
        StackValue val;
        val.sVal = value;
        myStack.push(val);    
    }
    
    //VMStorage
    int64_t VMFunctionContext::getInt64(int16_t id) {
        return get(id).iVal;
    }
    
    int16_t VMFunctionContext::getStringId(int16_t id) {
        return get(id).sVal; 
    }
    double VMFunctionContext::getDouble(int16_t id) {
        return get(id).dVal;
    }
    StackValue VMFunctionContext::get(int16_t id) {
        if(myStorage.count(id) == 0) throw RuntimeException() << 
                "(VMFunctionContext::get) attempt to get value of uninitialized var";
        return myStorage[id];
    }

    void VMFunctionContext::storeStringId(int16_t id, int16_t value) {
        StackValue val;
        val.sVal = value; 
        myStorage[id] = val;
    }
    void VMFunctionContext::storeInt64(int16_t id, int64_t value) {
        StackValue val;
        val.iVal = value; 
        myStorage[id] = val;
    }
    void VMFunctionContext::storeDouble(int16_t id, double value) {
        StackValue val;
        val.dVal = value; 
        myStorage[id] = val;
    }
    
    void VMFunctionContext::store(int16_t id, StackValue value) {
        myStorage[id] = value;
    }

}
