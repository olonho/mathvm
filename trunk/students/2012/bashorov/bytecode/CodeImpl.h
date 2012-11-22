#ifndef CODE_IMPL_EXECUTE
#define CODE_IMPL_EXECUTE

#include "mathvm.h"
namespace mathvm {

class CodeImpl : public Code
{
    typedef map<string, uint16_t> VariableMap;
    VariableMap _variableById;

public:

    // uint16_t makeVariable(const AstVar* var) {
    // 	return 0;
    // }

    // const uint16_t variableId(uint16_t id) const {
    // 	return 0;
    // }
	
    virtual Status* execute(vector<Var*>&);
};

}
#endif
