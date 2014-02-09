#ifndef CODEIMPL_H_
#define CODEIMPL_H_

#include <vector>
#include "mathvm.h"


using namespace mathvm;


//Simple Code implementation

class CodeImpl: public Code {

        public:

		CodeImpl(AstFunction * entry_point) : _entry_point(entry_point) {
			//addFunction(&_entry_point);
		}

		Status* execute(std::vector<Var*> & vars) {
                return 0;
        }

        Bytecode* bytecode() {
        	return _entry_point.bytecode();
        }

        //private:
        BytecodeFunction _entry_point;
};



#endif /* CODEIMPL_H_ */
