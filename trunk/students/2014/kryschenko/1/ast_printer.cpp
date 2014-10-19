#include "mathvm.h"
#include "ast.h"
#include "parser.h"

using std::ostream;
using std::cout;
using std::endl;

namespace mathvm {

    class AstPrinterVisitor : public AstVisitor {

        ostream & out;
        int currentTabNum;

        void addIndent() {
            for (int i = 0; i < currentTabNum; ++i) {
                out<< "    ";
            }

        }

        string escape(string const & str) {
            string res = "";
            for (size_t i = 0; i < str.size(); ++i) {
                switch (str[i]) {
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

        void visitScope(Scope * scope) {
            Scope::VarIterator varIter(scope);
            while (varIter.hasNext()) {
                addIndent();
                AstVar * var = varIter.next();
                out<< typeToName(var->type())<< " "<< var->name()<< ";"<< endl;

            }
            Scope::FunctionIterator funcIter(scope);
            while (funcIter.hasNext()) {

                funcIter.next()->node()->visit(this);
            }
        }

    public:
        AstPrinterVisitor(ostream & out) : out(out), currentTabNum(-1) {

        }

        virtual ~AstPrinterVisitor() {

        }

        virtual void visitBinaryOpNode(BinaryOpNode * node) {
            out<< "(";
            node->left()->visit(this);
            out<< " "<< tokenOp(node->kind())<< " ";
            node->right()->visit(this);
            out<< ")";
        }

        virtual void visitUnaryOpNode(UnaryOpNode * node) {
            out<< tokenOp(node->kind());
            node->operand()->visit(this);
        }

        virtual void visitIntLiteralNode(IntLiteralNode * node) {
            out<< node->literal();
        }

        virtual void visitDoubleLiteralNode(DoubleLiteralNode * node) {
            out<< node->literal();
        }

        virtual void visitStringLiteralNode(StringLiteralNode * node) {

            out<< "'"<< escape(node->literal())<< "'";
        }

        virtual void visitLoadNode(LoadNode * node) {
            out<< node->var()->name();
        }

        virtual void visitStoreNode(StoreNode * node) {
            out<< node->var()->name()<< " "<< tokenOp(node->op())<< " ";
            node->value()->visit(this);
        }

        virtual void visitIfNode(IfNode * node) {
            out<< "if (";
            node->ifExpr()->visit(this);
            out<< ") ";
            node->thenBlock()->visit(this);
            if (node->elseBlock()) {
                out<< "else ";
                node->elseBlock()->visit(this);
            }
        }

        virtual void visitForNode(ForNode * node) {
            out<< "for (";
            out<< node->var()->name();
            out<< " in ";
            node->inExpr()->visit(this);
            out<< ") ";
            node->body()->visit(this);
        }

        virtual void visitWhileNode(WhileNode * node) {
            out<< "while (";
            node->whileExpr()->visit(this);
            out<< ") ";
            node->loopBlock()->visit(this);
        }

        virtual void visitBlockNode(BlockNode * node) {
            if (currentTabNum >= 0) {
                out<< "{"<< endl;
            }
            ++currentTabNum;

            visitScope(node->scope());
            for (size_t i = 0; i < node->nodes(); ++i) {
                addIndent();
                AstNode * currentNode = node->nodeAt(i);
                currentNode->visit(this);
                if (currentNode->isCallNode() ||
                        currentNode->isLoadNode() ||
                        currentNode->isNativeCallNode() ||
                        currentNode->isPrintNode() ||
                        currentNode->isReturnNode() ||
                        currentNode->isStoreNode()) {

                    out<< ";"<< endl;
                }
            }

            --currentTabNum;
            if (currentTabNum >= 0) {
                addIndent();
                out<< "}"<< endl;
            }


        }



        virtual void visitFunctionNode(FunctionNode * node) {
            if (node->name() != AstFunction::top_name) {
                addIndent();
                out<< "function ";
                out<< typeToName(node->returnType())<< " ";
                out<< node->name()<< "(";
                for (size_t i = 0; i < node->parametersNumber(); ++i) {
                    out<< typeToName(node->parameterType(i));
                    if (i + 1 < node->parametersNumber()) {
                        out<< ", ";
                    }
                }
                out<< ") ";

            }
            node->body()->visit(this);
        }

        virtual void visitReturnNode(ReturnNode * node) {
            out<< "return ";
            if (node->returnExpr()) {
                node->returnExpr()->visit(this);
            }
        }

        virtual void visitNativeCallNode(NativeCallNode * node) {
            out<< "native '"<< node->nativeName()<< "'";
        }

        virtual void visitCallNode(CallNode * node) {
            out<< node->name()<< "(";
            for (size_t i = 0; i < node->parametersNumber(); ++i) {
                node->parameterAt(i)->visit(this);
                if (i + 1 < node->parametersNumber()) {
                    out<< ", ";
                }
            }
            out<< ") ";
        }

        virtual void visitPrintNode(PrintNode * node) {
            out<< "print(";
            for (size_t i = 0; i < node->operands(); ++i) {
                node->operandAt(i)->visit(this);
                if (i + 1 < node->operands()) {
                    out<< ",  ";
                }
            }
            out<< ")";
        }
    };

    class AstPrinter : public Translator {
    public:
        Status * translate(string const & program, Code * * code) {
            Parser parser;
            Status * status = parser.parseProgram(program);
            if (status && status->isError()) {
                return status;
            }
            AstPrinterVisitor printer(cout);
            parser.top()->node()->visit(&printer);
            return new Status();
        };

    };

    Translator * Translator::create(string const & impl) {
        if (impl == "printer") {
            return new AstPrinter();

        } else {
            return NULL;
        }
    }
}

