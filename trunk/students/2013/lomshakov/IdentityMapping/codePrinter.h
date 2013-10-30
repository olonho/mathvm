#ifndef _MATHVM_CODEPRINTER_H_
#define _MATHVM_CODEPRINTER_H_

#include "mathvm.h"
#include "ast.h"

namespace mathvm {

class CodePrinter : public AstVisitor {
  ostream& _out;

  void visitBlockNodeWOBraces(BlockNode* node);
  void printVariableDeclaration(AstVar* var);
  void printFunctionDeclaration(AstFunction* astFunction);
public:
  CodePrinter(ostream& out) : _out(out) {}

  void print(AstFunction * topLevelFunction);

#define VISITOR_FUNCTION(type, name)            \
  virtual void visit##type(type* node);

  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}

#endif //_MATHVM_CODEPRINTER_H_
