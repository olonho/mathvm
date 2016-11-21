//
// Created by Mark Geller on 13.10.16.
//

#include "translator_impl.h"
const int ASTPrinter::padding = 2;

ASTPrinter::ASTPrinter(AstFunction *root, ostream& os): os(os) {
    if(root->top_name == "<top>"){
        this->root = root->node();
    } else {
        root = nullptr;
    }
}

bool ASTPrinter::print() {
    if(root == nullptr){
        os << "invalid root provided";
        return false;
    }
    depth_ = 0;
    visitBlockNode(root->body());
    return true;
}

void ASTPrinter::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    cout << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
}

void ASTPrinter::visitUnaryOpNode(UnaryOpNode *node){
    os << tokenOp(node->kind());
    node->operand()->visit(this);
}


void ASTPrinter::visitBlockNode(BlockNode *node) {
    if(depth_ != 0){
        os << "{ \n";
    }
    int total_pad = depth_ * padding;
    std::string prefix = std::string(total_pad, ' ');
    depth_ ++;
    Scope::VarIterator varIterator(node->scope());
    while (varIterator.hasNext()){
        AstVar* var(varIterator.next());
        os << prefix << typeToName(var->type()) << " " << var->name() << ";\n";
    }
    Scope::FunctionIterator functionIterator(node->scope());
    while (functionIterator.hasNext()){
        AstFunction* fun = functionIterator.next();
        os << prefix;
        fun->node()->visit(this);
    }
    for (int i = 0; i < node->nodes(); ++i) {
        os << prefix;
        AstNode* child = node->nodeAt(i);
        child->visit(this);
        if(!(   child->isIfNode()      ||
                child->isWhileNode()   ||
                child->isForNode()     ||
                child->isFunctionNode()||
                child->isNativeCallNode())){
            os << ";\n";
        }


    }
    depth_--;
    if(depth_ != 0){
        total_pad = (depth_-1) * padding;
        prefix = std::string(total_pad, ' ');
        os << prefix << "} \n";
    }
}

void ASTPrinter::visitCallNode(CallNode *node) {
    os << node->name() << "(";
    for(int i = 0; i < node->parametersNumber(); i++){
        node->parameterAt(i)->visit(this);
        if(i != node->parametersNumber()-1){
            cout << ", ";
        }
    }
    cout << ")";
}

void ASTPrinter::visitForNode(ForNode *node) {
    os << "for ( " << node->var()->name() << "in ";
    node->inExpr()->visit(this);
    os << ")";
    node->body()->visit(this);

}

void ASTPrinter::visitFunctionNode(FunctionNode *node) {
    os << "function ";
    os << typeToName(node->returnType());
    os << " " << node->name() << " (";
    for (int i = 0; i < node->parametersNumber(); ++i) {
        os << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        if(i != node->parametersNumber()-1){
            os << ", ";
        }
    }
    os << ")";
    if(node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()){
        node->body()->nodeAt(0)->visit(this);
    }else{
        node->body()->visit(this);
    }
}

void ASTPrinter::visitIfNode(IfNode *node) {
    os << "if (";
    node->ifExpr()->visit(this);
    os << ")";
    node->thenBlock()->visit(this);
}

void ASTPrinter::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    os << node->literal();
}

void ASTPrinter::visitIntLiteralNode(IntLiteralNode *node) {
    os << node->literal();
}

void ASTPrinter::visitStringLiteralNode(StringLiteralNode *node) {
    std::string res = "";
    for(char c: node->literal()){
        switch (c){
            case '\n':
                res += "\\n";
                break;
            case '\r':
                res += "\\r";
                break;
            case '\t':
                res += "\\t";
                break;
            case '\v':
                res += "\\v";
                break;
            case '\a':
                res += "\\a";
                break;
            case '\b':
                res += "\\b";
                break;
            case '\f':
                res += "\\f";
                break;
            case '\'':
                res += "\\'";
                break;
            case '\"':
                res += "\\\"";
                break;
            case '\?':
                res += "\\?";
                break;
            case '\\':
                res += "\\\\";
                break;
            default:
                res += c;
        }
    }
    os << "'" << res << "'"; //todo - escape
}

void ASTPrinter::visitLoadNode(LoadNode *node) {
    os << node->var()->name();
}

void ASTPrinter::visitStoreNode(StoreNode *node) {
    os << node->var()->name() << " " << tokenOp(node->op()) << " ";
    node->value()->visit(this);
}

void ASTPrinter::visitNativeCallNode(NativeCallNode *node) {
    os << "native '" << node->nativeName() << "';\n";
}

void ASTPrinter::visitPrintNode(PrintNode *node) {
    os << "print(";
    for (int i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        if(i != node->operands()-1){
            os << ", ";
        }
    }
    os << ")";
}

void ASTPrinter::visitReturnNode(ReturnNode *node) {
    os << "return ";
    node->returnExpr()->visit(this);
}

void ASTPrinter::visitWhileNode(WhileNode *node) {
    os << "while (";
    node->whileExpr()->visit(this);
    node->loopBlock()->visit(this);
}