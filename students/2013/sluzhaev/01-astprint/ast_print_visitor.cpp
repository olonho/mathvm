#include "ast_print_visitor.h"

#include <sstream>

#define TAB_SIZE 4

using namespace mathvm;

AstPrintVisitor::AstPrintVisitor(std::ostream& out) : level(0), out(out) {}

void AstPrintVisitor::printCode(const AstFunction *root) {
    level = -1;
    root->node()->body()->visit(this);
}

void AstPrintVisitor::visitBlockNode(BlockNode *node) {
    ++level;
    Scope::VarIterator v(node->scope());
    while (v.hasNext()) {
        printIndent();
        AstVar* var = v.next();
        out << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
    }
    if (node->scope()->variablesCount() > 0) {
        out << std::endl;
    }
    Scope::FunctionIterator f(node->scope());
    while (f.hasNext()) {
        printIndent();
        AstFunction* func = f.next();
        func->node()->visit(this);
        if (func->node()->body()->nodeAt(0)->isNativeCallNode()) {
            out << " ";
            func->node()->body()->nodeAt(0)->visit(this);
            out << ";" << std::endl;
        } else {
            out << " {" << std::endl;
            func->node()->body()->visit(this);
            printIndent();
            out << "}" << std::endl;
        }    
        out << std::endl;
    }
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        printIndent();
        node->nodeAt(i)->visit(this);
        if (!node->nodeAt(i)->isForNode() && !node->nodeAt(i)->isWhileNode() &&
                !node->nodeAt(i)->isIfNode()) {
            out << ";";
        }
        out << std::endl;
    }
    --level;
}

void AstPrintVisitor::visitFunctionNode(FunctionNode *node) {
    out << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        if (i) {
            out << ", ";
        }
        out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
    }
    out << ")";
}

void AstPrintVisitor::visitNativeCallNode(NativeCallNode *node) {
    out << " native '" << node->nativeName() << "';";
}

void AstPrintVisitor::visitCallNode(CallNode *node) {
    out << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        if (i) {
            out << ", ";
        }
        node->parameterAt(i)->visit(this);
    }
    out << ")";
}

void AstPrintVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    out << tokenOp(node->kind());
    last_precedence = tokenPrecedence(node->kind());
    node->operand()->visit(this);
}

void AstPrintVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    bool parens_required = tokenPrecedence(node->kind()) < last_precedence;
    if (parens_required) {
        out << "(";
    }
    last_precedence = tokenPrecedence(node->kind());
    node->left()->visit(this);
    out << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
    if (parens_required) {
        out << ")";
    }
}

void AstPrintVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    out << node->literal();
}

void AstPrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    out << node->literal();
}

void AstPrintVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    out << "'";
    for (std::string::const_iterator c = node->literal().begin(); c != node->literal().end(); ++c) {
        switch (*c) {
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        case '\\': out << "\\\\"; break;
        default: out << *c;
        }
    }
    out << "'";
}

void AstPrintVisitor::visitLoadNode(LoadNode *node) {
    out << node->var()->name();
}

void AstPrintVisitor::visitStoreNode(StoreNode *node) {
    out << node->var()->name() << " " << tokenOp(node->op()) << " ";
    node->value()->visit(this);
}

void AstPrintVisitor::visitForNode(ForNode *node) {
    out << "for (" << node->var()->name() << " in ";
    last_precedence = 0;
    node->inExpr()->visit(this);
    out << ")";
    node->body()->visit(this);
}

void AstPrintVisitor::visitWhileNode(WhileNode *node) {
    out << "while (";
    last_precedence = 0;
    node->whileExpr()->visit(this);
    out << ") {" << std::endl;
    node->loopBlock()->visit(this);
}

void AstPrintVisitor::visitIfNode(IfNode *node) {
    out << "if (";
    last_precedence = 0;
    node->ifExpr()->visit(this);
    out << ") {" << std::endl;
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        out << "} else {" << std::endl;
        node->elseBlock()->visit(this);
    }
    printIndent();
    out << "}";
}

void AstPrintVisitor::visitReturnNode(ReturnNode *node) {
    out << "return";
    if (node->returnExpr()) {
        out << " ";
        node->returnExpr()->visit(this);
    }
}

void AstPrintVisitor::visitPrintNode(PrintNode *node) {
    out << "print(";
    if (node->operands()) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            if (i) {
                out << ", ";
            }
            last_precedence = 0;
            node->operandAt(i)->visit(this);
        }
    }
    out << ")";
}

void AstPrintVisitor::printIndent() {
    for (uint32_t j = 0; j < level; ++j) {
        out << "    ";
    }
}
