#include "printer.h"

static std::string escapeCharacter(char ch) {
  switch (ch) {
    case '\'':  
      return "\\'";
    case '\"':  
      return "\\'";
    case '\?':  
      return "\\?";
    case '\\':  
      return "\\";
    case '\a':  
      return "\\a";
    case '\b':  
      return "\\b";
    case '\f':  
      return "\\f";
    case '\n':  
      return "\\n";
    case '\r':  
      return "\\r";
    case '\t':  
      return "\\t";
    case '\v': 
      return "\\v";
    default:
      return std::string();
  }
}

void mathvm::Printer::visitBinaryOpNode(BinaryOpNode* node) {
  _out << LPAREN;
  node->left()->visit(this);

  _out << SEPARATOR;
  printTokenOp(node->kind());
  _out << SEPARATOR;

  node->right()->visit(this);
  _out << RPAREN;
}

void mathvm::Printer::visitUnaryOpNode(UnaryOpNode* node) {
  _out << LPAREN << tokenOp(node->kind());
  node->visitChildren(this);
  _out << RPAREN;
}

void mathvm::Printer::visitStringLiteralNode(StringLiteralNode* node) {
  const std::string& literal = node->literal();
  _out << QUOTE;
  for (char ch : literal) {
    _out << escapeCharacter(ch);
  }
  _out << QUOTE;
}

void mathvm::Printer::visitDoubleLiteralNode(DoubleLiteralNode* node) {
  _out << node->literal();
}

void mathvm::Printer::visitIntLiteralNode(IntLiteralNode* node) {
  _out << node->literal();
}

void mathvm::Printer::visitLoadNode(LoadNode* node) {
  _out << node->var()->name();
}

void mathvm::Printer::visitStoreNode(StoreNode* node) {
  _out << node->var()->name() << SEPARATOR;
  printTokenOp(node->op());
  _out << SEPARATOR;
  node->visitChildren(this);
  _out << SEMICOLON;
}

void mathvm::Printer::visitForNode(ForNode* node) {
  _out << "for" << SEPARATOR << LPAREN << node->var()->name()
       << SEPARATOR << "in" << SEPARATOR;
  node->inExpr()->visit(this);
  
  _out << RPAREN << SEPARATOR;
  node->body()->visit(this);
}

void mathvm::Printer::visitWhileNode(WhileNode* node) {
  _out << "while" << SEPARATOR << LPAREN;

  node->whileExpr()->visit(this);
  _out << RPAREN << SEPARATOR;
  node->loopBlock()->visit(this);
}

void mathvm::Printer::visitIfNode(IfNode* node) {
  _out << "if"<< SEPARATOR << LPAREN;
  node->ifExpr()->visit(this);
  _out << RPAREN << SEPARATOR;

  node->thenBlock()->visit(this);
  BlockNode* elseBlock = node->elseBlock();

  if(elseBlock) {
    _out << SEPARATOR << "else" << SEPARATOR;
    elseBlock->visit(this);
  }
}

void mathvm::Printer::visitBlockNode(BlockNode* node) {
  ++_scopeLevel;
  if(_scopeLevel - 1 != 0) {
    _out << LBRACE;
    newLine();
  }

  Scope::VarIterator variableIterator(node->scope());
  Scope::FunctionIterator functionIterator(node->scope());
  uint32_t childCount = node->nodes();

  while (variableIterator.hasNext()) {
    AstVar* variable = variableIterator.next();
    printAstVar(variable);
    if(variableIterator.hasNext() || functionIterator.hasNext() || childCount > 0) {
      newLine();
    }
  }

  while (functionIterator.hasNext()) {
    AstFunction* function = functionIterator.next();
    visitFunctionNode(function->node());
    if(functionIterator.hasNext() || childCount > 0) {
      newLine();
    }
  }

  for(uint32_t i = 0; i < childCount; ++i) {
    node->nodeAt(i)->visit(this);
    if(i + 1 < childCount) {
      newLine();
    }
  }

  --_scopeLevel;
  newLine();
  if (_scopeLevel != 0) {
    _out << RBRACE;
  }
}

void mathvm::Printer::visitFunctionNode(FunctionNode* node) {
  _out << "function" << SEPARATOR;
  _out << typeToName(node->returnType()) << SEPARATOR << node->name() << LPAREN;
  uint32_t parametersCount = node->parametersNumber();
  for(uint32_t i = 0; i < parametersCount; ++i) {
    _out << typeToName(node->parameterType(i)) << SEPARATOR << node->parameterName(i);
    if(i + 1 < parametersCount) {
      _out << COMMA << SEPARATOR;
    }
  }

  _out << RPAREN << SEPARATOR;


  if (isNativeCallNode(node)) {
    NativeCallNode* nativeCall = dynamic_cast<NativeCallNode*>(node->body()->nodeAt(0));
    visitNativeCallNode(nativeCall);
  } else {
    node->body()->visit(this);
  }
}

void mathvm::Printer::visitReturnNode(ReturnNode* node) {
  _out << "return" << SEPARATOR;
  AstNode* expr = node->returnExpr();
  if(expr) {
    expr->visit(this);
  }
  _out << SEMICOLON;
}

void mathvm::Printer::visitCallNode(CallNode* node) {
  _out << node->name() << LPAREN;
  uint32_t argCount = node->parametersNumber();
  for(uint32_t i = 0; i < argCount; ++i) {
    node->parameterAt(i)->visit(this);
    if(i + 1 != argCount) {
      _out << COMMA << SEPARATOR;
    }
  }

  _out << RPAREN;
}

void mathvm::Printer::visitNativeCallNode(NativeCallNode* node) {
  _out << "native " << QUOTE << node->nativeName() << QUOTE << SEMICOLON;
}

void mathvm::Printer::visitPrintNode(PrintNode* node) {
  _out << "print" << LPAREN;
  uint32_t operandCount = node->operands();
  for(uint32_t i = 0; i < operandCount; ++i) {
    node->operandAt(i)->visit(this);
    if(i + 1 != operandCount) {
      _out << COMMA << SEPARATOR;
    }
  }

  _out << RPAREN << SEMICOLON;
}

void mathvm::Printer::newLine() {
  _out << std::endl;
  for (uint32_t i = 1; i < _scopeLevel; ++i) {
    _out << BLOCK_INDENT;
  }
}

void mathvm::Printer::printAstVar(const AstVar* var) {
  _out << typeToName(var->type()) << SEPARATOR << var->name() << SEMICOLON;
}

void mathvm::Printer::printTokenOp(TokenKind op) {
  _out << tokenOp(op);
}

bool mathvm::Printer::isNativeCallNode(FunctionNode* node) {
  if (node->body()->nodes() == 2) {
    NativeCallNode* nativeCall = dynamic_cast<NativeCallNode*>(node->body()->nodeAt(0));
    return nativeCall != nullptr;
  }

  return false;
}
