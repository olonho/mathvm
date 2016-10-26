#include "mathvm.h" // status 
#include "parser.h" // parser
#include "formatter.h"
#include <boost/algorithm/string.hpp>

using namespace std;

namespace mathvm {

const std::string Formatter::blockIndent = "  ";
const std::string Formatter::separator = " ";

void Formatter::rewindLastSeparator() {
  if (sstream.str().empty() || 
       !std::equal(separator.rbegin(), separator.rend(), sstream.str().rbegin()))
    return;
  sstream.seekp(-separator.size(), std::ios_base::end);
}

void Formatter::visitBinaryOpNode(BinaryOpNode* node) { 
  sstream << "(";
  node->left()->visit(this);
  sstream << tokenOp(node->kind()) << separator;
  node->right()->visit(this);
  rewindLastSeparator();
  sstream << ") ";
}

void Formatter::visitUnaryOpNode(UnaryOpNode *node) {
  sstream << tokenOp(node->kind());
  node->operand()->visit(this);
}

void Formatter::visitStringLiteralNode(StringLiteralNode *node) {
  string tmp = node->literal();
  boost::replace_all(tmp, "\n", "\\n");
  sstream << "'" << tmp << "'" << separator;
}

void Formatter::visitDoubleLiteralNode(DoubleLiteralNode *node) { 
  sstream << node->literal() << separator;
} 

void Formatter::visitIntLiteralNode(IntLiteralNode *node) {
  sstream << node->literal() << separator;
} 

void Formatter::visitLoadNode(LoadNode *node) { 
  sstream << node->var()->name() << separator;
} 

void Formatter::visitStoreNode(StoreNode *node) { 
  sstream << node->var()->name() << separator;
  sstream << tokenOp(node->op()) << separator;
  node->value()->visit(this);
} 

void Formatter::visitForNode(ForNode *node) { 
  sstream << "for (" << node->var()->name() 
    << separator << "in" << separator;
  node->inExpr()->visit(this);
  rewindLastSeparator();
  sstream << ")\n";

  node->body()->visit(this);
} 

void Formatter::visitWhileNode(WhileNode *node) { 
  sstream << "while (";
  node->whileExpr()->visit(this);
  rewindLastSeparator();
  sstream << ")\n";

  if (node->loopBlock()) {
    node->loopBlock()->visit(this);
  }
} 

void Formatter::visitIfNode(IfNode *node) { 
  sstream << "if (";
  node->ifExpr()->visit(this);
  rewindLastSeparator();
  sstream << ")\n";

  if (node->thenBlock()) {
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
      sstream << currentIndent << "else" << "\n";
      node->thenBlock()->visit(this);
    }
  }
} 

/**
 * This and only this function is responsible for putting semicolons 
 * after statements and declarations. This is ok since we have no semicolons
 * in any other parts, like for loops and the friends. 
 * Also this function prepares output so that it has correct indent when entering 
 * each \a nodes child. Yet multiline child nodes are responsible for each 
 * inner new line and indent.
 * 
 * Another invariant that should be held is that every node visiting which
 * is performed on children of this block, should leave the string stream
 * with extra separator written, so that it is removed here afterwards. 
 * This is done because frequently it's necessary to put separator after printed node,
 * so it is convenient to suggest there is always separator. 
 */
void Formatter::visitBlockNode(BlockNode *node) { 
  currentBlock = node;

  bool isToplevel = currentBlock->scope()->parent()->parent() == nullptr;

  if (!isToplevel) {
    sstream << currentIndent + "{\n";
    currentIndent += blockIndent;
  }

  Scope::VarIterator iter(node->scope());
  while (iter.hasNext()) {
    AstVar* var = iter.next();
    sstream << currentIndent << typeToName(var->type()) 
      << separator << var->name() << ";\n";
  }

  Scope::FunctionIterator funcIter(node->scope());
  while (funcIter.hasNext()) {
    sstream << currentIndent;
    AstFunction* func = funcIter.next();
    func->node()->visit(this);
    if (*std::prev(sstream.str().end(), 2) != '}')
      sstream << ";";
    sstream << "\n";
  }

  for (uint32_t i = 0; i < node->nodes(); i++) {
    sstream << currentIndent;
    node->nodeAt(i)->visit(this);
    rewindLastSeparator();

    if (*std::prev(sstream.str().end(), 2) != '}')
      sstream << ";";
    sstream << "\n";
  }

  if (!isToplevel) {
    assert(currentIndent.size() >= blockIndent.size());
    currentIndent.resize(currentIndent.size() 
      - blockIndent.size());

    sstream << currentIndent << "}" << separator;
  }
} 

void Formatter::visitFunctionNode(FunctionNode *node) { 
  if (node->name() == "<top>") {
    return visitBlockNode(node->body());
  }

  sstream << "function" << separator << typeToName(node->returnType()) 
    << separator << node->name() << "(";

  for (int i = 0; i < (int)node->parametersNumber() - 1; i++) {
    sstream << typeToName(node->parameterType(i)) 
      << separator <<  node->parameterName(i) << ", ";
  }

  if (node->parametersNumber()) {
    int idx = node->parametersNumber() - 1;
    sstream << typeToName(node->parameterType(idx))
      << separator << node->parameterName(idx);
  }

  NativeCallNode* firstBodyNode = node->body()->nodes() != 0 ? 
    dynamic_cast<NativeCallNode*>(node->body()->nodeAt(0)) : nullptr;

  if (!firstBodyNode) {
    sstream << ")\n";
    return visitBlockNode(node->body());
  } 

  sstream << ")" << separator << "native" << separator << "'"
    << firstBodyNode->nativeName() << "'";
} 

void Formatter::visitReturnNode(ReturnNode *node) { 
  if (node->returnExpr()) {
    sstream << "return" + separator;
    node->returnExpr()->visit(this);
  } else {
    sstream << "return" + separator;
  }
} 

void Formatter::visitCallNode(CallNode *node) { 
  sstream << node->name() << "(";
  for (int i = 0; i < (int)node->parametersNumber() - 1; i++) {
    node->parameterAt(i)->visit(this);
    rewindLastSeparator();
    sstream << "," + separator;
  }

  if (node->parametersNumber()) {
    node->parameterAt(node->parametersNumber() - 1)->visit(this);
    rewindLastSeparator();
  }

  sstream << ")" + separator;
} 

void Formatter::visitNativeCallNode(NativeCallNode *) { } 

void Formatter::visitPrintNode(PrintNode *node) { 
  sstream << "print(";
  for (int i = 0; i < (int)node->operands() - 1; i++) {
    node->operandAt(i)->visit(this);
    rewindLastSeparator();
    sstream << "," + separator;
  }


  if (node->operands() > 0) {
    node->operandAt(node->operands() - 1)->visit(this);
    rewindLastSeparator();
  }
  sstream << ")" + separator;
} 

void Formatter::reset() {
  sstream = stringstream{};
  sstream.clear();
  currentIndent = "";
  currentBlock = nullptr;
}

Status *Formatter::formatCode(std::string &outputSource, 
      const std::string &inputSource) {
  Parser parser;

  reset();
 
  Status *s = parser.parseProgram(inputSource); 

  if (s->isError()) {
      return s;
  }

  delete s;

  parser.top()->node()->visit(this);

  outputSource = sstream.str();

  return Status::Ok();
}

}
