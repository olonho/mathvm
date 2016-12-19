#include <iomanip>
#include "IdentityTranslator.h"
#include "../../../../vm/parser.h"
#include "StringUtils.h"

mathvm::IdentityTranslator::~IdentityTranslator() {

}


mathvm::IdentityVisitor::IdentityVisitor(bool printTop) : AstBaseVisitor() {
    this->printTop = printTop;
}

mathvm::IdentityVisitor::~IdentityVisitor() {

}

void mathvm::IdentityVisitor::visitForNode(mathvm::ForNode *node) {
    std::cout << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    std::cout << ")";
    visitBlockNode(node->body());
}

void mathvm::IdentityVisitor::visitPrintNode(mathvm::PrintNode *node) {
    enterLine();
    std::cout << "print(";
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        if (i < node->operands() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ")";
}

void mathvm::IdentityVisitor::visitLoadNode(mathvm::LoadNode *node) {
    std::cout << node->var()->name();
    AstBaseVisitor::visitLoadNode(node);
}

void mathvm::IdentityVisitor::visitIfNode(mathvm::IfNode *node) {
    std::cout << "if (";
    node->ifExpr()->visit(this);
    std::cout << ")";
    visitBlockNode(node->thenBlock());
    if (node->elseBlock() != nullptr) {
        exitLine();
        enterLine();
        std::cout << "else";
        visitBlockNode(node->elseBlock());
    }
}

void mathvm::IdentityVisitor::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
    std::cout << "(";
    node->left()->visit(this);
    std::cout << ' ';
    std::cout << tokenOp(node->kind());
    std::cout << ' ';
    node->right()->visit(this);
    std::cout << ")";
}

void mathvm::IdentityVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode *node) {
    std::cout << std::fixed << node->literal();
}

void mathvm::IdentityVisitor::visitStoreNode(mathvm::StoreNode *node) {
    std::cout << node->var()->name();
    std::cout << ' ';
    std::cout << operationList[node->op()];
    std::cout << ' ';
    node->value()->visit(this);
}

void mathvm::IdentityVisitor::visitStringLiteralNode(mathvm::StringLiteralNode *node) {
    std::cout << "'" << utils::StringUtils::escapeString(node->literal()) << "'";
}

void mathvm::IdentityVisitor::visitWhileNode(mathvm::WhileNode *node) {
    std::cout << "while (";
    node->whileExpr()->visit(this);
    std::cout << ")";
    visitBlockNode(node->loopBlock());
}

void mathvm::IdentityVisitor::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
    std::cout << node->literal();
}

void mathvm::IdentityVisitor::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
    std::cout << tokenOp(node->kind());
    node->operand()->visit(this);
}

void mathvm::IdentityVisitor::visitNativeCallNode(mathvm::NativeCallNode *node) {
    throw std::runtime_error("no native call");
}

void mathvm::IdentityVisitor::visitBlockNode(mathvm::BlockNode *node) {
    bool printBrackets = !this->printTop;
    this->printTop = false;
    if (printBrackets) {
        std::cout << " {";
        exitLine(true);
        ++indentLevel;
    }
    scopePrinter(node->scope());
    for (uint32_t i = 0; i < node->nodes(); i++) {
        enterLine();
        node->nodeAt(i)->visit(this);
        exitLine();
    }
    if (printBrackets) {
        --indentLevel;
        enterLine();
        std::cout << "}";
    }
    softRequest = true;
}

void mathvm::IdentityVisitor::visitFunctionNode(mathvm::FunctionNode *node) {
    if (!this->printTop) {
        std::cout << "function " << typeToString(node->returnType()) << " " << node->name() << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
            std::cout << typeToString(node->parameterType(i)) << " " << node->parameterName(i);
            if (i < node->parametersNumber() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << ")";
    }
    if ((node->body()->nodes() > 0) && (node->body()->nodeAt(0)->isNativeCallNode())) {
        std::cout << " native '" << node->body()->nodeAt(0)->asNativeCallNode()->nativeName() << "'";
    }
    else {
        AstBaseVisitor::visitFunctionNode(node);
    }
}

void mathvm::IdentityVisitor::visitReturnNode(mathvm::ReturnNode *node) {
    std::cout << "return ";
    node->returnExpr()->visit(this);
}

void mathvm::IdentityVisitor::visitCallNode(mathvm::CallNode *node) {
    std::cout << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        node->parameterAt(i)->visit(this);
        if (i < node->parametersNumber() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ")";
}

void mathvm::IdentityVisitor::scopePrinter(Scope *scope) {
    Scope::VarIterator it = Scope::VarIterator(scope);
    while (it.hasNext()) {
        AstVar *next = it.next();
        enterLine();
        std::cout << typeToString(next->type()) << " " << next->name();
        exitLine();
    }
    Scope::FunctionIterator itFun = Scope::FunctionIterator(scope);
    while (itFun.hasNext()) {
        enterLine();
        AstFunction *next = itFun.next();
        visitFunctionNode(next->node());
        exitLine();
    }
}

std::string mathvm::IdentityVisitor::typeToString(mathvm::VarType type) {
    switch (type) {
        case VT_VOID:
            return "void";
        case VT_DOUBLE:
            return "double";
        case VT_INT:
            return "int";
        case VT_STRING:
            return "string";
        default:
            return "unknown";
    }
}

void mathvm::IdentityVisitor::enterLine() {
    if (shift) {
        for (int32_t i = 0; i < indentLevel * indentSize; ++i) {
            std::cout << ' ';
        }
        shift = false;
    }
}

void mathvm::IdentityVisitor::exitLine(bool soft) {
    if (!soft) {
        if (softRequest) {
            softRequest = false;
        }
        else {
            std::cout << ';';
        }
    }
    std::cout << std::endl;
    shift = true;
}


mathvm::Status *mathvm::IdentityTranslator::translate(const std::string &program, mathvm::Code **code) {
    if (code == nullptr) {
        return Status::Error("Code is nullptr");
    }

    Parser parserInstance = Parser();
    Status *parseStatus = parserInstance.parseProgram(program);
    if (parseStatus->isError()) {
        return parseStatus;
    }
    delete parseStatus;

    IdentityVisitor *visitor = new IdentityVisitor();
    parserInstance.top()->node()->visit(visitor);
    delete visitor;

    return Status::Ok();
}

