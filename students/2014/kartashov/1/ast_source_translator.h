#ifndef ASTm_source_TRANSLATOR_H__
#define ASTm_source_TRANSLATOR_H__

#include <sstream>

#include "ast.h"

using namespace mathvm;

class AstSourceTranslator: public AstVisitor {
  public:
    std::string source() {return m_source.str();}

    void start(AstFunction* top) {
      BlockNode* node = top->node()->body();
      declareVariables(node);

      for(size_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
      }
    }

    void visitStringLiteralNode(StringLiteralNode* node) {
      m_source << "\'";
      m_source << escaped(node->literal());
      m_source << "\'";
    }

    void visitIntLiteralNode(IntLiteralNode* node) {
      m_source << node->literal();
    }

    void visitDoubleLiteralNode(DoubleLiteralNode* node) {
      m_source << node->literal();
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
      newLine();
    ;}

    void visitLoadNode(LoadNode* node) {
      var(node->var());
    }

    void visitCallNode(CallNode* node) {
      m_source << node->name();
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
      newLine();
    }

    void visitFunctionNode(FunctionNode* node) {
      functionHeading(node);
      if (node->body()->nodes() == 1 && node->body()->nodeAt(0)->isNativeCallNode()) {
        native();
        space();
        leftParen();
        rightParen();
        newLine();
      } else {
        block(node->body());
        newLine();
      }
    }

    void visitIfNode(IfNode* node) {
      ifkw();
      condition(node->ifExpr());
      space();
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
      m_source << node->var()->name();
      space();
      inkw();
      space();
      node->inExpr()->visit(this);
      rightBracket();
      block(node->body());
      newLine();
    }

    void visitWhileNode(WhileNode* node) {
      whilekw();
      condition(node->whileExpr());
      block(node->loopBlock());
      newLine();
    }

    void visitReturnNode(ReturnNode* node) {
      returnkw();
      space();
      node->returnExpr()->visit(this);
      semiColon();
      newLine();
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
      leftBracket();
      arguments(node);
      rightBracket();
      space();
    }

    void condition(AstNode* node) {
      space();
      leftBracket();
      node->visit(this);
      rightBracket();
      space();
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
          argument(node->signature()[i]);
        }
      }
    }

    void var(const AstVar* astVar) {
      m_source << astVar->name();
    }

    void type(VarType varType) {
      switch (varType) {
        case VT_VOID: voidt(); break;
        case VT_DOUBLE: doublet(); break;
        case VT_INT: intt(); break;
        case VT_STRING: stringt(); break;
        case VT_INVALID: invalid(); break;
        default: unknownType();
      }
    }

    void op(TokenKind kind) {
      m_source << tokenOp(kind);
    }

    void block(BlockNode* node) {
      leftParen();
      in();
      newLine();
      declareVariables(node);
      for(size_t i = 0; i < node->nodes(); ++i) {
        indentedVisit(node->nodeAt(i));
      }
      out();
      indent();
      rightParen();
    }

    void declareVariable(AstVar* var) {
      type(var->type());
      space();
      m_source << var->name();
      semiColon();
      newLine();
    }

    void declareVariables(BlockNode* node) {
      Scope::VarIterator iter(node->scope());
      while(iter.hasNext()) {
        declareVariable(iter.next());
      }
    }

    void argument(std::pair<VarType, std::string> argumentPair) {
      type(argumentPair.first);
      space();
      m_source << argumentPair.second;
    }

    void assign() {m_source << "=";}

    void orop() {m_source << "||";}

    void andop() {m_source << "&&";}

    void notop() {m_source << "!";}

    void aor() {m_source << "|";}

    void aand() {m_source << "&";}

    void axor() {m_source << "^";}

    void eq() {m_source << "==";}

    void neq() {m_source << "!=";}

    void ge() {m_source << ">=";}

    void gt() {m_source << ">";}

    void lt() {m_source << "<";}

    void le() {m_source << "<=";}

    void range() {m_source << "..";}

    void add() {m_source << "+";}

    void sub() {m_source << "-";}

    void mul() {m_source << "*";}

    void div() {m_source << "/";}

    void mod() {m_source << "%";}

    void iset() {m_source << "+=";}

    void dset() {m_source << "-=";}

    void unknownKind() {m_source << "unknownKind";}

    void unknownType() {m_source << "unknownType";}

    void indent() {m_source << buildIndent(m_currentIndentLevel);}

    void in() {++m_currentIndentLevel;}

    void out() {--m_currentIndentLevel;}

    void inkw() {m_source << "in";}

    void space() {m_source << " ";}

    void semiColon() {m_source << ";";}

    void forkw() {m_source << "for";}

    void whilekw() {m_source << "while";}

    void ifkw() {m_source << "if";}

    void elsekw() {m_source << "else";}

    void returnkw() {m_source << "return";}

    void native() {m_source << "native";}

    void function() {m_source << "function";}

    void leftBracket() {m_source << "(";}

    void rightBracket() {m_source << ")";}

    void leftParen() {m_source << "{";}

    void rightParen() {m_source << "}";}

    void comma() {m_source << ",";}

    void newLine() {m_source << "\n";}

    void intt() {m_source << "int";}

    void doublet() {m_source << "double";}

    void stringt() {m_source << "string";}

    void voidt() {m_source << "void";}

    void invalid() {m_source << "invalid";}

    void print() {m_source << "print";}

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

    size_t m_currentIndentLevel;
    std::stringstream m_source;
};

#endif
