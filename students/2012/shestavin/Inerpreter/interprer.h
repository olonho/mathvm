
#ifndef __Interpreter__interprer__
#define __Interpreter__interprer__

#include <iostream>
#include <string>
#include <stack>
#include <map>
#include <cmath>
#include <iomanip>
#include "parser.h"
#include "mathvm.h"


namespace mathvm {
    
    class Interpreter: public Code {
        
        static const int commandLen[];
        
        stack<Var> programStack;
        map<const uint16_t, Var> memory;
        stack<uint16_t> returnAddresses;
        stack<BytecodeFunction*> returnLocations;
        
        void funExec(BytecodeFunction* f);
        
    public:
        virtual Status* execute(vector<Var*> & vars);
        
    };
}

#endif /* defined(__Interpreter__interprer__) */
