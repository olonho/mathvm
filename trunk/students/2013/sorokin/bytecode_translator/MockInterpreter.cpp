#include "MockInterpreter.h"
//just a useless file
namespace mathvm { 
    MockInterpreter::MockInterpreter() {}
    
    Status* MockInterpreter::execute(std::vector<Var*>&) {
        return 0; //execute does nothing!
    }
    
} // mathvm
