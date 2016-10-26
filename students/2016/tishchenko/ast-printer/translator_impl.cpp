#include <sstream>
#include <fstream>
#include <string>
#include "translator_impl.h"
#include "ast.h"
#include "parser.h"
#include "visitors.h"

using namespace std;
using namespace mathvm;

string ReplaceAllOccurences(string s, char pattern, const string &replacement) {
    std::string::size_type n = 0;
    while ((n = s.find(pattern, n)) != std::string::npos) {
        s.replace(n, 1, replacement);
        n += replacement.size();
    }
    return s;
}

string EscapeNonPrintableChars(const string &s) {
    auto result = ReplaceAllOccurences(s, '\n', "\\n");
    result = ReplaceAllOccurences(result, '\t', "\\t");
    result = ReplaceAllOccurences(result, '\r', "\\r");
    result = ReplaceAllOccurences(result, '\\', "\\\\");
    result = ReplaceAllOccurences(result, '\'', "\\\'");
    return result;
}

class DocumentFormatter : public AstBaseVisitor {
public:
    DocumentFormatter(const string &documentName) : documentPath(documentName), blockIndent(-1) {

    }

    virtual void visitBinaryOpNode(BinaryOpNode *node) override {
        node->left()->visit(this);
        out << ' ' << tokenOp(node->kind()) << ' ';
        node->right()->visit(this);
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node) override {
        out << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node) override {
        out << '\'' << EscapeNonPrintableChars(node->literal()) << '\'';
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) override {
        out << node->literal();
    }

    virtual void visitIntLiteralNode(IntLiteralNode *node) override {
        out << node->literal();
    }

    virtual void visitLoadNode(LoadNode *node) override {
        out << node->var()->name();
    }

    virtual void visitStoreNode(StoreNode *node) override {
        out << node->var()->name() << ' ' << tokenOp(node->op()) << ' ';
        node->value()->visit(this);
    }

    virtual void visitForNode(ForNode *node) override {
        out << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        out << ") ";
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode *node) override {
        out << "while (";
        node->whileExpr()->visit(this);
        out << ") ";
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode *node) override {
        out << "if (";
        node->ifExpr()->visit(this);
        out << ") ";
        node->thenBlock()->visit(this);

        if (node->elseBlock()) {
            out << BlockIndentation() << "else ";
            node->elseBlock()->visit(this);
        }
    }

    bool shouldAddSemicolon(const AstNode *node) {
        return !(node->isForNode()
                 || node->isIfNode()
                 || node->isWhileNode()
                 || node->isBlockNode()
                 || node->isFunctionNode());
    }

    virtual void visitBlockNode(BlockNode *node) override {
        blockIndent++;
        if (blockIndent) {
            out << '{' << endl;
        }

        Scope::VarIterator varIt(node->scope());

        while (varIt.hasNext()) {
            auto *var = varIt.next();
            out << BlockIndentation() << typeToName(var->type()) << " " << var->name() << ';' << endl;
        }

        Scope::FunctionIterator funcIt(node->scope());

        while (funcIt.hasNext()) {
            auto *func = funcIt.next();
            out << BlockIndentation();
            func->node()->visit(this);
        }

        for (size_t i = 0; i < node->nodes(); ++i) {
            out << BlockIndentation();
            auto *innerNode = node->nodeAt(i);
            innerNode->visit(this);

            if (shouldAddSemicolon(innerNode)) {
                out << ';' << endl;
            }
        }

        --blockIndent;
        if (blockIndent > -1) {
            out << BlockIndentation() << '}' << endl;
        }
    }

    virtual void visitFunctionNode(FunctionNode *node) override {
        out << "function " << typeToName(node->returnType()) << " " << node->name() << "(";

        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
            if (i > 0) {
                out << ", ";
            }
            out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        }
        out << ") ";
        node->body()->visit(this);
    }

    virtual void visitReturnNode(ReturnNode *node) override {
        out << "return";
        if (node->returnExpr()) {
            out << ' ';
            node->returnExpr()->visit(this);
        }
    }

    virtual void visitCallNode(CallNode *node) override {
        out << node->name() << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
            if (i > 0) {
                out << ", ";
            }
            node->parameterAt(i)->visit(this);
        }
        out << ")";
    }

    virtual void visitNativeCallNode(NativeCallNode *node) override {
        out << "native '" << node->nativeName() << '\'' << endl;
    }

    virtual void visitPrintNode(PrintNode *node) override {
        out << "print(";
        for (uint32_t i = 0; i < node->operands(); i++) {
            if (i > 0)
                out << ", ";
            node->operandAt(i)->visit(this);
        }
        out << ")";
    }

    string FormatDocument() {
        Parser parser;
        ifstream infile(documentPath);
        auto file_contents = string(istreambuf_iterator<char>(infile), istreambuf_iterator<char>());
        parser.parseProgram(file_contents);
        parser.top()->node()->body()->visit(this);
        return out.str();
    }

private:

    string BlockIndentation() {
        if (blockIndent > 0) {
            return string(blockIndent * BLOCK_INDENTATION_LENGTH, ' ');
        }
        return "";
    }

    constexpr static uint BLOCK_INDENTATION_LENGTH = 4;

    string documentPath;
    int blockIndent;
    stringstream out;
};

string FormatDocument(const string &inputPath) {
    DocumentFormatter formatter(inputPath);
    return formatter.FormatDocument();
}