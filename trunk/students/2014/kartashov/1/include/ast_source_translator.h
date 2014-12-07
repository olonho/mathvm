#ifndef ASTSource_TRANSLATOR_H__
#define ASTSource_TRANSLATOR_H__

#include <sstream>

#include "ast.h"

using namespace mathvm;

class AstSourceTranslator: public AstVisitor {
  public:
    std::string source() {return mSource.str();}

    void visitTop(AstFunction* top) {
      cleanBlock(top->node()->body());
    }

    void visitAstFunction(AstFunction* astFunction) {
      astFunction->node()->visit(this);
    }

    void visitStringLiteralNode(StringLiteralNode* node) {
      quote();
      mSource << escaped(node->literal());
      quote();
    }

    void visitIntLiteralNode(IntLiteralNode* node) {
      mSource << node->literal();
    }

    void visitDoubleLiteralNode(DoubleLiteralNode* node) {
      mSource << node->literal();
    }

    void visitBinaryOpNode(BinaryOpNode* node) {
      leftBracket();
      node->left()->visit(this);
      space();
      op(node->kind());
      space();
      node->right()->visit(this);
      rightBracket();
    }

    void visitUnaryOpNode(UnaryOpNode* node) {
      op(node->kind());
      node->operand()->visit(this);
    }

    void visitStoreNode(StoreNode* node) {
      var(node->var());
      space();
      op(node->op());
      space();
      node->value()->visit(this);
      semiColon();
    }

    void visitLoadNode(LoadNode* node) {
      var(node->var());
    }

    void visitCallNode(CallNode* node) {
      mSource << node->name();
      leftBracket();
      if (node->parametersNumber() >= 1) {
        node->parameterAt(0)->visit(this);
        for (size_t i = 1; i < node->parametersNumber(); ++i) {
          comma();
          space();
          node->parameterAt(i)->visit(this);
        }
      }
      rightBracket();
    }

    void visitPrintNode(PrintNode* node) {
      print();
      leftBracket();
      if (node->operands() >= 1) {
        node->operandAt(0)->visit(this);
        for (size_t i = 1; i < node->operands(); ++i) {
          comma();
          space();
          node->operandAt(i)->visit(this);
        }
      }
      rightBracket();
      semiColon();
    }

    void visitFunctionNode(FunctionNode* node) {
      functionHeading(node);
      if (node->body()->nodeAt(0)->isNativeCallNode()) {
        NativeCallNode* nativeCall = node->body()->nodeAt(0)->asNativeCallNode();
        space();
        native();
        space();
        quote();
        mSource << nativeCall->nativeName();
        quote();
        semiColon();
      } else {
        block(node->body());
      }
      newLine();
    }

    void visitIfNode(IfNode* node) {
      ifkw();
      condition(node->ifExpr());
      block(node->thenBlock());
      if (node->elseBlock()) {
        space();
        elsekw();
        space();
        block(node->elseBlock());
      }
      newLine();
    }

    void visitForNode(ForNode* node) {
      forkw();
      space();
      leftBracket();
      mSource << node->var()->name();
      space();
      inkw();
      space();
      node->inExpr()->visit(this);
      rightBracket();
      block(node->body());
    }

    void visitWhileNode(WhileNode* node) {
      whilekw();
      condition(node->whileExpr());
      block(node->loopBlock());
    }

    void visitReturnNode(ReturnNode* node) {
      returnkw();
      if (node->returnExpr()) {
        space();
        node->returnExpr()->visit(this);
      }
      semiColon();
    }

  private:
    void indentedVisit(AstNode* node) {
      indent();
      node->visit(this);
    }

    std::string buildIndent(size_t level) const {
      return std::string(level, '\t');
    }

    void functionHeading(FunctionNode* node) {
      function();
      space();
      returnType(node);
      space();
      mSource << node->name();
      leftBracket();
      arguments(node);
      rightBracket();
    }

    void condition(AstNode* node) {
      space();
      leftBracket();
      node->visit(this);
      rightBracket();
    }

    void returnType(FunctionNode* node) {
      type(node->returnType());
    }

