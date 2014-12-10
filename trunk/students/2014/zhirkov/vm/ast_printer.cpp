
#include "../../../../include/mathvm.h"


#include "ast_printer.h"
#include "util.h"


namespace mathvm {

    void AstPrinterVisitor::indent() {
        for (size_t i = 0; i < _spacesForIndent * _indent; i++)
            _out << ' ';
    }

    void AstPrinterVisitor::_enter() {
        _indent++;
    }

    void AstPrinterVisitor::_leave() {
        _indent--;
    }

    void AstPrinterVisitor::functionDeclaration(Scope *scope) {
        Scope::FunctionIterator iter(scope);
        while (iter.hasNext())
            iter.next()->node()->visit(this);

    }

    void AstPrinterVisitor::variableDeclaration(Scope *scope) {
        Scope::VarIterator iter(scope);
        while (iter.hasNext()) {
            AstVar &x = *(iter.next());
            indent();
            _out <<
                    typeToName(x.type()) <<
                    " " <<
                    x.name() <<
                    ";" <<
                    std::endl;
        }
    }


    void AstPrinterVisitor::enterBlock(BlockNode *node) {
        variableDeclaration(node->scope());
        functionDeclaration(node->scope());

        for (uint32_t i = 0; i < node->nodes(); i++) {
            indent();
            AstNode &current = *(node->nodeAt(i)); //I d think it should be size_t -_-
            current.visit(this);

            //hacky
            if (current.isCallNode()) _out << ';';
            _out << endl;
        }
    }

    void AstPrinterVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        _out << '(';
        node->left()->visit(this);
        _out << ' ' << tokenOp(node->kind()) << ' ';
        node->right()->visit(this);
        _out << ')';
    }

    void AstPrinterVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        _out << tokenOp(node->kind()) << ' ';
        node->operand()->visit(this);
    }

    void AstPrinterVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        _out << '\'' << escape(node->literal()) << '\'';
    }

    void AstPrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        _out << node->literal();
    }

    void AstPrinterVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        _out << node->literal();
    }

    void AstPrinterVisitor::visitLoadNode(LoadNode *node) {
        _out << node->var()->name();
    }

    void AstPrinterVisitor::visitStoreNode(StoreNode *node) {
        _out << node->var()->name() << ' '
                << tokenOp(node->op()) << ' ';
        node->value()->visit(this);
        _out << ';';
    }

    void AstPrinterVisitor::visitForNode(ForNode *node) {
        _out << "for ("
                << node->var()->name()
                << " in ";
        node->inExpr()->visit(this);
        _out << ')';
        node->body()->visit(this);
    }

    void AstPrinterVisitor::visitWhileNode(WhileNode *node) {
        _out << "while (";
        node->whileExpr()->visit(this);
        _out << ") ";
        node->loopBlock()->visit(this);
    }

    void AstPrinterVisitor::visitIfNode(IfNode *node) {
        _out << "if (";
        node->ifExpr()->visit(this);
        _out << ") ";
        node->thenBlock()->visit(this);
        if (node->elseBlock() != NULL) {
            _out << " else ";
            node->elseBlock()->visit(this);
        }
    }

    void AstPrinterVisitor::visitBlockNode(BlockNode *node) {
        _out << endl;
        indent();
        _out << '{';
        _enter();
        _out << endl;

        enterBlock(node);
        _leave();
        indent();
        _out << '}' << endl;
    }

    void AstPrinterVisitor::visitNativeCallNode(NativeCallNode *node) {
        _out << " native '" << node->nativeName()
                << "';" << endl;
    }

    void AstPrinterVisitor::visitFunctionNode(FunctionNode *node) {
        if (node->name() != AstFunction::top_name) {
            _out << "function "
                    << typeToName(node->returnType()) << ' '
                    << node->name()
                    << '(';
            for (uint32_t i = 0; i < node->parametersNumber(); i++) // IT SHOULD BE size_t, shouldn't it?
            {
                if (i != 0)
                    _out << ", ";
                _out << typeToName(node->parameterType(i))
                        << " " << node->parameterName(i);
            }
            _out << ")";
        }
        if (node->body()->nodeAt(0)->isNativeCallNode())
            visitNativeCallNode(node->body()->nodeAt(0)->asNativeCallNode());
        else {
            node->body()->visit(this);
            _out << endl;
        }
    }

    void AstPrinterVisitor::visitReturnNode(ReturnNode *node) {
        _out << "return";
        if (node->returnExpr() != NULL) {
            _out << ' ';
            node->returnExpr()->visit(this);
        }
        _out << ';';
    }

    void AstPrinterVisitor::visitCallNode(CallNode *node) {
        _out << node->name() << '(';
        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
            if (i != 0) _out << ", ";
            node->parameterAt(i)->visit(this);
        }
        _out << ')';
    }

    void AstPrinterVisitor::visitPrintNode(PrintNode *node) {
        _out << "print (";
        for (uint32_t i = 0; i < node->operands(); i++) {
            if (i != 0) _out << ", ";
            node->operandAt(i)->visit(this);
        }
        _out << ");";
    }

}


