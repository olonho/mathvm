
#ifndef MOCK_INTERPRETER_H
#define	MOCK_INTERPRETER_H

#include "mathvm.h"
#include "ast.h"
#include "parser.h"

//Wrapper to use functions of class Code.
namespace mathvm {

    class MockInterpreter : public Code {        
    public:
        MockInterpreter();
        virtual Status* execute(std::vector<Var*>&); //empty
    };

}
#endif	/* MOCK_INTERPRETER_H */

