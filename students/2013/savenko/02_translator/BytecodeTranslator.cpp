#include <mathvm.h>
#include <visitors.h>

#include <stdexcept>

namespace mathvm {

class BytecodeTranslator : public AstVisitor {

void visitBinaryOpNode(BinaryOpNode * binaryOpNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  binaryOpNode->left()->visit(this);
  std::cout << " " << tokenOp(binaryOpNode->kind()) << " ";
  binaryOpNode->right()->visit(this);
}

void visitUnaryOpNode(UnaryOpNode * unaryOpNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << tokenOp(unaryOpNode->kind());
  unaryOpNode->operand()->visit(this);
}

void visitStringLiteralNode(StringLiteralNode * stringLiteralNode) {
 throw std::logic_error("NOT IMPLEMENTED");
 std::cout << '\'' << stringLiteralNode->literal() << '\'';
}

void visitDoubleLiteralNode(DoubleLiteralNode * doubleLiteralNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << doubleLiteralNode->literal();
} 

void visitIntLiteralNode(IntLiteralNode * intLiteralNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << intLiteralNode->literal();
}

void visitLoadNode(LoadNode * loadNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << loadNode->var()->name();
}

void visitStoreNode(StoreNode * storeNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << storeNode->var()->name()
            << " " << tokenOp(storeNode->op()) << " ";
  storeNode->value()->visit(this);
}

void visitForNode(ForNode * forNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << "for (" << forNode->var()->name() << " in ";
  forNode->inExpr()->visit(this);
  std::cout << ")" << std::endl;
  forNode->body()->visit(this);
}

void visitWhileNode(WhileNode * whileNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << "while (";
  whileNode->whileExpr()->visit(this);
  std::cout << ")" << std::endl;
  whileNode->loopBlock()->visit(this);
}

void visitIfNode(IfNode * ifNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << "if (";
  ifNode->ifExpr()->visit(this);
  std::cout << ")" << std::endl;
  ifNode->thenBlock()->visit(this);
  if (ifNode->elseBlock()) {
    std::cout << "else" << std::endl;
    ifNode->elseBlock()->visit(this);
  }
}

void visitBlockNodeInternal(BlockNode * blockNode, bool needIndentation) {
  if (needIndentation) {
    std::cout << "{" << std::endl;
  }
  for(Scope::VarIterator varIterator(blockNode->scope()); varIterator.hasNext(); ) {
    AstVar * var = varIterator.next();
    std::cout << typeToName(var->type()) << " " << var->name()
              << ";" << std::endl;
  }
  std::cout << std::endl;
  for (uint32_t i = 0; i < blockNode->nodes(); ++i) {
    blockNode->nodeAt(i)->visit(this);
    std::cout << ";" << std::endl;
  }
  if (needIndentation) {
    std::cout << "}" << std::endl;
  }
}

void visitBlockNode(BlockNode * blockNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  visitBlockNodeInternal(blockNode, true);
}

void visitFunctionNode(FunctionNode * functionNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  bool topNode = functionNode->position() == 0;
  if (!topNode) {
    std::cout << "function "
              << typeToName(functionNode->returnType())
              << " " << functionNode->name() << "(";
  }
  for (uint32_t i = 0; i != functionNode->parametersNumber(); ++i) {
    std::cout << typeToName(functionNode->parameterType(i))
              << " "
              << functionNode->parameterName(i);
    if (!topNode && i != functionNode->parametersNumber() - 1) {
      std::cout << ", ";
    }
  }
  if (!topNode) std::cout << ")" << std::endl;
  visitBlockNodeInternal(functionNode->body(), !topNode);
}

void visitReturnNode(ReturnNode * returnNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << "return ";
  returnNode->returnExpr()->visit(this);
}

void visitCallNode(CallNode * callNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << callNode->name() << "(";
  for (uint32_t i = 0; i != callNode->parametersNumber(); ++i) {
    callNode->parameterAt(i)->visit(this);
    if (i != callNode->parametersNumber() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << ")";
}

void visitNativeCallNode(NativeCallNode * NativeCallNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << "NativeCall" << std::endl;
}

void visitPrintNode(PrintNode * printNode) {
  throw std::logic_error("NOT IMPLEMENTED");
  std::cout << "print (";
  for (uint32_t i = 0; i != printNode->operands(); ++i) {
    printNode->operandAt(i)->visit(this);
    if (i != printNode->operands() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << ")";
}


};

Status* BytecodeTranslatorImpl::translateBytecode(std::string const & program, InterpreterCodeImpl** code) {
  throw std::logic_error("NOT IMPLEMENTED");
  return 0;
}

Status* BytecodeTranslatorImpl::translate(std::string const & program, Code** code) {
  throw std::logic_error("NOT IMPLEMENTED");
  return 0;
}

}
