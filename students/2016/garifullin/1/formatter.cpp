#include "mathvm.h"
#include "parser.h"
#include "formatter.h"

namespace mathvm {

    using namespace std;

    string convert(char c){
        switch(c){
            case '\n': return "\\n";
            case '\r': return "\\r";
            case '\\': return "\\\\";
            case '\'': return "\\'";
            case '\t': return "\\t";
            default: return c + "";
        }
    }

    string stringToLiteral(const string &in) {
        string result = "";
        for (char c : in){
            result += convert(c);
        }
        return result;
    }

    string FormatVisitor::indent() {
        return string(currentIndent * 2 - 2, ' ');
    }

    void FormatVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        out << "(";
        node->left()->visit(this);
        out << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
        out << ")";
    }

    void FormatVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        out << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    void FormatVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        out << "'" << stringToLiteral(node->literal()) << "'";
    }

    void FormatVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        out << node->literal();
    }

    void FormatVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        out << node->literal();
    }

    void FormatVisitor::visitLoadNode(LoadNode *node) {
        out << node->var()->name();
    }

    void FormatVisitor::visitStoreNode(StoreNode *node) {
        out << node->var()->name() << " "
            << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    void FormatVisitor::visitForNode(ForNode *node) {
        out << "for (" << node->var()->name()
            << " in ";
        node->inExpr()->visit(this);
        out << ") ";
        node->body()->visit(this);
    }

    void FormatVisitor::visitWhileNode(WhileNode *node) {
        out << "while (";
        node->whileExpr()->visit(this);
        out << ") ";
        node->loopBlock()->visit(this);
    }

    void FormatVisitor::visitIfNode(IfNode *node) {
        out << "if (";
        node->ifExpr()->visit(this);
        out << ") ";
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            out << indent() << "else ";
            node->thenBlock()->visit(this);
        }
    }

    void FormatVisitor::visitBlockNode(BlockNode *node) {
        if (currentIndent > 0) {
            out << "{\n";
        }
        ++currentIndent;

        Scope::VarIterator varIter(node->scope());

        while (varIter.hasNext()) {
            AstVar *var = varIter.next();
            out << indent() << typeToName(var->type()) << " " << var->name() << ";\n";
        }

        Scope::FunctionIterator funcIter(node->scope());

        while (funcIter.hasNext()) {
            AstFunction *func = funcIter.next();
            out << indent();
            func->node()->visit(this);
        }

        for (size_t i = 0; i < node->nodes(); ++i) {
            out << indent();
            AstNode *currenNode = node->nodeAt(i);
            currenNode->visit(this);

            if (!(currenNode->isForNode() || currenNode->isIfNode() || currenNode->isWhileNode() ||
                  currenNode->isBlockNode() || currenNode->isFunctionNode())) {
                out << ";\n";
            }
        }

        --currentIndent;
        if (currentIndent > 0) {
            out << indent() << "}\n";
        }

    }

    void FormatVisitor::visitFunctionNode(FunctionNode *node) {
        out << "function " << typeToName(node->returnType())
            << " " << node->name() << "(";

        for (size_t i = 0; i < node->parametersNumber(); i++) {
            if (i > 0)
                out << ", ";
            out << typeToName(node->parameterType(i))
                << " " << node->parameterName(i);
        }
        out << ") ";

        if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->visit(this);
        } else {
            ++currentIndent;
            node->body()->visit(this);
            --currentIndent;
        }

    }

    void FormatVisitor::visitReturnNode(ReturnNode *node) {
        out << "return";
        if (node->returnExpr()) {
            out << " ";
            node->returnExpr()->visit(this);
        }
    }

    void FormatVisitor::visitCallNode(CallNode *node) {
        out << node->name() << "(";
        for (size_t i = 0; i < node->parametersNumber(); i++) {
            if (i > 0)
                out << ", ";
            node->parameterAt(i)->visit(this);
        }

        out << ")";
    }

    void FormatVisitor::visitNativeCallNode(NativeCallNode *node) {
        out << "native '" << node->nativeName() << "'\n";
    }

    void FormatVisitor::visitPrintNode(PrintNode *node) {
        out << "print(";
        for (size_t i = 0; i < node->operands(); i++) {
            if (i > 0)
                out << ", ";
            node->operandAt(i)->visit(this);
        }
        out << ")";
    }

    Status *Formatter::formatCode(std::string &input, std::string &output) {
        Parser parser;
        stringstream out;
        FormatVisitor fmt(out);

        Status *s = parser.parseProgram(input);

        if (s->isError()) {
            return s;
        }

        parser.top()->node()->body()->visit(&fmt);

        output = out.str();

        delete s;
        return Status::Ok();
    }

}
