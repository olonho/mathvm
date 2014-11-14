#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "ast.h"

namespace mathvm {

class PrinterVisitor : public AstVisitor {
public:
    PrinterVisitor(ostream &stream) : out(stream) {}

    virtual void visitBinaryOpNode(BinaryOpNode *node)
    {
        out << "(";
        node->left()->visit(this);
        out << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
        out << ")";
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node)
    {
        out << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node)
    {
        out << "'" << escape(node->literal()) << "'";
    }
    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node)
    {
        out << showpoint << node->literal();
    }
    virtual void visitIntLiteralNode(IntLiteralNode *node) { out << node->literal(); }
    virtual void visitLoadNode(LoadNode *node) { out << node->var()->name(); }
    virtual void visitNativeCallNode(NativeCallNode *node)
    {
        out << " native '" << node->nativeName() << "';" << endl;
    }

    virtual void visitStoreNode(StoreNode *node)
    {
        out << node->var()->name() << " " << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    virtual void visitForNode(ForNode *node)
    {
        out << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        out << ") ";
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode *node)
    {
        out << "while (";
        node->whileExpr()->visit(this);
        out << ") ";
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode *node)
    {
        out << "if (";
        node->ifExpr()->visit(this);
        out << ") ";
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            out << "else ";
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitBlockNode(BlockNode *node)
    {
        bool topLevel = node->scope()->parent()->parent() == NULL;
        if (!topLevel) {
            out << "{" << endl;
        }

        printVars(node->scope());
        for (uint32_t i = 0; i < node->nodes(); i++) {
            AstNode *currentNode = node->nodeAt(i);
            currentNode->visit(this);
            if (currentNode->isLoadNode() || currentNode->isStoreNode() ||
                currentNode->isReturnNode() || currentNode->isCallNode() ||
                currentNode->isNativeCallNode() || currentNode->isPrintNode()) {
                out << ";" << endl;
            }
        }
        if (!topLevel) {
            out << "}" << endl;
        }
    }

    virtual void visitFunctionNode(FunctionNode *node)
    {
        if (node->name() != AstFunction::top_name) {
            out << "function " << typeToName(node->returnType()) << " " << node->name() << "(";

            for (uint32_t i = 0; i < node->parametersNumber(); i++) {
                out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
                if (i + 1 < node->parametersNumber()) {
                    out << ", ";
                }
            }
            out << ") ";
        }
        node->body()->visit(this);
    }

    virtual void visitReturnNode(ReturnNode *node)
    {
        out << "return ";
        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
        }
    }

    virtual void visitCallNode(CallNode *node)
    {
        out << node->name() << "(";

        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
            node->parameterAt(i)->visit(this);
            if (i + 1 < node->parametersNumber()) {
                out << ", ";
            }
        }
        out << ")";
    }

    virtual void visitPrintNode(PrintNode *node)
    {
        out << "print(";

        for (uint32_t i = 0; i < node->operands(); i++) {
            node->operandAt(i)->visit(this);
            if (i + 1 < node->operands()) {
                out << ", ";
            }
        }

        out << ")";
    }

private:
    ostream &out;

    void printVars(Scope *scope)
    {
        Scope::VarIterator iter(scope);
        while (iter.hasNext()) {
            AstVar *astVar = iter.next();
            out << typeToName(astVar->type()) << " " << astVar->name() << ";" << endl;
        }
    }

    string escape(string const &s)
    {
        string result;
        for (size_t i = 0; i < s.length(); ++i) {
            switch (s[i]) {
                case '\n':
                    result += "\\n";
                    break;
                case '\r':
                    result += "\\r";
                    break;
                case '\t':
                    result += "\\t";
                    break;
                default:
                    result += s[i];
                    break;
            }
        }
        return result;
    }
};

class AstPrinter : public Translator {
public:
    virtual Status *translate(const string &program, Code **)
    {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status && status->isError()) {
            return status;
        }
        PrinterVisitor visitor(cout);
        parser.top()->node()->visit(&visitor);
        return Status::Ok();
    }
};

Translator *Translator::create(const string &impl)
{
    if (impl == "printer") {
        return new AstPrinter();
    } else {
        return NULL;
    }
}
}