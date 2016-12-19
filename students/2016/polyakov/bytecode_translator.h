#pragma once

#include <string>
#include <sstream>
#include "visitors.h"
#include "ast.h"
#include "context.h"

namespace mathvm {

class BytecodeTranslator : public AstBaseVisitor {
private:
    Code* code;
    Context* context;

public:
  BytecodeTranslator(Code* code):
    code(code),
    context(NULL) {}

#define VISITOR_FUNCTION(type, name) \
  virtual void visit##type(type* node);

  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

  void processFunc(AstFunction *f);

private:
  Bytecode* bc() {
      return context->bc();
  }
  void addVarIns(const AstVar * var, Instruction& localIns, Instruction& globalIns);
  void storeVar(const AstVar * var);
  void loadVar(const AstVar * var);
  void castTOS(VarType type);
  Instruction chooseInsn(VarType type, Instruction intInsn, Instruction doubleInsn);
  void visitArithmOp(BinaryOpNode* node, Instruction& insnInt, Instruction& insnDouble);
  void castArithmTypes(VarType type1, VarType type2);
  void visitLogicOp(BinaryOpNode* pNode);
  void visitCompareOp(BinaryOpNode* pNode, Instruction& insnCmp);
  void visitBitwiseOp(BinaryOpNode* pNode, Instruction& insnBitwise);
  void addNotNode(UnaryOpNode *Node);
  void addMinusNode(UnaryOpNode *node);
};

}