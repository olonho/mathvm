#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "ast.h"

namespace mathvm {

class PrintVisitor : public AstVisitor {

public:
    PrintVisitor(ostream & out) : out(out), level(0) {}

    virtual void visitBinaryOpNode(BinaryOpNode *node) {
        out << '(';
        node->left()->visit(this);
        out << ' ' << token(node->kind()) << ' ';
        node->right()->visit(this);
        out << ')';
    }

    virtual void visitBlockNode(BlockNode *node) {
        if (node->nodes() && node->nodeAt(0)->isNativeCallNode()) {
            node->nodeAt(0)->visit(this);
            out << ';';
            return;
        }
        bool topLevel = !node->scope()->parent()->parent();
        if (!topLevel) {
            out << "{" << endl;
            ++level;
        }
        scope(node->scope());
        for (size_t i = 0; i < node->nodes(); ++i) {
            indent();
            AstNode *current_node =  node->nodeAt(i);
            current_node->visit(this);
            if (!current_node->isIfNode() && !current_node->isForNode()) {
                out << ';' << endl;
            }
        }
        if (!topLevel) {
            --level;
            indent();
            out << '}';
        }
    }

    virtual void visitCallNode(CallNode *node) {
        out << node->name() << '(';
        for (size_t i = 0; i < node->parametersNumber(); i++) {
            node->parameterAt(i)->visit(this);
            out << (i + 1 < node->parametersNumber() ? ", " : "");
        }
        out << ')';
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        out << showpoint << node->literal();
    }

    virtual void visitForNode(ForNode *node) {
        out << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        out << ") ";
        node ->body()->visit(this);
        out << endl;
    }

    virtual void visitFunctionNode(FunctionNode *node) {
        bool topLevel = node->name() == AstFunction::top_name;
        if (!topLevel) {
            out << "function " << type(node->returnType()) << ' ' << node->name() << '(';
            for (size_t i = 0; i < node->parametersNumber(); ++i) {
                out << type(node->parameterType(i)) << ' '
                    << node->parameterName(i)
                    << (i + 1 < node->parametersNumber() ? ", " : "");
            }
            out << ") ";
        }
        node->body()->visit(this);
        if (!topLevel) { out << endl << endl; }
    }

    virtual void visitIfNode(IfNode *node) {
        out << "if (";
        node->ifExpr()->visit(this);
        out << ") ";
        node ->thenBlock()->visit(this);
        if (node->elseBlock()) {
            out << " else ";
            node->elseBlock()->visit(this);
        }
        out << endl;
    }

    virtual void visitIntLiteralNode(IntLiteralNode *node) {
        out << node->literal();
    }

    virtual void visitLoadNode(LoadNode *node) {
        out << node->var()->name();
    }

    virtual void visitNativeCallNode(NativeCallNode *node) {
        out << "native '" << node->nativeName() << "'";
    }

    virtual void visitPrintNode(PrintNode *node) {
        out << "print(";
        for (size_t i = 0; i < node->operands(); i++) {
            node->operandAt(i)->visit(this);
            out << (i + 1 < node->operands() ? ", " : "");
        }
        out << ")";
    }

    virtual void visitReturnNode(ReturnNode *node) {
        out << "return";
        if (node->returnExpr()) {
            out << ' ';
            node->returnExpr()->visit(this);
        }
    }

    virtual void visitStoreNode(StoreNode *node) {
        out << node->var()->name() << ' ' << token(node->op()) << ' ';
        node->value()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node) {
        out << '\'' << escape(node->literal()) << '\'';
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node) {
        out << '(' << token(node->kind());
        node->operand()->visit(this);
        out << ')';
    }

    virtual void visitWhileNode(WhileNode *node) {
        out << "while (";
        node->whileExpr()->visit(this);
        out << ") ";
        node->loopBlock()->visit(this);
        out << endl;
    }

private:
    const static string types[];
    ostream & out;
    unsigned int level;

    void indent() {
        for (unsigned int i = 0; i < level; i++) {
            out << "    ";
        }
    }

    void scope(Scope *scope) {
        Scope::VarIterator vars(scope);
        while (vars.hasNext()) {
            indent();
            AstVar *var = vars.next();
            out << type(var->type()) << ' ' << var->name() << ';' << endl;
        }
        if (scope->variablesCount()) {
            out << endl;
        }
        Scope::FunctionIterator funs(scope);
        while (funs.hasNext()) {
            funs.next()->node()->visit(this);
        }
    }

    static string token(TokenKind kind) {
        return tokenOp(kind);
    }

    static string type(VarType type) {
        return types[type];
    }

    static string escape(const string & input) {
        string result;
        for (size_t i = 0; i < input.size(); ++i) {
            char c = input[i];
            switch (c) {
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\r"; break;
                default: result += c;
            }
        }
        return result;
    }
};

const string PrintVisitor::types[5] = {"<invalid>", "void", "double", "int", "string"};

class AstPrinter : public Translator {

public:
    virtual Status *translate(const string & program, Code **)  {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status && status->isError()) {
            return status;
        }
        PrintVisitor visitor(cout);
        parser.top()->node()->visit(&visitor);
        return new Status();
    }
};

Translator *Translator::create(const string & impl) {
    if (impl == "printer") {
        return new AstPrinter;
    } else {
        return NULL;
    }
}

}
