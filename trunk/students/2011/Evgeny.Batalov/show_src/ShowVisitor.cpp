#include <iostream>
#include <sstream>

#include "ShowVisitor.h"

const int sw = 4;

ShowVisitor::ShowVisitor(std::ostream& o)
    :  need_sw(false)
    ,  prec(0)
    ,  level(0)
    ,  stream(o)
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

//let us generate transformation for our string from type BadLetterList!
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
    node->left()->visit(this);
    stream << tokenOp(node->kind());
    node->right()->visit(this);
}

void ShowVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    stream << tokenOp(node->kind());
    node->visitChildren(this);
}

void ShowVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
    std::string literal = node->literal();
    StringTransformer<BadLetterList>::transform(literal);
    stream << "'" << literal << "'";
    node->visitChildren(this);
}

void ShowVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
    stream << node->literal();
    node->visitChildren(this);
}

void ShowVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
    stream << node->literal();
    node->visitChildren(this);
}

void ShowVisitor::visitLoadNode(mathvm::LoadNode* node) {
    PRINT_AST_VAR(node->var())"";
}

void ShowVisitor::visitStoreNode(mathvm::StoreNode* node) {
    PRINT_AST_VAR(node->var())"";
    node->visitChildren(this);
}

void ShowVisitor::visitForNode(mathvm::ForNode* node) {
    PRINT_AST_VAR(node->var())"";
    node->visitChildren(this);
}

void ShowVisitor::visitWhileNode(mathvm::WhileNode* node) {
    stream << " " << "while()" << " ";
    node->visitChildren(this);
}

void ShowVisitor::visitIfNode(mathvm::IfNode* node) {
    stream << "If ()" << " ";
    node->visitChildren(this);
}

void ShowVisitor::visitBlockNode(mathvm::BlockNode* node) {
    stream << " " << "{ }" << " ";
    mathvm::Scope::VarIterator it(node->scope());
    while(it.hasNext()) {
        mathvm::AstVar* curr = it.next();
        PRINT_AST_TYPE(curr) " ";
        PRINT_AST_VAR(curr) ";" << std::endl;
    }
    node->visitChildren(this);
}

void ShowVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
    stream << " " << node->name() << " ";
    node->visitChildren(this);
}

void ShowVisitor::visitPrintNode(mathvm::PrintNode* node) {
    stream << "print (";
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        stream << ", ";
    }
    stream << ")" << std::endl;
}
