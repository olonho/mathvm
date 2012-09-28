#include "Ast2SrcVisitor.h"

namespace mathvm
{

Ast2SrcVisitor::Ast2SrcVisitor(std::ostream& out) : _out(out) {}

void Ast2SrcVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    _out << "(";
    node->left()->visit(this);
    _out << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
    _out << ")";
}
void Ast2SrcVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    _out << " " << tokenOp(node->kind()) << "(";
    node->operand()->visit(this);
    _out << ")";
}

std::string escape(const std::string& str) {
    std::string t;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        switch (*it) {
        case '\n':
            t += "\\n";
        case '\r':
            t +=  "\\r";
        case '\\':
            t +=  "\\\\";
        case '\t':
            t +=  "\\t";
        default:
            t += *it;
        }
    }
    return t;
}

void Ast2SrcVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    _out << "\'" << escape(node->literal()) << "\'";
}
void Ast2SrcVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    _out << node->literal();
}
void Ast2SrcVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    _out << node->literal();
}

void Ast2SrcVisitor::visitLoadNode(LoadNode* node) {
    _out << node->var()->name();
}
void Ast2SrcVisitor::visitStoreNode(StoreNode* node) {
    _out << node->var()->name();
    switch (node->op()) {
        case tASSIGN :
            _out << " = ";
            break;
        case tINCRSET :
            _out << " += ";
            break;
        case tDECRSET :
            _out << " -= ";
            break;
        default: assert("Wrong operation" == 0);
    }
    node->visitChildren(this);
}

void Ast2SrcVisitor::initScope(Scope* scope) {
    Scope::VarIterator varIt(scope);
    while (varIt.hasNext()) {
        AstVar* var = varIt.next();
        _out << mathvm::typeToName(var->type()) << " "<< var->name() << ";" << std::endl;
    }

    Scope::FunctionIterator funcIt(scope);
    while (funcIt.hasNext()) {
        AstFunction* func = funcIt.next();
        func->node()->visit(this);
    }
}

void Ast2SrcVisitor::visitBlockNode(BlockNode* node) {
    _out << "{" << std::endl;
    initScope(node->scope());
    node->visitChildren(this);
    _out << "}" << std::endl;
}

void Ast2SrcVisitor::visitForNode(ForNode* node) {
    _out << "for (" 
         << mathvm::typeToName(node->var()->type()) << " " << node->var()->name();
    _out << " in ";
    node->inExpr()->visit(this);
    _out << ") ";
    node->body()->visit(this);
}

void Ast2SrcVisitor::visitWhileNode(WhileNode* node) {
    _out << "while (";
    node->whileExpr()->visit(this);
    _out << ") ";
    node->loopBlock()->visit(this);
}

void Ast2SrcVisitor::visitIfNode(IfNode* node) {
    _out << "if (";
    node->ifExpr()->visit(this);
    _out << ") ";
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }
}
void Ast2SrcVisitor::outSignatureParams(const Signature& signature) {
    Signature::const_iterator it = signature.begin() + 1;
    const Signature::const_iterator endIt = signature.end();

    if (it != endIt) {
        while (true) {
            _out << mathvm::typeToName(it->first) << " " << it->second;
            ++it;
            if (it == endIt)
                break;
            _out << ",";
        }
    }
}

void Ast2SrcVisitor::visitFunctionNode(FunctionNode* node) {
    _out << "function "
         << mathvm::typeToName(node->returnType()) << " "
         << node->name() << "(";
    
    outSignatureParams(node->signature());

    _out << ")" << std::endl;

    node->visitChildren(this);
}
void Ast2SrcVisitor::visitReturnNode(ReturnNode* node) {
    _out << "return ";
    node->visitChildren(this);
    _out << ";" << std::endl;
}

void Ast2SrcVisitor::visitCallNode(CallNode* node) {
    _out << node->name() << "(";
    if (node->parametersNumber() > 0) {
        node->parameterAt(0)->visit(this);
    }
    for (uint32_t i = 1; i < node->parametersNumber(); i++) {
        _out << ", ";
        node->parameterAt(i)->visit(this);
    }
    _out << ")";
}

void Ast2SrcVisitor::visitNativeCallNode(NativeCallNode* node) {
    _out << node->nativeName() << "(";
    outSignatureParams(node->nativeSignature());
    _out << ")" << std::endl;
}

void Ast2SrcVisitor::visitPrintNode(PrintNode* node) {
    _out << "print(";
    uint32_t size = node->operands();
    for (uint32_t i = 0; i < size - 1; ++i) {
        node->operandAt(i)->visit(this);
        _out << ",";
    }
    if (size > 0)
        node->operandAt(size - 1)->visit(this);

    _out << ");" << std::endl;
}
    
}
