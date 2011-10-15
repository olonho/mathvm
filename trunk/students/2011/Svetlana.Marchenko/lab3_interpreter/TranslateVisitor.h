#pragma once

#include "ast.h"
#include "mathvm.h"
#include "VarTable.h"
#include "CodeInterpreter.h"

class TranslateVisitor: public mathvm::AstVisitor {
	VarTable _varTable;
	mathvm::Bytecode _byteCode;
	mathvm::VarType _operandType;
	CodeInterpreter _code;
public:
	TranslateVisitor(): _operandType(mathvm::VT_INVALID) {
	}
    
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(mathvm::type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    void visit( mathvm::BlockNode * rootNode );
    mathvm::Bytecode* GetBytecode();
    std::vector<std::string> GetStringsVector();
    void dump();

};

struct TranslationException {
  
  TranslationException(std::string const& message) : _message(message) {}
  
  virtual std::string what() const { return _message; }
  
private:
  std::string _message;
};

