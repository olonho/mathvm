#include "printer.h"


void mathvm::AstPrinterVisitor::print_symbol(char c) {
    switch (c) {
        case '\n': os << "\\n"; break;
        case '\t': os << "\\t"; break;
        case '\r': os << "\\r"; break;
        case '\\': os << "\\";  break;
        case '\'': os << "\\'"; break;
        default  : os << c;     break;
    }
}

void mathvm::AstPrinterVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    os << '\'';
    for (char c: node->literal()) {
        print_symbol(c);
    }
    os << '\'';
}

void mathvm::AstPrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    os << node->literal();
}

void mathvm::AstPrinterVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    os << node->literal();
}

void mathvm::AstPrinterVisitor::visitLoadNode(LoadNode* node) {
    os << node->var()->name();
}

void mathvm::AstPrinterVisitor::visitStoreNode(StoreNode* node) {
    os << node->var()->name();
    os << " ";
    os << tokenOp(node->op());
    os << " ";
    node->visitChildren(this);
    os << ";";
}

void mathvm::AstPrinterVisitor::visitForNode(ForNode* node) {
    os << "for (" << node->var()->name()
         << " in ";
    node->inExpr()->visit(this);

    os << ") ";
    node->body()->visit(this);
}

void mathvm::AstPrinterVisitor::visitWhileNode(WhileNode* node) {
    os << "while (";

    node->whileExpr()->visit(this);
    os << ") ";
    node->loopBlock()->visit(this);
}

void mathvm::AstPrinterVisitor::visitIfNode(IfNode* node) {
    os << "if ";
    os << "(";
    node->ifExpr()->visit(this);
    os << ") ";

    node->thenBlock()->visit(this);
    BlockNode* elseBlock = node->elseBlock();

    if(elseBlock) {
        os << " else ";
        elseBlock->visit(this);
    }
}

void mathvm::AstPrinterVisitor::visitBlockNode(BlockNode* node) {
    ++identNum;
    if(identNum - 1 != 0) {
        os << "{";
        newLine();
    }

    Scope::VarIterator variableIterator(node->scope());
    Scope::FunctionIterator functionIterator(node->scope());
    uint32_t childCount = node->nodes();

    while (variableIterator.hasNext()) {
        AstVar* variable = variableIterator.next();
        print(variable);
        if(variableIterator.hasNext() || functionIterator.hasNext() || childCount > 0) {
            newLine();
        }
    }

    while (functionIterator.hasNext()) {
        AstFunction* function = functionIterator.next();
        visitFunctionNode(function->node());
        if(functionIterator.hasNext() || childCount > 0) {
            newLine();
        }
    }

    for(uint32_t i = 0; i < childCount; ++i) {
        node->nodeAt(i)->visit(this);
        if(i + 1 < childCount) {
            newLine();
        }
    }

    --identNum;
    newLine();
    if (identNum != 0) {
        os << "}";
    }
}

void mathvm::AstPrinterVisitor::visitFunctionNode(FunctionNode* node) {
    os << "function ";
    os << typeToName(node->returnType());
    os << " " << node->name() << " (";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        os << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        if (i != node->parametersNumber() - 1) {
            os << ", ";
        }
    }
    os << ")";
    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
    } else {
        node->body()->visit(this);
    }
}

void mathvm::AstPrinterVisitor::visitReturnNode(ReturnNode* node) {
    os << "return ";
    AstNode* expr = node->returnExpr();
    if(expr) {
        expr->visit(this);
    }
    os << ";";
}

void mathvm::AstPrinterVisitor::visitCallNode(CallNode* node) {
    os << node->name();
    os << "(";
    uint32_t argCount = node->parametersNumber();
    for(uint32_t i = 0; i < argCount; ++i) {
        node->parameterAt(i)->visit(this);
        if(i + 1 != argCount) {
            os << ", ";
        }
    }
    os << ")";
}

void mathvm::AstPrinterVisitor::visitNativeCallNode(NativeCallNode* node) {
    os << "native " << "\'" << node->nativeName() << "\'" << ";";
}

void mathvm::AstPrinterVisitor::visitPrintNode(PrintNode* node) {
    os << "print";
    os << "(";
    uint32_t operandCount = node->operands();
    for(uint32_t i = 0; i < operandCount; ++i) {
        node->operandAt(i)->visit(this);
        if(i + 1 != operandCount) {
            os << "," << " ";
        }
    }
    os << ")";
    os << ";";
}

void mathvm::AstPrinterVisitor::newLine() {
    os << std::endl;
    for (uint32_t i = 1; i < identNum; ++i) {
        os << "  ";
    }
}

void mathvm::AstPrinterVisitor::print(const AstVar* var) {
    os << typeToName(var->type()) << " " << var->name() << ";";
}

void mathvm::AstPrinterVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    os << "(";
    node->left()->visit(this);

    os << " ";
    os << tokenOp(node->kind());
    os << " ";

    node->right()->visit(this);
    os << ")";
}

void mathvm::AstPrinterVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    os << "(";
    os << tokenOp(node->kind());
    node->visitChildren(this);
    os << ")";
}
