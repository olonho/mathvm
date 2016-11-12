#include "pretty_print.h"
#include "ast.h"
#include <iostream>


pretty_print::pretty_print() : AstVisitor(), indent(0) {}

pretty_print::~pretty_print() {
    std::cout << ss.str();
}

void pretty_print::visitForNode(mathvm::ForNode *node) {
    ss << "for (" <<  node->var()->name() << " in ";
    node->inExpr()->visit(this);
    ss << ")";
    node->body()->visit(this);
}

void pretty_print::visitPrintNode(mathvm::PrintNode *node) {
    ss << "print(";
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        if (i != node->operands() - 1) {
            ss << ", ";
        }
    }
    ss << ")";
}

void pretty_print::visitLoadNode(mathvm::LoadNode *node) {
    ss << node->var()->name();
}

void pretty_print::visitIfNode(mathvm::IfNode *node) {
    ss << "if (";
    node->ifExpr()->visit(this);
    ss << ")";
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        ss << std::string(indent - 4, ' ');
        ss << " else";
        node->elseBlock()->visit(this);
    }
}

void pretty_print::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
    ss << node->literal();
}

void pretty_print::visitDoubleLiteralNode(mathvm::DoubleLiteralNode *node) {
    ss << node->literal();
}

void pretty_print::visitStringLiteralNode(mathvm::StringLiteralNode *node) {
    ss << "\'";
    for (auto c: node->literal()) {
        switch (c) {
            case '\n':
                ss << "\\n";
                break;
            default:
                ss << c;
                break;
        }
    }
    ss << "\'";
}

void pretty_print::visitWhileNode(mathvm::WhileNode *node) {
    ss << "while (";
    node->whileExpr()->visit(this);
    ss << ") ";
    node->loopBlock()->visit(this);
}

void pretty_print::visitBlockNode(mathvm::BlockNode *node) {
    if (indent >= 4) {
        ss << " {" << std::endl;
    }
    indent += 4;
    mathvm::Scope::VarIterator varIterator{node->scope()};
    while (varIterator.hasNext()) {
        auto *var = varIterator.next();
        ss << std::string(indent - 4, ' ');
        ss << mathvm::typeToName(var->type()) << " " << var->name() << ";" << "\n";
    }

    mathvm::Scope::FunctionIterator functionIterator{node->scope()};
    while (functionIterator.hasNext()) {
        ss << std::string(indent - 4, ' ');
        functionIterator.next()->node()->visit(this);
//        ss << std::endl;
    }

    for (uint32_t i = 0; i < node->nodes(); ++i) {
        ss << std::string(indent - 4, ' ');
        auto block = node->nodeAt(i);
        block->visit(this);
        if (!(block->isBlockNode() || block->isIfNode() || block->isForNode() || block->isWhileNode())) {
            ss << ";";
            ss << std::endl;
        }
    }
    indent -= 4;
    if (indent >= 4) {
        ss << std::string(indent - 4, ' ') << "}" << std::endl;
    }
}

void pretty_print::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
    ss << "(";
    node->left()->visit(this);
    ss << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
    ss << ")";
}

void pretty_print::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
    ss << tokenOp(node->kind());
    node->operand()->visit(this);
}

void pretty_print::visitNativeCallNode(mathvm::NativeCallNode *node) {
    ss << "native \'" << node->nativeName() << "\'";
}

void pretty_print::visitFunctionNode(mathvm::FunctionNode *node) {
    ss << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        ss << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        if (i != node->parametersNumber() - 1) {
            ss << ", ";
        }
    }
    ss << ")";
    node->body()->visit(this);
}

void pretty_print::visitReturnNode(mathvm::ReturnNode *node) {
    ss << "return ";
    if (node->returnExpr() != nullptr) node->returnExpr()->visit(this);
}

void pretty_print::visitStoreNode(mathvm::StoreNode *node) {
    ss << node->var()->name() << " ";
    ss << tokenOp(node->op()) << " ";
    node->value()->visit(this);
}

void pretty_print::visitCallNode(mathvm::CallNode *node) {
    ss << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
    }
    ss << ")";
}

std::string pretty_print::get_text() {
    return ss.str();
}