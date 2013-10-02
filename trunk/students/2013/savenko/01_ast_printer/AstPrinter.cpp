#include "AstPrinter.h"
#include <iostream>

namespace mathvm {

// utility

std::string unescape(std::string const & str) {
  std::string result;
  for (std::string::size_type i = 0; i != str.size(); ++i) {
    char ch = str[i];
    switch (ch) {
      case '\n': {
        result += "\\n";
        break;
      }
      case '\t': {
        result += "\\t";
        break;
      }
      case '\r': {
        result += "\\r";
        break;
      }
      case '\\': {
        result += "\\\\";
        break;
      }
      default: {
        result += ch;
      }
    }
  }
  return result;
}

// visitor implementation

void AstPrinter::visitBinaryOpNode(BinaryOpNode * binaryOpNode) {
  binaryOpNode->left()->visit(this);
  std::cout << " " << tokenOp(binaryOpNode->kind()) << " ";
  binaryOpNode->right()->visit(this);
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode * unaryOpNode) {
  std::cout << tokenOp(unaryOpNode->kind());
  unaryOpNode->operand()->visit(this);
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode * stringLiteralNode) {
  std::cout << '\'' << unescape(stringLiteralNode->literal()) << '\'';
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode * doubleLiteralNode) {
  std::cout << doubleLiteralNode->literal();
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode * intLiteralNode) {
  std::cout << intLiteralNode->literal();
}

void AstPrinter::visitLoadNode(LoadNode * loadNode) {
  std::cout << loadNode->var()->name();
}

void AstPrinter::visitStoreNode(StoreNode * storeNode) {
  std::cout << storeNode->var()->name()
            << " " << tokenOp(storeNode->op()) << " ";
  storeNode->value()->visit(this);
}

void AstPrinter::visitForNode(ForNode * forNode) {
  std::cout << "for (" << forNode->var()->name() << " in ";
  forNode->inExpr()->visit(this);
  std::cout << ")" << std::endl;
  forNode->body()->visit(this);
}

void AstPrinter::visitWhileNode(WhileNode * whileNode) {
  std::cout << "while (";
  whileNode->whileExpr()->visit(this);
  std::cout << ")" << std::endl;
  whileNode->loopBlock()->visit(this);
}

void AstPrinter::visitIfNode(IfNode * ifNode) {
  std::cout << "if (";
  ifNode->ifExpr()->visit(this);
  std::cout << ")" << std::endl;
  ifNode->thenBlock()->visit(this);
  if (ifNode->elseBlock()) {
    std::cout << indent() << "else" << std::endl;
    ifNode->elseBlock()->visit(this);
  }
}

void AstPrinter::visitBlockNode(BlockNode * blockNode) {
  std::cout << indent() << "{" << std::endl;
  increaseIndent();
  for (uint32_t i = 0; i < blockNode->nodes(); ++i) {
    std::cout << indent();
    blockNode->nodeAt(i)->visit(this);
    std::cout << ";" << std::endl;
  }
  decreaseIndent();
  std::cout << indent() << "}" << std::endl;
}

void AstPrinter::visitFunctionNode(FunctionNode * functionNode) {
  std::cout << indent() << "function "
            << typeToName(functionNode->returnType())
            << " " << functionNode->name() << "(";
  for (uint32_t i = 0; i != functionNode->parametersNumber(); ++i) {
    std::cout << typeToName(functionNode->parameterType(i))
              << " "
              << functionNode->parameterName(i);
    if (i != functionNode->parametersNumber() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << ")" << std::endl;
  visitBlockNode(functionNode->body());
}

void AstPrinter::visitReturnNode(ReturnNode * returnNode) {
  std::cout << indent() << "return ";
  returnNode->returnExpr()->visit(this);
}

void AstPrinter::visitCallNode(CallNode * callNode) {
  std::cout << callNode->name() << "(";
  for (uint32_t i = 0; i != callNode->parametersNumber(); ++i) {
    callNode->parameterAt(i)->visit(this);
    if (i != callNode->parametersNumber() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << ")";
}

void AstPrinter::visitNativeCallNode(NativeCallNode * NativeCallNode) {
  std::cout << "NativeCall" << std::endl;
}

void AstPrinter::visitPrintNode(PrintNode * printNode) {
  std::cout << "print (";
  for (uint32_t i = 0; i != printNode->operands(); ++i) {
    printNode->operandAt(i)->visit(this);
    if (i != printNode->operands() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << ")";
}


}
