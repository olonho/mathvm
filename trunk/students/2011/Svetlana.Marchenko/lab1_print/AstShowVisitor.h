#pragma once

#include <ostream>
#include <string>
#include "ast.h"
#include "mathvm.h"

class AstShowVisitor: public mathvm::AstVisitor {
	std::ostream& _outputStream;
		
	std::string getTypeName(mathvm::VarType type);
	
public:
	AstShowVisitor(std::ostream& o):
		_outputStream(o){
	}
    
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(mathvm::type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
}; 

