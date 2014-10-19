#include "ast_printer.h"
#include "parser.h"
#include <cmath>

namespace mathvm {

Translator* Translator::create(const string & impl) {
    if (impl == "printer") {
        return new AstPrinter();
    }
    else {
        return NULL;
    }
}

Status* AstPrinter::translate(const string& program, Code* *code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status != NULL && status->isError()) {
        return status;
    }

    AstFunction* top = parser.top();
    AstPrinterVisitor printer;
    printer.visitFunctionNode(top->node());

    return new Status();
}

void AstPrinterVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    _output << "(";
    node->left()->visit(this);
    _output << " ";
    printTokenKind(node->kind());
    _output << " ";
    node->right()->visit(this);
    _output << ")";
}

void AstPrinterVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    _output << "(";
    printTokenKind(node->kind());
    node->operand()->visit(this);
    _output << ")";
}

void AstPrinterVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    _output << '\'';
    for (string::const_iterator it = node->literal().begin(); it != node->literal().end(); ++it) {
        if (*it == '\n') {
            _output << "\\" << "n";
        }
        else {
            _output << *it;
        }
    }
    _output << '\'';
}

void AstPrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    _output << node->literal();
}

void AstPrinterVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    _output << node->literal();
}

void AstPrinterVisitor::visitLoadNode(LoadNode* node) {
    _output << node->var()->name();
}

void AstPrinterVisitor::visitStoreNode(StoreNode* node) {
    _output << node->var()->name();
    _output << " ";
    printTokenKind(node->op());
    _output << " ";
    node->value()->visit(this);
}

void AstPrinterVisitor::visitForNode(ForNode* node) {
    _output << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    _output << ")" << endl;
    node->body()->visit(this);
}

void AstPrinterVisitor::visitWhileNode(WhileNode* node) {
    _output << "while";
    node->visitChildren(this);
}

void AstPrinterVisitor::visitIfNode(IfNode* node) {
    _output << "if";
    node->ifExpr()->visit(this);
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        _output << "else ";
        node->elseBlock()->visit(this);
    }
}

void AstPrinterVisitor::visitBlockNode(BlockNode* node) {
    if (!isMainScope(node->scope())) {
        _output << "{" << endl;
    }

    Scope::VarIterator varIt(node->scope());
    while(varIt.hasNext()) {
        AstVar* var = varIt.next();
        printVarType(var->type());
        _output << " " << var->name();
        printSemicolon();
    }

    Scope::FunctionIterator funcIt(node->scope());
    while(funcIt.hasNext()) {
        FunctionNode* func = funcIt.next()->node();
        func->visit(this);
    }

    for (uint32_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
        if (!(node->nodeAt(i)->isIfNode()
              || node->nodeAt(i)->isWhileNode()
              || node->nodeAt(i)->isForNode()
              || node->nodeAt(i)->isReturnNode()
              || node->nodeAt(i)->isBlockNode())) {
            printSemicolon();
        }
    }
    if (!isMainScope(node->scope())) {
        _output << "}" << endl;
    }
}

void AstPrinterVisitor::visitFunctionNode(FunctionNode* node) {
    if (node->name() != "<top>") {
        _output << "function ";
        printVarType(node->returnType());
        _output << " " << node->name() << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            printVarType(node->parameterType(i));
            _output << " " << node->parameterName(i);
            if (i != node->parametersNumber() - 1) {
                _output << ", ";
            }
        }
        _output << ")";
    }
    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
        printSemicolon();
    }
    else {
        _output << endl;
        node->body()->visit(this);
    }
}

void AstPrinterVisitor::visitReturnNode(ReturnNode* node) {
    if (node->returnExpr() != NULL) {
        _output << "return ";
        node->returnExpr()->visit(this);
        printSemicolon();
    }
}

void AstPrinterVisitor::visitCallNode(CallNode* node) {
    _output << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        if (i != node->parametersNumber() - 1) {
            _output << ", ";
        }
    }
    _output << ")";
}

void AstPrinterVisitor::visitNativeCallNode(NativeCallNode* node) {
    _output << " native " << '\'' << node->nativeName() << '\'';
}

void AstPrinterVisitor::visitPrintNode(PrintNode* node) {
    _output << "print(";
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        if (i != node->operands() - 1) {
            _output << ", ";
        }
    }
    _output << ")";
}

#define FOR_VARTYPES(DO)           \
    DO(VT_INVALID, "", 0)          \
    DO(VT_VOID, "void", 0)         \
    DO(VT_DOUBLE, "double", 0)     \
    DO(VT_INT, "int", 0)           \
    DO(VT_STRING, "string", 0)

#define CASE_ELEM(t,s,p)           \
    case t:                        \
    _output << s;                     \
    break;

void AstPrinterVisitor::printTokenKind(TokenKind kind) {
    switch(kind) {
    FOR_TOKENS(CASE_ELEM)
            default:
        break;
    }
}

void AstPrinterVisitor::printVarType(VarType type) {
    switch(type) {
    FOR_VARTYPES(CASE_ELEM)
            default:
        break;
    }
}

void AstPrinterVisitor::printSemicolon() {
    _output << ";" << endl;
}

bool AstPrinterVisitor::isMainScope(Scope* scope) {
    return scope->parent()->parent() == NULL;
}


}
