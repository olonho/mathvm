#include "ast.h" 
#include "ast_printing.h"

#include <string>
#include <iostream>

using std::cout;
using std::endl;

#define DEBUG

namespace mathvm {
#define VISITOR_FUNCTION(type, name)                        \
  void AstDumper::visit##type(type* node) {}                \

FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION


#define VISITOR_FUNCTION(type, name)                        \
  void AstOffsetDumper::visit##type(type* node) {           \
    cout << std::string(offset, ' ') << name << endl;       \
    offset += 2;                                            \
    node->visitChildren(this);                              \
    offset -= 2;                                            \
  }                                                         \

FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION


Status* AstPrinterTranslator::translate(const string& program, Code* *code) {
  Parser parser;
  Status* status = parser.parseProgram(program);
  if (not status->isError()) {
#ifdef DEBUG
    AstOffsetDumper* dumper = new AstOffsetDumper();
#else
    AstPrettyPrinter* dumper = new AstPrettyPrinter();  
#endif
    parser.top()->node()->visitChildren(dumper);
    delete dumper;
  }

  return status;
}

void AstPrettyPrinter::printScope(Scope* scope) {
  Scope::VarIterator varIter(scope);
  while (varIter.hasNext()) {
    AstVar* var = varIter.next();
    printOffset();
    write(varTypeToText(var->type()));
    write(" ");
    write(var->name());
    writeLn(";");
  }

  Scope::FunctionIterator funIter(scope);
  while (funIter.hasNext()) {
    AstFunction* fun = funIter.next();
    printOffset();
    fun->node()->visit(this);
  }
}

std::string AstPrettyPrinter::escapeCharacters(const std::string& s) {
  std::string escaped = "";
  for (char c: s) {
    if (c == '\n')
        escaped += "\\n";
    else if (c == '\r')
        escaped += "\\r";
    else if (c == '\\')
        escaped += "\\\\";
    else if (c == '\t')
        escaped += "\\t";
    else
        escaped += c;
  }

  return escaped;
}

const char* AstPrettyPrinter::varTypeToText(const VarType type) const {
  switch (type) {
    case VT_VOID: return "void";
    case VT_DOUBLE: return "double";
    case VT_INT: return "int";
    case VT_STRING: return "string";
    default: return "UNKNOWN_TYPE";
  }
}

void AstPrettyPrinter::printOffset() {
  for (int i = 0; i < offset; i++) {
    write(" ");
  }
}

void AstPrettyPrinter::writeSemicolonAndNewline() {
  // Every statement is separated by ';' and a newline.
  // Trailing semicolon is not needed in case like
  // if (smth) {
  // }
  // We can distinguish it by looking at the last printed character:
  // if it is newline, then no semicolon is needed.
  if (lastPrintedChar != '\n') {
    write(";\n");
  }
}

void AstPrettyPrinter::write(const char* data) {
  for (int i = 0; data[i]; i++) {
    cout << data[i];
    lastPrintedChar = data[i];
  }
}

void AstPrettyPrinter::write(const string& data) {
  write(data.c_str());
}

template<class T>
void AstPrettyPrinter::write(T data) {
  write(std::to_string(data));
}

template<class T>
void AstPrettyPrinter::writeLn(T data) {
  write(data);
  write("\n");
}

const char* AstPrettyPrinter::tokenToText(const TokenKind kind) const {
#define ENUM_ELEM(t, s, p) case t: return s;
  switch (kind) {
    FOR_TOKENS(ENUM_ELEM)
  default:
    return "UNKNOWN_TOKEN";
  }
#undef ENUM_ELEM
}


void AstPrettyPrinter::visitBinaryOpNode(BinaryOpNode* node) {
  write("("); 
  node->left()->visit(this);
  write(" "); 
  write(tokenToText(node->kind()));
  write(" "); 
  node->right()->visit(this);
  write(")"); 
}
 
void AstPrettyPrinter::visitUnaryOpNode(UnaryOpNode* node) {
  write(tokenToText(node->kind()));
  node->operand()->visit(this);
}
 
