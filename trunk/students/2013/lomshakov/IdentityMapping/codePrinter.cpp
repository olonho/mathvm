#include "codePrinter.h"
#include <algorithm>

namespace mathvm {

void CodePrinter::print(AstFunction* topLevelFunction) {
  visitBlockNodeWOBraces(topLevelFunction->node()->body());
}

void CodePrinter::printVariableDeclaration(AstVar *var) {
  _out << typeToName(var->type()) << " "
      << var->name()
      << ";" << endl;
}

void CodePrinter::visitPrintNode(PrintNode *node) {
  _out << "print(";
  for(uint32_t i = 0; i != node->operands(); ++i) {
    node->operandAt(i)->visit(this);
    _out << ((i+1) == node->operands() ? ");" : ",");
  }
  _out << endl;
}

void CodePrinter::visitBinaryOpNode(BinaryOpNode *node) {
  _out << "(";
  node->left()->visit(this);
  _out << tokenOp(node->kind());
  node->right()->visit(this);
  _out << ")";
}

void CodePrinter::visitBlockNodeWOBraces(BlockNode* node)
{
  for (Scope::VarIterator i(node->scope()); i.hasNext();) {
    printVariableDeclaration(i.next());
  }

  for (Scope::FunctionIterator i(node->scope()); i.hasNext();) {
    printFunctionDeclaration(i.next());
  }

//  blockHook = node;
  for (uint32_t i = 0; i != node->nodes(); ++i) {
    node->nodeAt(i)->visit(this);
//    blockHook = node;
  }

}

void CodePrinter::visitBlockNode(BlockNode *node) {
  _out << "{" << endl;

  visitBlockNodeWOBraces(node);

  _out << "}" << endl;
}

void CodePrinter::visitCallNode(CallNode *node) {
  _out << node->name() <<  "(";
  for(uint32_t i = 0; i != node->parametersNumber(); ++i) {
    node->parameterAt(i)->visit(this);
    _out << ((i+1) == node->parametersNumber() ? "" : ",");
  }
  _out << ") ";
}

void CodePrinter::visitDoubleLiteralNode(DoubleLiteralNode *node) {
  _out << fixed << node->literal();
}

void CodePrinter::visitForNode(ForNode *node) {
  _out << "for(" << node->var()->name() << " in ";
  node->inExpr()->visit(this);
  _out << ")";
  node->body()->visit(this);
}

void CodePrinter::visitLoadNode(LoadNode *node) {
  _out << node->var()->name();
}

void CodePrinter::visitIfNode(IfNode *node) {
  _out << "if " ;
  node->ifExpr()->visit(this);
  _out << endl;
  node->thenBlock()->visit(this);
  if (node->elseBlock() != 0) {
    _out << "else";
    node->elseBlock()->visit(this);
  }
}

void CodePrinter::visitStoreNode(StoreNode *node) {
  _out << node->var()->name() << tokenOp(node->op());
  node->value()->visit(this);
  _out << ";" << endl;
}

void CodePrinter::visitStringLiteralNode(StringLiteralNode *node) {
  string rawString;
  for_each(node->literal().begin(), node->literal().end(), [&](char it) {
    if (it == '\n' )
      rawString.append("\\n");
    else if (it == '\t')
      rawString.append("\\t");
    //TODO: add other escape sequence
    else
      rawString.push_back(it);
  });
  _out << "'" << rawString << "'";

}

void CodePrinter::visitWhileNode(WhileNode *node) {
  _out << "while";
  node->whileExpr()->visit(this);
  _out << endl;
  node->loopBlock()->visit(this);

}

void CodePrinter::visitIntLiteralNode(IntLiteralNode *node) {
  _out << node->literal();
}

void CodePrinter::visitUnaryOpNode(UnaryOpNode *node) {
  _out << tokenOp(node->kind());
  node->operand()->visit(this);
}

void CodePrinter::visitReturnNode(ReturnNode *node) {
  _out << "return ";
  if (node->returnExpr() != 0)
    node->returnExpr()->visit(this);
  _out << ";" << endl;
}

void CodePrinter::printFunctionDeclaration(AstFunction *astFunction) {
  FunctionNode* node = astFunction->node();

  _out << "function " << typeToName(node->returnType())
      << " " << node->name() << "(";
  for(uint32_t i = 0; i != node->parametersNumber(); ++i) {
    _out << typeToName(node->parameterType(i)) << " "
        << node->parameterName(i)
        << ((i+1) == node->parametersNumber() ? ")" : ",");
  }

  BlockNode* block = node->body();
  if (block->nodeAt(0)->isNativeCallNode()) {
    _out << " native " << "'" + block->nodeAt(0)->asNativeCallNode()->nativeName() + "'" << ";" << endl;
  } else {
    _out << endl;
    block->visit(this);
  }
}

void CodePrinter::visitFunctionNode(FunctionNode *node) {

}

void CodePrinter::visitNativeCallNode(NativeCallNode *node) {

}

}
