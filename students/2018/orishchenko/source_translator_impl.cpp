#include "source_translator_impl.h"

namespace mathvm {

    Status *SourceTranslatorImpl::translate(const string &program, Code **code) {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status->isError()) {
            std::cout << "Error in parsing \n";
        } else {
            auto *visitor = new MyVisitor();
            parser.top()->node()->body()->visit(visitor);
            delete visitor;
        }
        return status;
    }

    void MyVisitor::visitForNode(ForNode *node) {
        cout << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        cout << ") "<< "{\n";

        indention += INDENT;
        node->body()->visit(this);
        indention -= INDENT;

        printIndention();
        cout << "}\n";
    }


    void MyVisitor::visitPrintNode(PrintNode *node) {
        cout << "print(";

        if (node->operands() == 0) {
            cout << ")";
            return;
        }

        if (node->operands() > 0)
            node->operandAt(0)->visit(this);

        for (uint32_t i = 1; i < node->operands(); ++i) {
            cout << ", ";
            node->operandAt(i)->visit(this);
        }

        cout << ")";
    }


    void MyVisitor::visitLoadNode(LoadNode *node) {
        cout << node->var()->name();
    }


    void MyVisitor::visitIfNode(IfNode *node) {
        cout << "if (";
        node->ifExpr()->visit(this);
        cout << ") {\n";

        indention += INDENT;
        node->thenBlock()->visit(this);
        indention -= INDENT;

        if (node->elseBlock()) {
            indention += INDENT;
            printIndention();
            cout << "} \nelse { \n";
            indention -= INDENT;
            node->elseBlock()->visit(this);
        }

        printIndention();
        cout << "}\n";
    }


    void MyVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        std::cout << "(";
        node->left()->visit(this);

        cout << " ";
        std::cout << tokenOp(node->kind()) << " ";

        node->right()->visit(this);
        std::cout << ")";
    }


    void MyVisitor::visitCallNode(CallNode *node) {
        cout << node->name() << "(";

        if (node->parametersNumber() == 0) {
            cout << ")";
            return;
        }

        if (node->parametersNumber() > 0)
            node->parameterAt(0)->visit(this);

        for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
            cout << ", ";
            node->parameterAt(i)->visit(this);
        }

        cout << ")";
    }


    void MyVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        cout << to_string(node->literal());
    }


    void MyVisitor::visitStoreNode(StoreNode *node) {
        cout << node->var()->name() << " ";
        cout << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }


    void MyVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        cout << "'";
        string str;
        for (char i : node->literal()) {
            switch (i) {
                case '\n':
                    str += "\\n";
                    break;
                case '\r':
                    str += "\\r";
                    break;
                case '\t':
                    str += "\\t";
                    break;
                case '\\':
                    str += "\\\\";
                    break;
                default:
                    str += i;
            }
        }
        cout << str << "'";
    }


    void MyVisitor::visitWhileNode(WhileNode *node) {
        cout << "while (";
        node->whileExpr()->visit(this);
        cout << ") {\n";

        indention += INDENT;
        node->loopBlock()->visit(this);
        indention -= INDENT;

        printIndention();
        cout << "}\n";
    }


    void MyVisitor::visitBlockNode(BlockNode *node) {

        Scope *scope = node->scope();

        Scope::VarIterator it(scope);
        while (it.hasNext()) {
            printIndention();
            auto item = it.next();
            std::cout << typeToName(item->type()) << " ";
            cout << item->name() << ";\n";
        }

        Scope::FunctionIterator funit(scope);
        while (funit.hasNext()) {
            auto fun = funit.next();
            fun->node()->visit(this);
        }

        for (uint32_t i = 0; i < node->nodes(); ++i) {
            printIndention();
            auto stmt = node->nodeAt(i);
            stmt->visit(this);
            cout << ";\n";
        }
    }


    void MyVisitor::visitFunctionNode(FunctionNode *node) {
        printIndention();

        cout << "function " << typeToName(node->returnType()) << " " << node->name() << "(";

        if (node->parametersNumber() > 0)
            cout << typeToName(node->parameterType(0)) << " " << node->parameterName(0);

        for (uint32_t i = 1; i < node->parametersNumber(); ++i)
            cout << ", " << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        cout << ")";

        auto firstNode = node->body()->nodeAt(0);
        if (firstNode->isNativeCallNode()) {
            firstNode->visit(this);
            cout << "\n";
            return;
        }

        cout << "{\n";

        indention += INDENT;
        node->body()->visit(this);
        indention -= INDENT;

        printIndention();
        cout << "}\n";
    }


    void MyVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        cout << node->literal();
    }


    void MyVisitor::visitNativeCallNode(NativeCallNode *node) {
        cout << " native '" << node->nativeName() << "';";
    }


    void MyVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        cout << tokenOp(node->kind());
        node->operand()->visit(this);
    }


    void MyVisitor::visitReturnNode(ReturnNode *node) {
        cout << "return";
        if (!node->returnExpr())
            return;

        cout << " ";
        node->returnExpr()->visit(this);
    }


    void MyVisitor::printIndention() {
        for (int i = 0; i < indention; ++i)
            std::cout << " ";
    }
}