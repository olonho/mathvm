#include "mathvm.h"
#include "parser.h"

namespace mathvm {
    class Printer : public AstVisitor {
        std::ostream &out;
        int indent;

        void makeIdent() {
            out << string(indent, '\t');
        }

        bool notScopeNode(AstNode *node) {
            bool isScope = node->isBlockNode() || node->isForNode() || node->isWhileNode() || node->isIfNode() || node->isFunctionNode();
            return !isScope;
        }

        void printScope(Scope *scope) {
            Scope::VarIterator varIt(scope);
            while (varIt.hasNext()) {
                AstVar *var = varIt.next();
                makeIdent();
                out << typeToName(var->type()) << " ";
                out << var->name() << ";" << endl;
            }

            Scope::FunctionIterator funcIt(scope);
            while (funcIt.hasNext()) {
                AstFunction *func = funcIt.next();
                makeIdent();
                func->node()->visit(this);
            }
        }

        string escape(const string &str) {
            string res = "";
            for (uint32_t i = 0; i < str.length(); ++i) {
                switch (str.at(i)) {
                    case '\\':
                        res += "\\\\";
                        break;
                    case '\n':
                        res += "\\n";
                        break;
                    case '\t':
                        res += "\\t";
                        break;
                    case '\r':
                        res += "\\r";
                        break;
                    default:
                        res += str[i];
                }
            }
            return res;
        }

    public:
        Printer() :  out(std::cout),indent(-1) {
        }

        ~Printer() {
        }

        void visitForNode(ForNode *node) {
            out << " for (";
            out << node->var()->name();
            out << " in ";
            node->inExpr()->visit(this);
            out << ")";
            node->body()->visit(this);
        }

        void visitPrintNode(PrintNode *node) {
            out << "print (";
            for (uint32_t i = 0; i < node->operands(); ++i) {
                if (i != 0) {
                    out << ", ";
                }
                node->operandAt(i)->visit(this);
            }
            out << ")";
        }

        void visitLoadNode(LoadNode *node) {
            out << node->var()->name();
        }

        void visitIfNode(IfNode *node) {
            out << "if (";
            node->ifExpr()->visit(this);
            out << ") ";
            node->thenBlock()->visit(this);
            if (node->elseBlock()) {
                makeIdent();
                out << "else ";
                node->elseBlock()->visit(this);
            }
        }

        void visitIntLiteralNode(IntLiteralNode *node) {
            out << node->literal();
        }

        void visitDoubleLiteralNode(DoubleLiteralNode *node) {
            out << node->literal();
        }

        void visitStringLiteralNode(StringLiteralNode *node) {
            out << "\'" << escape(node->literal()) << "\'";
        }

        void visitWhileNode(WhileNode *node) {
            out << "while (";
            node->whileExpr()->visit(this);
            out << ") ";
            node->loopBlock()->visit(this);
        }


        void visitBinaryOpNode(BinaryOpNode *node) {
            out << "(";
            node->left()->visit(this);
            out << " " << tokenOp(node->kind()) << " ";
            node->right()->visit(this);
            out << ")";
        }

        void visitUnaryOpNode(UnaryOpNode *node) {
            out << tokenOp(node->kind());
            node->operand()->visit(this);
        }

        void visitNativeCallNode(NativeCallNode *node) {
            out << " native \'" << node->nativeName() << "\';" << endl;
        }

        void visitFunctionNode(FunctionNode *node) {
            out << "function ";
            out << typeToName(node->returnType()) << " ";
            out << node->name();
            out << "(";
            for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
                if (i != 0) {
                    out << ", ";
                }
                out << typeToName(node->parameterType(i)) << " ";
                out << node->parameterName(i);
            }
            out << ")";
            BlockNode *b = node->body();
            if (b->nodes() > 0 && b->nodeAt(0)->isNativeCallNode()) {
                b->nodeAt(0)->visit(this);
            } else {
                b->visit(this);
            }
        }

        void visitReturnNode(ReturnNode *node) {
            out << "return";
            if (node->returnExpr()) {
                out << " ";
                node->returnExpr()->visit(this);
            }
        }

        void visitStoreNode(StoreNode *node) {
            out << node->var()->name() << " ";
            out << tokenOp(node->op()) << " ";
            node->value()->visit(this);
        }

        void visitCallNode(CallNode *node) {
            out << node->name() << "(";
            for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
                if (i != 0) {
                    out << ", ";
                }
                AstNode *n = node->parameterAt(i);
                n->visit(this);
            }
            out << ")";
        }


        void visitBlockNode(BlockNode *node) {
            if (indent >= 0) {
                out << "{" << endl;
            }
            ++indent;
            printScope(node->scope());
            for (uint32_t i = 0; i < node->nodes(); ++i) {
                makeIdent();
                AstNode *n = node->nodeAt(i);
                bool needSemicolon = notScopeNode(n);
                n->visit(this);
                if (needSemicolon) {
                    out << ";";
                }
                out << endl;
            }
            --indent;
            if (indent >= 0) {
                makeIdent();
                out << "}" << endl;
            }
        }
    };

    class AstPrinter : public Translator {
    public:
        virtual Status *translate(const string &program, Code **code) {
            Parser parser;
            Status *status = parser.parseProgram(program);
            if (status != 0) {
                if (status->isError()) return status;
            }
            Printer printer;
            FunctionNode *root = parser.top()->node();
            root->body()->visit(&printer);
            return new Status();
        }
    };

    Translator *Translator::create(const string &impl) {
        if (impl == "printer") {
            return new AstPrinter();
        } else {
            return 0;
        }
    }
}
