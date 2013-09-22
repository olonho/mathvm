#include "parser.h"
#include "ast.h"

#include <iostream>
#include <sstream>

#define TAB_SIZE 4

using namespace mathvm;

class AstConverter : public AstVisitor {
public:
    AstConverter() : level(0), code("") {
    }

    std::string restoreCode(const AstFunction *root) {
        level = 0;
        printBlockNode(root->node()->body());
        return code.str();
    }

    //----------------------------------------------------------------

    void visitBlockNode(BlockNode *node) {
        ++level;
        code << " {" << std::endl;
        printBlockNode(node);
        code << std::string(--level * TAB_SIZE, ' ') << "}";
    }

    //----------------------------------------------------------------

    void visitFunctionNode(FunctionNode *node) {
        code << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
        for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if(i > 0) code << ", ";
            code << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        }
        code << ")";
        if(node->body()->nodeAt(0)->isNativeCallNode()) node->body()->nodeAt(0)->visit(this);
        else node->body()->visit(this);
        code << std::endl;
    }

    void visitNativeCallNode(NativeCallNode *node) {
        code << " native '" << node->nativeName() << "';";
    }

    void visitCallNode(CallNode *node) {
        code << node->name() << "(";
        for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if(i > 0) code << ", ";
            node->parameterAt(i)->visit(this);
        }
        code << ")";
    }

    //----------------------------------------------------------------

    void visitUnaryOpNode(UnaryOpNode *node) {
        code << tokenOp(node->kind());
        bool p = node->operand()->isBinaryOpNode() || node->operand()->isUnaryOpNode();
        if(p) code << "(";
        node->operand()->visit(this);
        if(p) code << ")";
    }

    void visitBinaryOpNode(BinaryOpNode *node) {
        bool leftParens = needParens(node, node->left());
        bool rightParens = needParens(node, node->right());
        bool opSpaces = node->kind() != tRANGE;
        code << (leftParens ? "(" : "");
        node->left()->visit(this);
        code << (leftParens ? ")" : "") << (opSpaces ? " " : "");
        code << tokenOp(node->kind());
        code << (opSpaces ? " " : "") << (rightParens ? "(" : "");
        node->right()->visit(this);
        code << (rightParens ? ")" : "");
    }

    //----------------------------------------------------------------

    void visitIntLiteralNode(IntLiteralNode *node) {
        code << node->literal();
    }

    void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        code << node->literal();
    }

    void visitStringLiteralNode(StringLiteralNode *node) {
        code << "'";
        for(std::string::const_iterator c = node->literal().begin(); c != node->literal().end(); ++c) {
            switch(*c) {
            case '\n': code << "\\n"; break;
            case '\r': code << "\\r"; break;
            case '\t': code << "\\t"; break;
            case '\\': code << "\\\\"; break;
            default: code << *c;
            }
        }
        code << "'";
    }

    void visitLoadNode(LoadNode *node) {
        code << node->var()->name();
    }

    void visitStoreNode(StoreNode *node) {
        code << node->var()->name() << " " << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    //----------------------------------------------------------------

    void visitForNode(ForNode *node) {
        code << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        code << ")";
        node->body()->visit(this);
    }

    void visitWhileNode(WhileNode *node) {
        code << "while (";
        node->whileExpr()->visit(this);
        code << ")";
        node->loopBlock()->visit(this);
    }

    void visitIfNode(IfNode *node) {
        code << "if (";
        node->ifExpr()->visit(this);
        code << ")";
        node->thenBlock()->visit(this);
        if(node->elseBlock()) {
            code << " else";
            node->elseBlock()->visit(this);
        }
    }

    void visitReturnNode(ReturnNode *node) {
        code << "return";
        if(node->returnExpr()) {
            code << " ";
            node->returnExpr()->visit(this);
        }
    }

    void visitPrintNode(PrintNode *node) {
        code << "print(";
        for(uint32_t i = 0; i < node->operands(); ++i) {
            if(i > 0) code << ", ";
            node->operandAt(i)->visit(this);
        }
        code << ")";
    }

    //----------------------------------------------------------------

private:
    bool needParens(const BinaryOpNode *current, const AstNode *child) {
        const BinaryOpNode *bchild = dynamic_cast<const BinaryOpNode*>(child);
        if(bchild) {
            if(current->kind() == tRANGE) return true;
            return tokenPrecedence(current->kind()) > tokenPrecedence(bchild->kind());
        }
        const UnaryOpNode *uchild = dynamic_cast<const UnaryOpNode*>(child);
        if(uchild) {
            if(current->kind() == tRANGE) return true;
            return tokenPrecedence(current->kind()) > tokenPrecedence(uchild->kind());
        }
        return false;
    }

    void printBlockNode(BlockNode *block) {
        std::string tab(level * TAB_SIZE, ' ');

        Scope::VarIterator vars(block->scope());
        while(vars.hasNext()) {
            AstVar *v = vars.next();
            code << tab << typeToName(v->type()) << " " << v->name() << ";" << std::endl;
        }

        Scope::FunctionIterator funcs(block->scope());
        while(funcs.hasNext()) funcs.next()->node()->visit(this);

        for(uint32_t i = 0; i < block->nodes(); ++i) {
            code << tab;
            AstNode *node = block->nodeAt(i);
            node->visit(this);
            if(!node->isForNode() && !node->isWhileNode() && !node->isIfNode()) code << ";";
            code << std::endl;
        }
    }

    unsigned int level;
    std::ostringstream code;
};

/********************************************************************/

int main(int argc, char *argv[]) {
    if(argc != 2) {
        std::cout << "Usage: astprint <program.mvm>" << std::endl;
        return -1;
    }

    char *code = mathvm::loadFile(argv[1]);
    if(!code) {
        std::cout << "Unable to open file: " << argv[1] << std::endl;
        return -1;
    }

    mathvm::Parser p;
    mathvm::Status *status = p.parseProgram(code);
    if(status && status->isError()) {
        std::cout << "Unable to parse file " << argv[1] << ": " << status->getError() << std::endl;
        return -1;
    }

    AstConverter c;
    std::cout << c.restoreCode(p.top()) << std::endl;
    
    return 0;
}
