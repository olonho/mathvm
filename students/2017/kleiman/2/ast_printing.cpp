#include "include/ast_printing.h"
#include "ast.h"
#include "parser.h"
#include "iostream"

namespace mathvm {
    class PrintCode : public AstVisitor
    {
    public:
        void visitBinaryOpNode(BinaryOpNode* node)
        {
            std::cout << "(";
            node->left()->visit(this);
            std::cout << ' ' << tokenOp(node->kind()) << ' ';
            node->right()->visit(this);
            std::cout << ")";
        }

        void visitUnaryOpNode(UnaryOpNode* node)
        {
            std::cout << tokenOp(node->kind());
            node->operand()->visit(this);
        }

        void visitStringLiteralNode(StringLiteralNode* node)
        {
            std::string tmp = node->literal();
            std::cout << "'";
            for (size_t i = 0; i < tmp.size(); ++i)
            {
                switch (tmp[i])
                {
                    case '\r':
                        std::cout << "\\r";
                        break;
                    case '\n':
                        std::cout << "\\n";
                        break;
                    case '\\':
                        std::cout << "\\\\";
                        break;
                    case '\t':
                        std::cout << "\\t";
                        break;
                    case '\a':
                        std::cout << "\\a";
                        break;
                    case '\'':
                        std::cout << "\\\'";
                        break;
                    case '\v':
                        std::cout << "\\v";
                        break;
                    case '\f':
                        std::cout << "\\f";
                        break;
                    case '\0':
                        std::cout << "\\0";
                        break;
                    case '\b':
                        std::cout << "\\b";
                        break;
                    default:
                        std::cout << tmp[i];
                        break;
                }
            }
            std::cout << "'";
        }

        void visitIntLiteralNode(IntLiteralNode* node)
        {
            std::cout << node->literal();
        }

        void visitDoubleLiteralNode(DoubleLiteralNode* node)
        {
            std::cout << node->literal();
        }

        void visitLoadNode(LoadNode* node)
        {
            std::cout << node->var()->name();
        }

        void visitStoreNode(StoreNode* node)
        {
            std::cout << node->var()->name();
            std::cout << ' ' << tokenOp(node->op()) << ' ';
            node->value()->visit(this);
        }

        void visitBlockNode(BlockNode* node)
        {
            for (Scope::VarIterator it(node->scope()); it.hasNext(); )
            {
                printSpace();
                auto var = it.next();
                std::cout << getType(var->type());
                std::cout << " " << var->name();
                std::cout << ";" << std::endl;
            }
            for (Scope::FunctionIterator it(node->scope()); it.hasNext(); )
            {
                printSpace();
                it.next()->node()->visit(this);
                std::cout << std::endl;
            }
            for (size_t i = 0; i < node->nodes(); ++i) 
            {
                printSpace();
                AstNode *cld = node->nodeAt(i);
                cld->visit(this);
                if (dynamic_cast<ForNode*>(cld) == nullptr &&
                    dynamic_cast<WhileNode*>(cld) == nullptr &&
                    dynamic_cast<IfNode*>(cld) == nullptr &&
                    dynamic_cast<ReturnNode*>(cld) == nullptr)
                {
                    std::cout << ";" << std::endl;
                } else {
                    std::cout << std::endl;
                }
            }      
        }

        void visitNativeCallNode(NativeCallNode* node)
        {
            std::cout << "native '" << node->nativeName() << "'";
        }

        void visitForNode(ForNode* node)
        {
            std::cout << "for (";
            std::cout << node->var()->name();
            std::cout << " in ";
            node->inExpr()->visit(this);
            std::cout << ") {" << std::endl;
            offset++;
            node->body()->visit(this);
            offset--;
            printSpace();
            std::cout << "}";
        }

        void visitWhileNode(WhileNode* node)
        {
            std::cout << "while (";
            node->whileExpr()->visit(this);
            std::cout << ") {" << std::endl;
            offset++;
            node->loopBlock()->visit(this);
            offset--;
            printSpace();
            std::cout << "}";
        }

        void visitIfNode(IfNode* node)
        {
            std::cout << "if (";
            node->ifExpr()->visit(this);
            std::cout << ") {" << std::endl;
            offset++;
            node->thenBlock()->visit(this);
            offset--;
            printSpace();
            std::cout << "}";
            if (node->elseBlock() != nullptr) 
            {
                std::cout << " else {" << std::endl;
                offset++;
                node->elseBlock()->visit(this);
                offset--;
                printSpace();
                std::cout << "}";
            }
        }

        void visitReturnNode(ReturnNode* node)
        {
            if (node->returnExpr()) 
            {
                std::cout << "return";
                std::cout << " ";
                node->returnExpr()->visit(this);
                std::cout << ";";
            }
        }

        void visitFunctionNode(FunctionNode* node)
        {
            if (node->name() == "<top>")
            {
                node->body()->visit(this);
                return;
            }
            std::cout << "function " << getType(node->returnType());
            std:: cout << " " << node->name() << "(";
            for (uint32_t i = 0; i < node->parametersNumber(); ++i) 
            {
                std::cout << getType(node->parameterType(i));
                std::cout << " " << node->parameterName(i);
                if (i != node->parametersNumber() - 1) 
                    std::cout << ", ";
            }
            if (node->body()->nodes() == 2 && node->body()->nodeAt(0)->isNativeCallNode())
            {
                std::cout << ") ";
                node->body()->visit(this);
            } else
            {
                std::cout << ") {" << std::endl;
                offset++;
                node->body()->visit(this);
                offset--;
                printSpace();
                std::cout << "}";
            }
        }

        void visitCallNode(CallNode* node)
        {
            std::cout << node->name() << "(";
            for (size_t i = 0; i < node->parametersNumber(); ++i)
            {
                node->parameterAt(i)->visit(this);
                if (i != node->parametersNumber() - 1)
                    std::cout << ", ";
            }
            std::cout << ")";
        }

        void visitPrintNode(PrintNode* node)
        {
            std::cout << "print(";
            for (size_t i = 0; i < node->operands(); ++i)
            {
                node->operandAt(i)->visit(this);
                if (i != node->operands() - 1)
                    std::cout << ", ";
            }
            std::cout << ")";
        }
    private:
        size_t offset = 0;
        size_t spaceSize = 4;

        void printSpace()
        {
            for (size_t i = 0; i < offset * spaceSize; ++i)
            {
                std::cout << ' ';
            }
        }
        
        std::string getType(VarType type)
        {
            switch (type) 
            {
                case VT_VOID: 
                    return "void"; 
                case VT_DOUBLE: 
                    return "double";
                case VT_INT: 
                    return "int";
                case VT_STRING: 
                    return "string";
                case VT_INVALID:
                    return "INVALID";
            }
            return "";
        }
    };

    Status* ProgramTranslatorImpl::translate(const string &program, Code **code)
    {
        Parser pr;
        Status* st = pr.parseProgram(program);
        if (st->isError())
            return st;
        PrintCode printVisitor;
        pr.top()->node()->visit(&printVisitor);
        return st;
    }
}