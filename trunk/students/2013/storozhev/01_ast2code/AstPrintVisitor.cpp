#include "AstPrintVisitor.hpp"

namespace mathvm {

std::string AstPrintVisitor::makeRaw(const std::string& str) {
    std::string result;
    for (std::string::size_type i = 0; i < str.size(); ++i) {
        std::string::value_type ch = str[i];
        switch (ch) {
            case '\\':
                result.append("\\\\");
                break;
            case '\n':
                result.append("\\n");
                break;
            case '\t':
                result.append("\\t");
                break;
            case '\r':
                result.append("\\r");
                break;
            default:
                result.push_back(ch);
        }
    }
    return result;
}

void AstPrintVisitor::visitBinaryOpNode(BinaryOpNode * node) {
    node->left()->visit(this);
    std::cout << ' ' << tokenOp(node->kind()) << ' ';
    node->right()->visit(this);
}

void AstPrintVisitor::visitUnaryOpNode(UnaryOpNode * node) {
    std::cout << tokenOp(node->kind());
    node->operand()->visit(this);
}

void AstPrintVisitor::visitStringLiteralNode(StringLiteralNode * node) {
    std::cout << '\'' << makeRaw(node->literal()) << '\'';
}

void AstPrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode * node) {
    std::cout << node->literal();
}

void AstPrintVisitor::visitIntLiteralNode(IntLiteralNode * node) {
    std::cout << node->literal();
}

void AstPrintVisitor::visitLoadNode(LoadNode * node) {
    std::cout << node->var()->name();
}

void AstPrintVisitor::visitStoreNode(StoreNode * node) {
    std::cout << node->var()->name() << ' '
            << tokenOp(node->op()) << ' ';
    node->value()->visit(this);
}

void AstPrintVisitor::visitForNode(ForNode * node) {
    std::cout << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    std::cout << ")" << std::endl;
    node->body()->visit(this);
}

void AstPrintVisitor::visitWhileNode(WhileNode * node) {
    std::cout << "while (";
    node->whileExpr()->visit(this);
    std::cout << ")" << std::endl;
    node->loopBlock()->visit(this);
}

void AstPrintVisitor::visitIfNode(IfNode * node) {
    std::cout << "if (";
    node->ifExpr()->visit(this);
    std::cout << ")" << std::endl;
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        std::cout << "else" << std::endl;
        node->elseBlock()->visit(this);
    }
}


void AstPrintVisitor::visitBlockNodeHelper(BlockNode * node, bool needsBraces) {
    if (needsBraces)
        std::cout << "{" << std::endl;
    for (Scope::VarIterator it(node->scope()); it.hasNext(); ) {
        AstVar *v = it.next();
        std::cout << typeToName(v->type()) << " " << v->name() << ";" << std::endl;
    }
    for (size_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
        std::cout << ";" << std::endl;
    }
    if (needsBraces)
        std::cout << "}" << std::endl;
}

void AstPrintVisitor::visitBlockNode(BlockNode * node) {
    visitBlockNodeHelper(node, true);
}

void AstPrintVisitor::visitFunctionNode(FunctionNode * node) {
    bool isTop = node->position() == 0;
    if (!isTop) {
        std::cout << "function " << typeToName(node->returnType())
                << " " << node->name() << "(";
    }
    for (size_t i = 0; i < node->parametersNumber(); ++i) {
        std::cout << typeToName(node->parameterType(i)) << " "
                << node->parameterName(i);
        if (i != node->parametersNumber() - 1 && !isTop)
            std::cout << ",";
    }
    if (!isTop) {
        std::cout << ")"; 
    }
    visitBlockNodeHelper(node->body(), !isTop);
}

void AstPrintVisitor::visitReturnNode(ReturnNode * node) {
    std::cout << "return ";
    node->returnExpr()->visit(this);
}

void AstPrintVisitor::visitCallNode(CallNode * node) {
    std::cout << node->name() << "(";
    for (size_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        if (i != node->parametersNumber() - 1)
            std::cout << ",";
    }
    std::cout << ")";
}

void AstPrintVisitor::visitNativeCallNode(NativeCallNode * node) {
    std::cout << "NativeCall" << std::endl;
}

void AstPrintVisitor::visitPrintNode(PrintNode * node) {
    std::cout << "print (";
    for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        if (i != node->operands() - 1)
            std::cout << ",";
    }
    std::cout << ")";
}

}//namespace