    void arguments(FunctionNode* node) {
      if (node->parametersNumber() >= 1) {
          argument(node->signature()[1]);
        for (size_t i = 1; i < node->parametersNumber(); ++i) {
          comma();
          space();
          argument(node->signature()[i + 1]);
        }
      }
    }

    void var(const AstVar* astVar) {
      mSource << astVar->name();
    }

    void type(VarType varType) {
      mSource << typeToName(varType);
    }

    void op(TokenKind kind) {
      mSource << tokenOp(kind);
    }

    void block(BlockNode* node) {
      space();
      leftParen();
      in();
      newLine();
      cleanBlock(node);
      out();
      indent();
      rightParen();
    }

    void cleanBlock(BlockNode* node) {
      declareVariables(node);
      declareFunctions(node);
      for(size_t i = 0; i < node->nodes(); ++i) {
        indentedVisit(node->nodeAt(i));
        if (node->nodeAt(i)->isCallNode()) {
          semiColon();
        }
        newLine();
      }
    }

    void declareVariable(AstVar* var) {
      indent();
      type(var->type());
      space();
      mSource << var->name();
      semiColon();
      newLine();
    }

    void declareVariables(BlockNode* node) {
      Scope::VarIterator iter(node->scope());
      while(iter.hasNext()) {
        declareVariable(iter.next());
      }
    }

    void declareFunctions(BlockNode* node) {
      Scope::FunctionIterator iter(node->scope());
      while (iter.hasNext()) {
        visitAstFunction(iter.next());
      }
    }

    void argument(std::pair<VarType, std::string> argumentPair) {
      type(argumentPair.first);
      space();
      mSource << argumentPair.second;
    }

    bool isTopLevel() {
      return mCurrentIndentLevel == 0;
    }

    void assign() {mSource << "=";}

    void orop() {mSource << "||";}

    void andop() {mSource << "&&";}

    void notop() {mSource << "!";}

    void aor() {mSource << "|";}

    void aand() {mSource << "&";}

    void axor() {mSource << "^";}

    void eq() {mSource << "==";}

    void neq() {mSource << "!=";}

    void ge() {mSource << ">=";}

    void gt() {mSource << ">";}

    void lt() {mSource << "<";}

    void le() {mSource << "<=";}

    void range() {mSource << "..";}

    void add() {mSource << "+";}

    void sub() {mSource << "-";}

    void mul() {mSource << "*";}

    void div() {mSource << "/";}

    void mod() {mSource << "%";}

    void iset() {mSource << "+=";}

    void dset() {mSource << "-=";}

    void unknownKind() {mSource << "unknownKind";}

    void unknownType() {mSource << "unknownType";}

    void indent() {mSource << buildIndent(mCurrentIndentLevel);}

    void in() {++mCurrentIndentLevel;}

    void out() {--mCurrentIndentLevel;}

    void inkw() {mSource << "in";}

    void space() {mSource << " ";}

    void semiColon() {mSource << ";";}

    void forkw() {mSource << "for";}

    void whilekw() {mSource << "while";}

    void ifkw() {mSource << "if";}

    void elsekw() {mSource << "else";}

    void returnkw() {mSource << "return";}

    void native() {mSource << "native";}

    void function() {mSource << "function";}

    void leftBracket() {mSource << "(";}

    void rightBracket() {mSource << ")";}

    void leftParen() {mSource << "{";}

    void rightParen() {mSource << "}";}

    void comma() {mSource << ",";}

    void quote() {mSource << "\'";}

    void newLine() {mSource << "\n";}

    void intt() {mSource << "int";}

    void doublet() {mSource << "double";}

    void stringt() {mSource << "string";}

    void voidt() {mSource << "void";}

    void invalid() {mSource << "invalid";}

    void print() {mSource << "print";}

    std::string escaped(std::string const& raw) {
      std::string result;
      for (size_t i = 0; i < raw.size(); ++i) {
        switch (raw[i]) {
          case '\n': result += "\\n"; break;
          case '\t': result += "\\t"; break;
          case '\r': result += "\\r"; break;
          case '\\': result += "\\\\"; break;
          case '\'': result += "\\'"; break;
          default: result += raw[i];
        }
      }
      return result;
    }

    size_t mCurrentIndentLevel;
    std::stringstream mSource;
};

#endif
