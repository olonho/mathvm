#include "mathvm.h"
#include "parser.h"
#include "ast.h"

#include <iostream>

using namespace mathvm;
using std::ostream;

class AstPrinterVisitor: public AstVisitor {
public:
    AstPrinterVisitor(ostream& out) :
        _out(out)
      , SPACE(" ")
      , OPEN_BR("(")
      , CLOSE_BR(")")
      , SEMI_COL(";")
      , NEW_LINE("\n")
      , QUOTE("\'")
      , COMMA(",")
      , OPEN_CBR("{")
      , CLOSE_CBR("}")
      , KW_NATIVE("native")
      , KW_FUN("function")
      , KW_IF("if")
      , KW_ELSE("else")
      , KW_FOR("for")
      , KW_WHILE("while")
      , KW_IN("in")
      , KW_RET("return")
      , KW_PRINT("print")
    {}

    void visitProgram(AstFunction* astFun) {
        visitBlockBody(astFun->node()->body());
    }

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
        _out << OPEN_BR;
        node->left()->visit(this);
        _out << CLOSE_BR << SPACE << tokenOp(node->kind()) << SPACE << OPEN_BR;
        node->right()->visit(this);
        _out << CLOSE_BR;
    }

    virtual void visitUnaryOpNode(UnaryOpNode* node) {
        _out << tokenOp(node->kind()) << OPEN_BR;
        node->operand()->visit(this);
        _out << CLOSE_BR;
    }

    virtual void visitStringLiteralNode(StringLiteralNode* node) {
        _out << QUOTE;
        string const& lit = node->literal();
        for (size_t i = 0; i != lit.size(); ++i) {
            switch (lit[i]) {
                case '\\' : _out << "\\\\"; break;
                case '\'' : _out << "\\'"; break;
                case '\n': _out << "\\n"; break;
                case '\r': _out << "\\r"; break;
                case '\t': _out << "\\t"; break;
                default  : _out << lit[i]; break;
            }
        }
        _out << QUOTE;
    }

    virtual void visitIntLiteralNode(IntLiteralNode* node) { _out << node->literal(); }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) { _out << node->literal(); }

    virtual void visitLoadNode(LoadNode* node) { _out << node->var()->name(); }

    virtual void visitStoreNode(StoreNode* node) {
        _out << node->var()->name() << SPACE << tokenOp(node->op()) << SPACE;
        node->value()->visit(this);
    }

    virtual void visitBlockNode(BlockNode* node) {
        _out << OPEN_CBR << NEW_LINE;
        visitBlockBody(node);
        _out << CLOSE_CBR;
    }

    virtual void visitNativeCallNode(NativeCallNode* node) {
        _out << KW_NATIVE << SPACE << QUOTE << node->nativeName() << QUOTE << SEMI_COL;
    }

    virtual void visitForNode(ForNode* node) {
        _out << KW_FOR << SPACE << OPEN_BR << node->var()->name() << SPACE << KW_IN << SPACE;
        node->inExpr()->visit(this);
        _out << CLOSE_BR << SPACE;
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode* node) {
        _out << KW_WHILE << SPACE << OPEN_BR;
        node->whileExpr()->visit(this);
        _out << CLOSE_BR << SPACE;
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode* node) {
        _out << KW_IF << SPACE << OPEN_BR;
        node->ifExpr()->visit(this);
        _out << CLOSE_BR << SPACE;
        node->thenBlock()->visit(this);
        if(node->elseBlock() != 0) {
            _out << SPACE << KW_ELSE << SPACE;
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitReturnNode(ReturnNode* node) {
        _out << KW_RET << SPACE;
        node->returnExpr()->visit(this);
    }

    virtual void visitFunctionNode(FunctionNode* node) {
        outFunHeader(node);
        _out << SPACE;
        if(node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->asNativeCallNode()->visit(this);
        } else {
            node->body()->visit(this);
        }
    }

    virtual void visitCallNode(CallNode* node) {
        _out << node->name() << SPACE << OPEN_BR;
        for(size_t i = 0; i != node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            if(i != node->parametersNumber() - 1) {
                _out << COMMA << SPACE;
            }
        }
        _out << CLOSE_BR;
    }

    virtual void visitPrintNode(PrintNode* node) {
        _out << KW_PRINT << SPACE << OPEN_BR;
        for(size_t i = 0; i != node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            if(i != node->operands() - 1) {
                _out << COMMA << SPACE;
            }
        }
        _out << CLOSE_BR;
    }

private:
    string varTypeToStr(VarType const& varType) {
        switch (varType) {
            case VT_VOID: return "void";
            case VT_INT: return "int";
            case VT_DOUBLE: return "double";
            case VT_STRING: return "string";
            case VT_INVALID: return "<invalid_type>";
        }
        return "<no_type>";
    }

    void visitBlockBody(BlockNode* node) {
        outVarDecls(node);
        outFunDefs(node);
        outExprs(node);
    }

    void outVarDecls(BlockNode* node) {
        Scope::VarIterator varIt(node->scope());
        while(varIt.hasNext()) {
            AstVar* var = varIt.next();
            _out << varTypeToStr(var->type()) << SPACE << var->name() << SEMI_COL << NEW_LINE;
        }
    }

    void outFunDefs(BlockNode* node) {
        Scope::FunctionIterator funIt(node->scope());
        while(funIt.hasNext()) {
            AstFunction* fun = funIt.next();
            fun->node()->visit(this);
            _out << NEW_LINE;
        }
    }

    void outExprs(BlockNode* node) {
        for(size_t i = 0; i != node->nodes(); ++i) {
            node->nodeAt(i)->visit(this);
            _out << SEMI_COL << NEW_LINE;
        }
    }

    string getFunRetType(FunctionNode* node) { return varTypeToStr(node->signature()[0].first); }

    string signElemToStr(SignatureElement const& se) { return varTypeToStr(se.first) + ' ' + se.second; }

    void outFunHeader(FunctionNode* node) {
        Signature const& signature = node->signature();
        _out << KW_FUN << SPACE << getFunRetType(node) << SPACE << node->name() << OPEN_BR;
        for(size_t i = 1; i != signature.size(); ++i) {
            _out << signElemToStr(signature[i]);
            if(i != signature.size() - 1) {
                _out << COMMA << SPACE;
            }
        }
        _out << CLOSE_BR;
    }

    ostream& _out;

    string const SPACE;
    string const OPEN_BR;
    string const CLOSE_BR;
    string const SEMI_COL;
    string const NEW_LINE;
    string const QUOTE;
    string const COMMA;
    string const OPEN_CBR;
    string const CLOSE_CBR;
    string const KW_NATIVE;
    string const KW_FUN;
    string const KW_IF;
    string const KW_ELSE;
    string const KW_FOR;
    string const KW_WHILE;
    string const KW_IN;
    string const KW_RET;
    string const KW_PRINT;
};

class AstPrinter : public Translator {
public:
    AstPrinter(ostream& os): _printerVisitor(os) {}

    virtual Status* translate(const string& program, Code* *code)  {
      Parser parser;
      Status* status = parser.parseProgram(program);
      if (status != 0 && status->isError()) {
          return status;
      }
      _printerVisitor.visitProgram(parser.top());
      return new Status();
    }
private:
    AstPrinterVisitor _printerVisitor;
};

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
      return new AstPrinter(std::cout);
    }
    return 0;
}
