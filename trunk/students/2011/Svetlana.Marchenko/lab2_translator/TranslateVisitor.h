#pragma once

#include "ast.h"
#include "mathvm.h"
#include "VarTable.h"
#include "MyCode.h"
#include "ScopeWrapper.h"

class TranslateVisitor: public mathvm::AstVisitor {
	mathvm::Bytecode *_byteCode;
	mathvm::VarType _operandType;
	MyCode _code;
	ScopeWrapper *_scope;
		
public:
	TranslateVisitor(): 
		_byteCode(0),
		_operandType(mathvm::VT_INVALID),
		_scope(0)
	{
	}
    
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(mathvm::type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    void visit( mathvm::BlockNode * rootNode );
    //mathvm::Bytecode* getBytecode();
    std::vector<std::string> getStringsVector();
    void dump();

};

struct TranslationException {
  
  TranslationException(std::string const& message) : _message(message) {}
  
  virtual std::string what() const { return _message; }
  
private:
  std::string _message;
};

