#include <iostream>
#include <fstream>
#include <sstream>
#include "parser.h"
#include "mathvm.h"
#include "visitors.h"

using namespace std;
using namespace mathvm;

class PrintVisitor : public AstVisitor {
private:
    stringstream _out{};
    int32_t indentCount = 0;
public:
    PrintVisitor() {
    }

    virtual ~PrintVisitor() {}

    virtual void visitBinaryOpNode(BinaryOpNode *node) {
        node->left()->visit(this);
        _out << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node) {
        _out << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node) {
        _out << "'";
        for (auto c: node->literal()) {
            _out << escapeCharacter(c);
        }
        _out << "'";
    }

    static string escapeCharacter(char ch) {
        switch (ch) {
            case '\'':
                return "\\'";
            case '\"':
                return "\\'";
            case '\n':
                return "\\n";
            case '\r':
                return "\\r";
            case '\t':
                return "\\t";
            default:
                return string(1, ch);
        }
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        _out << node->literal();
    }

    virtual void visitIntLiteralNode(IntLiteralNode *node) {
        _out << node->literal();
    }

    virtual void visitLoadNode(LoadNode *node) {
        _out << node->var()->name();
    }

    virtual void visitStoreNode(StoreNode *node) {
        _out << node->var()->name() << " ";
        _out << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    virtual void visitBlockNode(BlockNode *node) {
        if (indentCount != 0) {
            _out << " {\n";
        }
        ++indentCount;

        Scope::VarIterator varIterator{node->scope()};
        while (varIterator.hasNext()) {
            auto *var = varIterator.next();
            addIndent();
            _out << typeToName(var->type()) << " " << var->name() << ";" << "\n";
        }


        Scope::FunctionIterator functionIterator{node->scope()};
        while (functionIterator.hasNext()) {
            auto *var = functionIterator.next();
            addIndent();
            visitFunctionNode(var->node());
        }

        uint32_t nodeCount = node->nodes();
        for (uint32_t i = 0; i < nodeCount; ++i) {
            addIndent();
            auto *child = node->nodeAt(i);
            child->visit(this);
            if (!(child->isBlockNode() || child->isIfNode() || child->isForNode()
                  || child->isWhileNode()))
                _out << ";" << "\n";
        }
        --indentCount;
        if (indentCount != 0) {
            addIndent();
            _out << "}\n";
        }
    }

    virtual void visitNativeCallNode(NativeCallNode *node) {
        _out << "native '" << node->nativeName() << "';";
    }

    virtual void visitForNode(ForNode *node) {
        _out << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        _out << ")";
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode *node) {
        _out << "while (";
        node->whileExpr()->visit(this);
        _out << ")";
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode *node) {
        _out << "if (";
        node->ifExpr()->visit(this);
        _out << ")";
        node->thenBlock()->visit(this);
        if (node->elseBlock() != 0) {
            addIndent();
            _out << "else\n";
            for (int32_t j = 0; j < 3 * (indentCount - 1) - 1; ++j) {
                _out << " ";
            }
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitReturnNode(ReturnNode *node) {
        _out << "return ";
        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
        }
    }

    virtual void visitFunctionNode(FunctionNode *node) {
        _out << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
        uint32_t parameterCount = node->parametersNumber();
        for (uint32_t i = 0; i < parameterCount; ++i) {
            if (i != 0) {
                _out << ", ";
            }
            _out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        }
        _out << ")";

        if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->visit(this);
        } else {
            node->body()->visit(this);
        }


    }

    virtual void visitCallNode(CallNode *node) {
        _out << node->name() << "(";
        uint32_t parameterCount = node->parametersNumber();
        for (uint32_t i = 0; i < parameterCount; ++i) {
            if (i != 0) {
                _out << ", ";
            }
            node->parameterAt(i)->visit(this);
        }
        _out << ")";
    }

    virtual void visitPrintNode(PrintNode *node) {
        _out << "print(";
        uint32_t operandCount = node->operands();
        for (uint32_t i = 0; i < operandCount; ++i) {
            if (i != 0) {
                _out << ", ";
            }
            node->operandAt(i)->visit(this);
        }
        _out << ")";
    }

    string toString() {
        return _out.str();
    }

private:
    void addIndent() {
        for (int32_t j = 0; j < indentCount - 1; ++j) {
            _out << "   ";
        }
    }
};


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "No file specified!" << std::endl;
        return 0;
    }

    string fileName = argv[1];

    ifstream inputFileStream{fileName};

    stringstream stringStream;
    stringStream << inputFileStream.rdbuf();

    string programText = stringStream.str();


    Parser parser{};
    Status *status = parser.parseProgram(programText);

    if (status->isError()) {
        cout << status->getError() << endl;
        return 0;
    }

    delete status;

    PrintVisitor printVisitor;
    parser.top()->node()->visitChildren(&printVisitor);
    cout << printVisitor.toString();

}


