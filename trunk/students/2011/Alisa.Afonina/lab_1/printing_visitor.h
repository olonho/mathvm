#pragma once

class PrintingVisitor : public mathvm::AstVisitor {
	static std::string getType(mathvm::VarType type);
  
public:
	PrintingVisitor(){};
  
#define VISIT_FUNCTION(type, name) \
	void visit##type(mathvm::type* node);
	FOR_NODES(VISIT_FUNCTION)
#undef VISIT_FUNCTION
 
};
