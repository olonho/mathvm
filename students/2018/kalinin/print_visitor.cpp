//
// Created by Владислав Калинин on 15.10.2018.
//

#include "print_visitor.h"

#define cout_ind cout << string(indent, '\t')

using namespace mathvm;

void Print_visitor::startVisit(FunctionNode *node) {
    node->body()->visit(this);
}

void Print_visitor::visitFunction(FunctionNode *node) {
    cout_ind << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
    uint32_t parametrsNumber = node->parametersNumber();
    if (parametrsNumber > 0) {
        uint32_t i = 0;
        for (; i < parametrsNumber - 1; i++) {
            cout << typeToName(node->parameterType(i)) << " " << node->parameterName(i) << ", ";
        }
        cout << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
    }
    cout << ") ";
    BlockNode *body = node->body();
    if (body->nodes() > 0 && body->nodeAt(0)->isNativeCallNode()) {
        body->visit(this);
        cout << endl;
    } else {
        cout << "{" << endl;
        indent++;
        body->visit(this);
        indent--;
        cout_ind << "}" << endl << endl;
    }
}

void Print_visitor::printFunctionInScope(BlockNode *node) {
    Scope::FunctionIterator functionIterator(node->scope());
    while (functionIterator.hasNext()) {
        functionIterator.next()->node()->visit(this);
    }
}

void Print_visitor::printVarInScope(BlockNode *node) {
    Scope::VarIterator varIterator(node->scope());
    if (node->scope()->variablesCount() > 0) {
        while (varIterator.hasNext()) {
            AstVar *var = varIterator.next();
            cout_ind << typeToName(var->type()) << " " << var->name() << ";" << endl;
        }
        cout << endl;
    }
}

void Print_visitor::printSingleArithmetic(AstNode *node) {
    cout_ind;
    node->visit(this);
    cout << ";" << endl;
}

void Print_visitor::visitBlocStatements(BlockNode *node) {
    for (uint32_t i = 0; i < node->nodes(); i++) {
        AstNode *stm = node->nodeAt(i);
        if (stm->isBinaryOpNode() || stm->isUnaryOpNode() || stm->isCallNode()) {
            printSingleArithmetic(stm);
        } else {
            node->nodeAt(i)->visit(this);
        }
    }
}

void Print_visitor::visitFunctionNode(FunctionNode *node) {
    if (node->name() == "<top>") {
        startVisit(node);
    } else {
        visitFunction(node);
    }
}

void Print_visitor::visitBlockNode(BlockNode *node) {
    printVarInScope(node);
    printFunctionInScope(node);
    visitBlocStatements(node);
}

void Print_visitor::visitIfNode(IfNode *node) {
    cout_ind << "if (";
    node->ifExpr()->visit(this);
    cout << ") {" << endl;
    indent++;
    node->thenBlock()->visit(this);
    indent--;
    if (node->elseBlock() != nullptr) {
        cout_ind << "} else {" << endl;
        indent++;
        node->elseBlock()->visit(this);
        indent--;
    }
    cout_ind << "}" << endl;
}

void Print_visitor::visitWhileNode(WhileNode *node) {
    cout_ind << "while (";
    node->whileExpr()->visit(this);
    cout << ") {" << endl;
    indent++;
    node->loopBlock()->visit(this);
    indent--;
    cout_ind << "}" << endl;
}

void Print_visitor::visitStoreNode(StoreNode *node) {
    const AstVar *var = node->var();
    cout_ind << var->name() << " " << tokenOp(node->op()) << " ";
    node->value()->visit(this);
    cout << ";" << endl;
}

void Print_visitor::visitLoadNode(LoadNode *node) {
    cout << node->var()->name();
}

void Print_visitor::visitBinaryOpNode(BinaryOpNode *node) {
    if (node->left()->isBinaryOpNode() && node->left()->asBinaryOpNode()->kind() < node->kind()) {
        printWithParens(node->left());
    } else {
        printWithoutParens(node->left());
    }
    cout << " " << tokenOp(node->kind()) << " ";
    if (node->right()->isBinaryOpNode() && node->right()->asBinaryOpNode()->kind() <= node->kind()) {
        printWithParens(node->right());
    } else {
        printWithoutParens(node->right());
    }
}

void Print_visitor::visitUnaryOpNode(UnaryOpNode *node) {
    cout << tokenOp(node->kind());
    if (node->operand()->isBinaryOpNode() || node->operand()->isUnaryOpNode()) {
        printWithParens(node->operand());
    } else {
        printWithoutParens(node->operand());
    }
}

void Print_visitor::printWithParens(AstNode *node) {
    cout << "(";
    node->visit(this);
    cout << ")";
}

void Print_visitor::printWithoutParens(AstNode *node) {
    node->visit(this);
}

void Print_visitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    cout << node->literal();
}

void Print_visitor::visitIntLiteralNode(IntLiteralNode *node) {
    cout << node->literal();
}

void Print_visitor::visitStringLiteralNode(StringLiteralNode *node) {
    const string &str = node->literal();
    cout << "'";
    for (char ch : str) {
        switch (ch) {
            case '\t':
                cout << ("\\t");
                break;
            case '\n':
                cout << ("\\n");
                break;
            case '\r':
                cout << ("\\r");
                break;
            case '\'':
                cout << ("\\'");
                break;
            case '\\':
                cout << ("\\\\");
                break;
            default:
                cout << (ch);
        }
    }
    cout << "'";
}

void Print_visitor::visitForNode(ForNode *node) {
    cout_ind << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    cout << ") {" << endl;
    indent++;
    node->body()->visit(this);
    indent--;
    cout_ind << "}" << endl;
}

void Print_visitor::visitReturnNode(ReturnNode *node) {
    if (node->returnExpr() != nullptr) {
        cout_ind << "return ";
        node->returnExpr()->visit(this);
        cout << ";" << endl;
    }
}

void Print_visitor::visitPrintNode(PrintNode *node) {
    cout_ind << "print (";
    uint32_t parametrsNumber = node->operands();
    if (parametrsNumber > 0) {
        uint32_t i = 0;
        for (; i < parametrsNumber - 1; i++) {
            node->operandAt(i)->visit(this);
            cout << ", ";
        }
        node->operandAt(i)->visit(this);
    }
    cout << ");" << endl;
}

void Print_visitor::visitCallNode(CallNode *node) {
    cout << node->name() << "(";
    uint32_t parametrsNumber = node->parametersNumber();
    if (parametrsNumber > 0) {
        uint32_t i = 0;
        for (; i < parametrsNumber - 1; i++) {
            node->parameterAt(i)->visit(this);
            cout << ", ";
        }
        node->parameterAt(i)->visit(this);
    }
    cout << ")";
}

void Print_visitor::visitNativeCallNode(NativeCallNode *node) {
    cout << " native '" << node->nativeName() << "';" << endl;
}
