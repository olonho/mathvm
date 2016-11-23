#pragma once

#include <visitors.h>
#include <mathvm.h>
#include <stack>
#include <limits>
#include "context.h"
#include "translator_exception.h"

namespace mathvm {
class MathVmTranslator : public AstBaseVisitor {
public:
  MathVmTranslator(Code* code);

  Status* run(AstFunction* root);

#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node) override;

  FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION
private:

  Code* _code;
  Context* _context;
  VarType _typeOfTopOfStack;

  void handleArithmeticOperation(BinaryOpNode* node);

  void handleLogicalOperation(BinaryOpNode* node);

  void handleBitwiseOperation(BinaryOpNode* node);

  void handleComparisonOperation(BinaryOpNode* node);

  void handleFunction(AstFunction* astFunction);

  VarType getBinaryOperationResultType(VarType first, VarType second);

  void convertTopOfStackTo(VarType to);

  inline void translationAssert(bool condition, std::string const& message, uint32_t position) {
    if (!condition) {
      throw TranslationException(message, position);
    }
  }

  void loadVariableToStack(AstVar const* variable);

  void storeTopOfStackToVariable(AstVar const* variable);

  void visitNodeWithResult(AstNode* node);

  void swap();

  inline Context* currentContext() const {
    return _context;
  }

  inline Bytecode* getBytecode() {
    return currentContext()->getCurrentFunction()->bytecode();
  }
}; // MathVmTranslator
} // mathvm