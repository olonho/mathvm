#include "ast.h"
#include "mathvm.h"
#include "VarTable.h"

class TranslateVisitor: public AstVisitor {
	VarTable _varTable;
	mathvm::ByteCode _byteCode;
	mathvm::VarType _operandType;
	mathvm::Code _code;
public:
	TranslateVisitor(): _operandType(VT_INVALID) {
	}
    
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node) {}

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION	
};