void AstPrettyPrinter::visitStringLiteralNode(StringLiteralNode* node) {
  write("'");
  write(escapeCharacters(node->literal()));
  write("'");
}
 
void AstPrettyPrinter::visitDoubleLiteralNode(DoubleLiteralNode* node) {
  write(node->literal());
}
 
void AstPrettyPrinter::visitIntLiteralNode(IntLiteralNode* node) {
  write(node->literal());
}
 
void AstPrettyPrinter::visitLoadNode(LoadNode* node) {
  write(node->var()->name());
}
 
void AstPrettyPrinter::visitStoreNode(StoreNode* node) {
  write(node->var()->name());
  write(" ");
  write(tokenToText(node->op()));
  write(" ");
  node->value()->visit(this);
}
 
void AstPrettyPrinter::visitForNode(ForNode* node) {
  write("for (");
  write(node->var()->name());
  write(" in ");
  node->inExpr()->visit(this);
  writeLn(") {");

  increaseOffset();
  node->body()->visit(this);
  decreaseOffset();

  printOffset();
  writeLn("}");
}
 
void AstPrettyPrinter::visitWhileNode(WhileNode* node) {
  write("while (");
  node->whileExpr()->visit(this);
  writeLn(") {");

  increaseOffset();
  node->loopBlock()->visit(this);
  decreaseOffset();

  printOffset();
  writeLn("}");
}
 
void AstPrettyPrinter::visitIfNode(IfNode* node) {
  write("if (");
  node->ifExpr()->visit(this);
  writeLn(") {");

  increaseOffset();
  node->thenBlock()->visit(this);
  decreaseOffset();

  printOffset();
  writeLn("}");
  
  if (node->elseBlock()) {
    printOffset();
    writeLn("else {");

    increaseOffset();
    node->elseBlock()->visit(this);
    decreaseOffset();

    printOffset();
    writeLn("}");
  }
}
 
void AstPrettyPrinter::visitBlockNode(BlockNode* node) {
  printScope(node->scope());
  for (uint32_t i = 0; i < node->nodes(); i++) {
    printOffset();
    node->nodeAt(i)->visit(this);
    writeSemicolonAndNewline();
  }
}
 
void AstPrettyPrinter::visitFunctionNode(FunctionNode* node) {
  write("function ");
  write(varTypeToText(node->returnType()));
  write(" ");
  write(node->name());
  write("(");

  for (uint32_t i = 0; i < node->parametersNumber(); i++) {
    write(varTypeToText(node->parameterType(i)));
    write(" ");
    write(node->parameterName(i));
    if (i + 1 != node->parametersNumber()) {
      write(", ");
    }
  }

  write(") ");

  // Special case for NativeCallNode: it must be the single operation
  // in function body (not counting the return statement).
  if (node->body()->nodes() == 2 
          and node->body()->nodeAt(0)->isNativeCallNode()) {
    node->body()->nodeAt(0)->visit(this);
    writeSemicolonAndNewline();
    return;
  }

  writeLn("{");

  increaseOffset();
  node->body()->visit(this);
  decreaseOffset();

  printOffset();
  writeLn("}");
}
 
void AstPrettyPrinter::visitReturnNode(ReturnNode* node) {
  write("return");
  if (node->returnExpr()) {
    write(" ");
    node->returnExpr()->visit(this);
  }
}
 
void AstPrettyPrinter::visitCallNode(CallNode* node) {
  write(node->name());
  write("(");
  for (uint32_t i = 0; i < node->parametersNumber(); i++) {
    node->parameterAt(i)->visit(this);
    if (i + 1 != node->parametersNumber()) {
      write(", ");
    }
  }
  write(")");
}
 
void AstPrettyPrinter::visitNativeCallNode(NativeCallNode* node) {
  write("native ");
  write("'");
  write(node->nativeName());
  write("'");
}
 
void AstPrettyPrinter::visitPrintNode(PrintNode* node) {
  write("print(");
  for (uint32_t i = 0; i < node->operands(); i++) {
    node->operandAt(i)->visit(this);
    if (i + 1 != node->operands()) {
      write(", ");
    }
  }
  write(")");
}


}
