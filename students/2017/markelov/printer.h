#pragma once
#include <string>
#include "parser.h"

namespace mathvm {

static inline std::string specString(const string &src) {
    std::string res = "'";
    for (auto c : src) {
        switch (c) {
        case '\n':
            res  += "\\n";
            break;
        case '\t':
            res += "\\t";
            break;
        case '\b':
            res += "\\b";
            break;
        case '\0':
            res = "\\0";
            break;
        default:
            res += c;
        }
    }
    res += "'";
    return res;
}

class PrettyPrinter: public AstVisitor {
    int identation = -ID;
    static const int ID = 4;
    int top = 1;

    bool atTop() {
        return top >= 0;
    }

    void CALL_VISIT(AstNode * node, bool spec = false) {
        if (spec)
            identation += ID;
        top --;
        if (node != nullptr)
            node->visit(this);

        if (spec)
            identation -= ID;
        top ++;
    }

    ostream& PR(void) { return std::cout; };
    ostream& PRN(void) {
        return PR() << std::string(identation, ' ');
    }

    ostream& type(ostream&a, VarType t) {
        return a << typeToName(t) << " ";
    }

    bool isNative(FunctionNode * node) {
        for (uint32_t i = 0; i < node->body()->nodes(); i++) {
            if (dynamic_cast<NativeCallNode*>(node->body()->nodeAt(i)) != nullptr)
                return true;
       }
    return false;
    }

    std::string getNativeName(FunctionNode * node) {
        for (uint32_t i = 0; i < node->body()->nodes(); i++) {
            NativeCallNode* n = dynamic_cast<NativeCallNode*>(node->body()->nodeAt(i));
           if (n != nullptr)
               return n->nativeName();
        }
        return "";
    }

public:
    PrettyPrinter(AstFunction * top) {
        CALL_VISIT(top->node());
    }




    void visitFunctionNode(FunctionNode* node) {
        if (!atTop()) {
            if (!isNative(node))
                PRN() << "function ";
            else
                PR() << "function ";
            type(PR(), node->returnType());
            PR() << node->name() << " (";
            for (uint32_t i = 0; i < node->parametersNumber(); i++) {
                type(PR(), node->parameterType(i));
                PR() << node->parameterName(i);
                if (i < node->parametersNumber() - 1)
                    PR() << ", ";
            }
            PR() << ") ";

            if (!isNative(node))
                PR() << "{\n";
            else
                PR() << "native '" << getNativeName(node) << "';\n";
        }

        if (!isNative(node)) {
            CALL_VISIT(node->body(), true);

            if (!atTop())
                PRN() << "}\n";
        }
    }

    void visitBlockNode(BlockNode* node) {
        {
            Scope::VarIterator it(node->scope());
            while (it.hasNext()) {
                AstVar * f = it.next();
                type(PRN(), f->type());
                PR() << f->name() << ";\n";
            }
        }
        Scope::FunctionIterator it(node->scope());
        while (it.hasNext()) {
            AstFunction* f = it.next();
            CALL_VISIT(f->node(), true);
        }


        for (uint32_t i = 0; i < node->nodes(); i++) {
            PRN() << "";
            CALL_VISIT(node->nodeAt(i), true);
            PR() << "\n";
        }
    }

    void visitBinaryOpNode(BinaryOpNode* node) {
        PR() << "(";
        CALL_VISIT(node->left(), true);
        PR() << " " << tokenOp(node->kind()) << " ";
        CALL_VISIT(node->right(), true);
        PR() << ")";
    }

    void visitLoadNode(LoadNode* node) {
        const AstVar * var = node->var();
        PR() << var->name();
    }


    void visitStoreNode(StoreNode* node) {
        const AstVar * var = node->var();
        PR() << var->name() << " = " ;
        CALL_VISIT(node->value());
        PR() << ";";
    }

    void visitPrintNode(PrintNode* node) {
        PR() << "print (";
        std::vector<VarType> operands_types;
        for (uint64_t i = 0; i < node->operands(); i++) {
            CALL_VISIT(node->operandAt(i));
            if (i < node->operands() - 1)
                PR() << ", ";
        }
        PR() << ");";
    }
    void visitStringLiteralNode(StringLiteralNode* node) {
        PR() << specString(node->literal());
    }

    void visitIntLiteralNode(IntLiteralNode* node) {
        PR() << node->literal();
    }
    void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        PR() << node->literal();
    }

    void visitUnaryOpNode(UnaryOpNode* node) {
        PR() << tokenOp(node->kind()) ; CALL_VISIT(node->operand());
    }

    void visitIfNode(IfNode* node) {
        PR() << "if " << "(" ; CALL_VISIT(node->ifExpr()); PR() << ") {\n";
        CALL_VISIT(node->thenBlock(), true);
        PRN() << "} else {\n";
        CALL_VISIT(node->elseBlock(), true);
        PRN() << "}\n";
    }

    void visitWhileNode(WhileNode* node) {
        PR() << "while " << "(" ; CALL_VISIT(node->whileExpr()); PR() << ") {\n";
        CALL_VISIT(node->loopBlock(), true);
        PRN() << "}\n";
    }

    void visitForNode(ForNode* node) {
        BinaryOpNode * range = dynamic_cast<BinaryOpNode *>(node->inExpr());
        PR() << "for " << "("  << node->var()->name() <<" in ";
                CALL_VISIT(range->left());
        PR() << ".." ;
        CALL_VISIT(range->right());
        PR()<< ") {\n";
        CALL_VISIT(node->body(), true);
        PRN() << "}\n";
    }

    void visitReturnNode(ReturnNode* node) {
        PR() << "return "; CALL_VISIT(node->returnExpr());
        PR() << ";";
    }
    void visitCallNode(CallNode * node) {
        PR() << node->name() << " ( ";
        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
            CALL_VISIT(node->parameterAt(i));
            if (i < node->parametersNumber() - 1)
                PR() << ", ";
        }
        PR() << ")";
    }

    void visitNativeCallNode(NativeCallNode * node) {

    }
};

}
