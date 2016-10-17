#include "parser.h"
#include "mathvm.h"

#include "ast_printer.h"

namespace mathvm {

const std::string ASTPrinter::indent = "  ";
const std::string ASTPrinter::separator = " ";
const std::string ASTPrinter::endline = "\n";

Status* ASTPrinter::print_code(std::string &outputSource, const std::string &inputSource) {
  Parser parser; 
  Status* status = parser.parseProgram(inputSource); 
  if (status->isError()) {
      return status;
  }
  parser.top()->node()->visit(this);
  outputSource = sstream.str();
  return Status::Ok();
}

void ASTPrinter::visitBinaryOpNode(BinaryOpNode* node) { 
  sstream << "(";
  node->left()->visit(this);
  sstream << separator << tokenOp(node->kind()) << separator;
  node->right()->visit(this);
  sstream << ")";
}

void ASTPrinter::visitUnaryOpNode(UnaryOpNode *node) {
  sstream << tokenOp(node->kind());
  node->operand()->visit(this);
}

void ASTPrinter::visitStringLiteralNode(StringLiteralNode *node) {
  string literal = node->literal();
  string result;
  for (size_t i = 0; i < literal.length(); ++i) {
    if (literal[i] == '\n') {
      result += "\\n";
    } else {
      result += literal[i];
    }
  }
  sstream << "'" << result << "'";
}

void ASTPrinter::visitDoubleLiteralNode(DoubleLiteralNode *node) { 
  sstream << node->literal();
} 

void ASTPrinter::visitIntLiteralNode(IntLiteralNode *node) {
  sstream << node->literal();
} 

void ASTPrinter::visitLoadNode(LoadNode *node) { 
  sstream << node->var()->name();
} 

void ASTPrinter::visitStoreNode(StoreNode *node) { 
  sstream << node->var()->name();
  sstream << separator << tokenOp(node->op()) << separator;
  node->value()->visit(this);
} 

void ASTPrinter::visitForNode(ForNode *node) { 
  sstream << "for (" << node->var()->name();
  sstream << separator << "in" << separator;
  node->inExpr()->visit(this);
  sstream << ")" << endline;

  node->body()->visit(this);
} 

void ASTPrinter::visitWhileNode(WhileNode *node) { 
  sstream << "while (";
  node->whileExpr()->visit(this);
  sstream << ")" << endline;

  if (node->loopBlock()) {
    node->loopBlock()->visit(this);
  }
} 

void ASTPrinter::visitIfNode(IfNode *node) { 
  sstream << "if" + separator + "(";
  node->ifExpr()->visit(this);
  sstream << ")";

  if (node->thenBlock()) {
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
      sstream  << endline << currentIndent << "else";
      node->thenBlock()->visit(this);
    }
  }
} 

void ASTPrinter::visitBlockNode(BlockNode *node) { 
  bool isTop = node->scope()->parent()->parent() == 0;
  bool needDoubleIndent = isFunctionBlock;
  isFunctionBlock = false;

  if (!isTop) {
    sstream << separator << "{" << endline;
    currentIndent += indent;
    if (needDoubleIndent) {
      currentIndent += indent;
    }
  }

  Scope::VarIterator iter(node->scope());
  while (iter.hasNext()) {
    AstVar* var = iter.next();
    sstream << currentIndent << typeToName(var->type());
    sstream << separator << var->name() << ";" << endline;
  }

  Scope::FunctionIterator funcIter(node->scope());
  while (funcIter.hasNext()) {
    sstream << currentIndent;
    AstFunction* func = funcIter.next();
    func->node()->visit(this);
    sstream << "" << endline;
  }

  for (uint32_t i = 0; i < node->nodes(); ++i) {
    sstream << currentIndent;
    node->nodeAt(i)->visit(this);
    if(!(node->nodeAt(i)->isIfNode()      ||
         node->nodeAt(i)->isWhileNode()   ||
         node->nodeAt(i)->isForNode()     ||
         node->nodeAt(i)->isFunctionNode()||
         node->nodeAt(i)->isNativeCallNode())){
      sstream << ";";
    }
    sstream << "" << endline;
  }

  if (!isTop) {
    currentIndent.resize(currentIndent.size() - indent.size());
    sstream << currentIndent << "}";
    if (needDoubleIndent) {
      currentIndent.resize(currentIndent.size() - indent.size());
    }    
  }
} 

void ASTPrinter::visitFunctionNode(FunctionNode *node) { 
  if (node->name() == AstFunction::top_name) {
    return visitBlockNode(node->body());
  }

  sstream << "function" << separator << typeToName(node->returnType());
  sstream << separator << node->name() << "(";

  for (size_t i = 0; i < node->parametersNumber(); ++i) {
    if (i > 0) {      
      sstream << "," + separator; 
    }
    sstream << typeToName(node->parameterType(i)) << separator << node->parameterName(i);
  }
  sstream << ")";

  if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
    node->body()->nodeAt(0)->visit(this);
    sstream << ";";
  } else {
    isFunctionBlock = true;
    node->body()->visit(this);
  }
} 

void ASTPrinter::visitReturnNode(ReturnNode *node) { 
  if (node->returnExpr()) {
    sstream << "return" + separator;
    return node->returnExpr()->visit(this);
  }
  sstream << "return";
} 

void ASTPrinter::visitCallNode(CallNode *node) { 
  sstream << node->name() << "(";
  for (size_t i = 0; i < node->parametersNumber(); ++i) {
    if (i > 0) {
      sstream << "," + separator;
    }
    node->parameterAt(i)->visit(this);
  }
  sstream << ")";
} 

void ASTPrinter::visitNativeCallNode(NativeCallNode * node) { 
    sstream << "native '" << node->nativeName() << "'";
} 

void ASTPrinter::visitPrintNode(PrintNode *node) { 
  sstream << "print(";
  for (size_t i = 0; i < node->operands(); ++i) {
    if (i > 0) {
      sstream << "," + separator;
    }
    node->operandAt(i)->visit(this);
  }
  sstream << ")";
} 
}
