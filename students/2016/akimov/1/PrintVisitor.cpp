#include "PrintVisitor.h"

using namespace mathvm;

void PrintVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    _out << "(";
    node->left()->visit(this);
    _out << " ";
    _out << tokenOp(node->kind());
    _out << " ";
    node->right()->visit(this);
    _out << ")";
}

void PrintVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    _out << tokenOp(node->kind());
    node->operand()->visit(this);
}

string escape(char ch) {
    switch (ch) {
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        case '\\': return "\\\\";
        default: return string(1, ch);
    }
}

void PrintVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    _out << "'";
    for (char ch : node->literal()) {
        _out << escape(ch);
    }
    _out << "'";
}

void PrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    _out << node->literal();
}

void PrintVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    _out << node->literal();
}

void PrintVisitor::visitLoadNode(LoadNode* node) {
    _out << node->var()->name();
}

void PrintVisitor::visitStoreNode(StoreNode* node) {
    _out << node->var()->name();
    _out << " ";
    _out << tokenOp(node->op());
    _out << " ";
    node->value()->visit(this);
}

void PrintVisitor::visitForNode(ForNode* node) {
    _out << "for (";
    _out << node->var()->name();
    _out << " in ";
    node->inExpr()->visit(this);
    _out << ") ";
    node->body()->visit(this);
}

void PrintVisitor::visitWhileNode(WhileNode* node) {
    _out << "while (";
    node->whileExpr()->visit(this);
    _out << ") ";
    node->loopBlock()->visit(this);
}

void PrintVisitor::visitIfNode(IfNode* node) {
    _out << "if (";
    node->ifExpr()->visit(this);
    _out << ") ";
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        _out << " else ";
        node->elseBlock()->visit(this);
    }
}

void PrintVisitor::printIndent() {
    _out << string((_indentLevel - 1) * 4, ' ');
}

void PrintVisitor::visitBlockNode(BlockNode* node) {
    if (_indentLevel) {
        _out << "{" << endl;
    }
    ++_indentLevel;

    for (Scope::VarIterator it(node->scope()); it.hasNext();) {
        printIndent();
        AstVar* var = it.next();
        _out << typeToName(var->type()) << " " << var->name();
        _out << ";" << endl;
    }
    for (Scope::FunctionIterator it(node->scope()); it.hasNext();) {
        printIndent();
        visitFunctionNode(it.next()->node());
        _out << endl;
    }
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        printIndent();
        AstNode* blockNode = node->nodeAt(i);
        blockNode->visit(this);
        if (!blockNode->isIfNode() &&
                !blockNode->isForNode() &&
                !blockNode->isWhileNode()) {
            _out << ";";
        }
        _out << endl;
    }

    --_indentLevel;
    if (_indentLevel) {
        printIndent();
        _out << "}";
    }
}

void PrintVisitor::visitFunctionNode(FunctionNode* node) {
    _out << "function ";
    _out << typeToName(node->returnType()) << " ";
    _out << node->name();
    _out << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        if (i) {
            _out << ", ";
        }
        _out << typeToName(node->parameterType(i));
        _out << " " << node->parameterName(i);
    }
    _out << ") ";
    if (node->body()->nodes() == 2 &&
            node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
    } else {
        node->body()->visit(this);
    }
}

void PrintVisitor::visitReturnNode(ReturnNode* node) {
    _out << "return ";
    node->returnExpr()->visit(this);
}

void PrintVisitor::visitCallNode(CallNode* node) {
    _out << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        if (i) {
            _out << ", ";
        }
        node->parameterAt(i)->visit(this);
    }
    _out << ")";
}

void PrintVisitor::visitNativeCallNode(NativeCallNode* node) {
    _out << "native '" << node->nativeName() << "';" << endl;
}

void PrintVisitor::visitPrintNode(PrintNode* node) {
    _out << "print(";
    for (uint32_t i = 0; i < node->operands(); ++i) {
        if (i) {
            _out << ", ";
        }
        node->operandAt(i)->visit(this);
    }
    _out << ")";
}
