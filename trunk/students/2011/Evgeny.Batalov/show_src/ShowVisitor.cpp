#include <iostream>
#include <sstream>

#include "ShowVisitor.h"

ShowVisitor::ShowVisitor(std::ostream& o, int tabs_)
    : need_tabs(false)
    , prec(0)
    , level(0)
    , stream(o)
    , tabs(tabs_)
    {}

//I WROTE ALL THE UNDER FOR FUN
//OBFUSCATION ;)
#define PRINT_AST_VAR(VAR) stream << VAR->name() << 

#define PRINT_AST_TYPE(VAR) \
switch(VAR->type()) { \
    case mathvm::VT_DOUBLE: \
        stream << "double"; \
    break; \
    case mathvm::VT_INT: \
        stream << "int"; \
    break;\
    case mathvm::VT_STRING: \
        stream << "string"; \
    break;\
    default: \
        stream << std::endl << "---ERROR---" << std::endl; \
}\
stream << 

#define NEW_LINE std::endl; need_tabs = true;

#define TABS \
if (need_tabs) {\
    for(int i = 0; i < level; ++i)\
        for(int t = 0; t < tabs; ++t)\
        stream << " ";\
    need_tabs = false;\
}

#define BLOCK(NODE) \
if (NODE->isBlockNode()) {\
stream << " {" << NEW_LINE\
    ++level;\
    NODE->visit(this);\
    --level;\
    stream << "}" << NEW_LINE;\
} else {\
    stream << NEW_LINE\
    NODE->visit(this);\
}

//StringTransformer generates transformation alg for string literal
//from BadLetterList which stores LetterPairs of char and char*
//We replace every LetterPair::first by its LetterPair::second
template<int i>
struct LetterPair{ static char const first; static char const * const second; };

template<> char const LetterPair<0>::first = '\a';
template<> char const * const LetterPair<0>::second = "\\a";
template<> char const LetterPair<1>::first = '\b';
template<> char const * const LetterPair<1>::second = "\\b";
template<> char const LetterPair<2>::first = '\t';
template<> char const * const LetterPair<2>::second = "\\t";
template<> char const LetterPair<3>::first = '\n';
template<> char const * const LetterPair<3>::second = "\\n";
template<> char const LetterPair<4>::first = '\f';
template<> char const * const LetterPair<4>::second = "\\f";
template<> char const LetterPair<5>::first = '\r';
template<> char const * const LetterPair<5>::second = "\\r";
template<> char const LetterPair<6>::first = '\'';
template<> char const * const LetterPair<6>::second = "\\'";
template<> char const LetterPair<7>::first = '\\';
template<> char const * const LetterPair<7>::second = "\\\\";

struct nil{};
template<typename LPAIR, typename TAIL>
struct cons {
    typedef TAIL T;
    typedef LPAIR P;
};

typedef cons<LetterPair<7>, cons<LetterPair<6>, cons<LetterPair<5>, cons<LetterPair<4>,
        cons<LetterPair<3>, cons<LetterPair<2>, cons<LetterPair<1>, cons<LetterPair<0>, 
        nil> > > > > > > > BadLetterList;


template<typename LETTER_LIST>
struct StringTransformer {
     static void transform(std::string& s) {
        struct ERROR_IN_CODE;
        ERROR_IN_CODE err;
    }
};

template<typename LPAIR, typename TAIL>
struct StringTransformer<cons<LPAIR, TAIL> > { 
    static void transform(std::string& s) {

        size_t pos = 0;
        for(;;) {
            pos = s.find(LPAIR::first, pos);
            if (pos == std::string::npos)
                break;
            s.replace(pos, 1, LPAIR::second);
        }
        StringTransformer<typename TAIL::T>::transform(s);
    }
};

template<>
struct StringTransformer<nil> {

    static void transform(std::string& s) {
        //end of transformation!
    }
};

void ShowVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {    
    TABS
    int prev_prec = prec;
    prec = tokenPrecedence(node->kind());
    if (prev_prec > prec) stream << "(";

    node->left()->visit(this);
    stream << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
    if (prev_prec > prec) stream << ")";
    prec = prev_prec;
}

void ShowVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    TABS
    int prev_prec = prec;
    prec = 9999999;
    stream << tokenOp(node->kind());
    node->visitChildren(this);
    prec = prev_prec;
}

void ShowVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
    TABS
    std::string literal = node->literal();
    StringTransformer<BadLetterList>::transform(literal);
    stream << "'" << literal << "'";
    node->visitChildren(this);
}

void ShowVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
    TABS
    std::stringstream str;
    str << node->literal();
    std::string s = str.str();
    size_t pos = s.find('e');
    if (pos != std::string::npos) {
        if (s[pos + 1] == '+')
            s.erase(pos + 1, 1);
    }
    stream << s;
    if (s.find("e") == std::string::npos && 
        s.find(".") == std::string::npos) 
        stream << ".0";
    node->visitChildren(this);
}

void ShowVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
    TABS
    stream << node->literal();
    node->visitChildren(this);
}

void ShowVisitor::visitLoadNode(mathvm::LoadNode* node) {
    TABS
    PRINT_AST_VAR(node->var())"";
}

void ShowVisitor::visitStoreNode(mathvm::StoreNode* node) {
    TABS
    PRINT_AST_VAR(node->var()) tokenOp(node->op());
    node->visitChildren(this);
    stream << ";" << NEW_LINE;
}

void ShowVisitor::visitForNode(mathvm::ForNode* node) {
    TABS
    stream << "for (";
    PRINT_AST_VAR(node->var())" in ";
    node->inExpr()->visit(this);
    stream << ")";
    BLOCK(node->body());
}

void ShowVisitor::visitWhileNode(mathvm::WhileNode* node) {
    TABS
    stream << "while (";
    node->whileExpr()->visit(this);
    stream << ") ";
    BLOCK(node->loopBlock())
}

void ShowVisitor::visitIfNode(mathvm::IfNode* node) {
    TABS
    stream << "if (";
    node->ifExpr()->visit(this);
    stream << ") ";
    BLOCK(node->thenBlock())
    if (node->elseBlock()) {
        stream << " else ";
        BLOCK(node->elseBlock())
    }
}

void ShowVisitor::visitBlockNode(mathvm::BlockNode* node) {
    TABS
    mathvm::Scope::VarIterator it(node->scope());
    while(it.hasNext()) {
        mathvm::AstVar* curr = it.next();
        PRINT_AST_TYPE(curr) " ";
        PRINT_AST_VAR(curr) ";" << NEW_LINE;
    }
    node->visitChildren(this);
}

void ShowVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
    TABS
    stream << "function" << node->name() << "(";
     node->args()->visit(this);
     stream << ") ";
     BLOCK(node->body());

}

void ShowVisitor::visitPrintNode(mathvm::PrintNode* node) {
    TABS
    stream << "print (";
    for (uint32_t i = 0; i < node->operands() - 1; ++i) {
        node->operandAt(i)->visit(this);
        stream << ", ";
    }
    if ( (node->operands() - 1) >= 0) 
        node->operandAt(node->operands() - 1)->visit(this);
    stream << ");" << NEW_LINE;
}
