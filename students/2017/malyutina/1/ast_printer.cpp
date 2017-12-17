//
// Created by kate on 23.09.17.
//

#include "ast_printer.h"

#include "../../../../vm/parser.h"

#include "iostream"

using namespace std;

namespace mathvm {

    Translator *Translator::create(const string &impl) {
        if (impl == "" || impl == "printer") {
            return new ast_printer::ast_printer();
        } else {
            return nullptr;
        }
    }

    namespace ast_printer {
        ast_printer::~ast_printer() {}

        Status *ast_printer::translate(const string &program, Code **code) {
            Parser parser;
            Status *resultStatus = parser.parseProgram(program);

            if (resultStatus != nullptr && resultStatus->isError()) {
                return resultStatus;
            }

            using namespace printer;
            Printer printer;
            parser.top()->node()->visit(&printer);

            delete resultStatus;

            return Status::Ok();
        }
    }

    namespace printer {

        Printer::Printer() : out(cout), level(-1) {}

        Printer::~Printer() {}

        void Printer::visitForNode(ForNode *node) {
            out << "for (";
            out << node->var()->name();
            out << " in ";
            node->inExpr()->visit(this);
            out << ") ";
            node->body()->visit(this);
        }

        void Printer::visitPrintNode(PrintNode *node) {
            out << "print(";
            for (uint32_t i = 0; i < node->operands(); ++i) {
                node->operandAt(i)->visit(this);
                if (i != node->operands() - 1) {
                    out << ", ";
                }
            }
            out << ")";
        }

        void Printer::visitLoadNode(LoadNode *node) {
            out << node->var()->name();
        }

        void Printer::visitIfNode(IfNode *node) {

            out << "if (";
            node->ifExpr()->visit(this);
            out << ") ";
            node->thenBlock()->visit(this);
            if (node->elseBlock()) {
                makeLevel();
                out << "else ";
                node->elseBlock()->visit(this);
            }
        }

        void Printer::visitIntLiteralNode(IntLiteralNode *node) {
            out << node->literal();
        }

        void Printer::visitDoubleLiteralNode(DoubleLiteralNode *node) {
            out << node->literal();
        }

        void Printer::visitStringLiteralNode(StringLiteralNode *node) {
            out << "\'" << escape(node->literal()) << "\'";
        }

        void Printer::visitWhileNode(WhileNode *node) {
            out << "while (";
            node->whileExpr()->visit(this);
            out << ") ";
            node->loopBlock()->visit(this);
        }


        void Printer::visitBinaryOpNode(BinaryOpNode *node) {
            out << "(";
            node->left()->visit(this);
            out << " " << tokenOp(node->kind()) << " ";
            node->right()->visit(this);
            out << ")";
        }

        void Printer::visitUnaryOpNode(UnaryOpNode *node) {
            out << tokenOp(node->kind());
            node->operand()->visit(this);
        }

        void Printer::visitNativeCallNode(NativeCallNode *node) {
            out << "native \'" << node->nativeName() << "\'" << ";" << endl;
        }

        void Printer::visitFunctionNode(FunctionNode *node) {
            if (node->name() == AstFunction::top_name) {
                node->body()->visit(this);
                return;
            }
            makeLevel();
            out << "function ";
            out << typeToName(node->returnType()) << " " << node->name() << "(";
            for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
                out << typeToName(node->parameterType(i)) << " ";
                out << node->parameterName(i);
                if (i != node->parametersNumber() - 1) {
                    out << ", ";
                }
            }
            out << ") ";

            if (node->body()->nodes() == 2
                and node->body()->nodeAt(0)->isNativeCallNode()) {
                node->body()->nodeAt(0)->visit(this);
//                writeSemicolonAndNewline();
                return;
            }

            node->body()->visit(this);
        }

        void Printer::visitReturnNode(ReturnNode *node) {
            out << "return";
            if (node->returnExpr()) {
                out << " ";
                node->returnExpr()->visit(this);
            }
        }

        void Printer::visitStoreNode(StoreNode *node) {
            out << node->var()->name() << " ";
            out << tokenOp(node->op()) << " ";
            node->value()->visit(this);
        }

        void Printer::visitCallNode(CallNode *node) {
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


        void Printer::visitBlockNode(BlockNode *node) {

            if (level >= 0) {
                out << "{" << endl;
            }

            ++level;
            printScope(node->scope());
            for (uint32_t i = 0; i < node->nodes(); ++i) {
                makeLevel();
                AstNode *n = node->nodeAt(i);
                n->visit(this);
                if (notScopeNode(n)) {
                    out << ";" << endl;
                }
            }
            --level;

            if (level >= 0) {
                makeLevel();
                out << "}" << endl;
            }
        }

        void Printer::makeLevel() {
            out << string(level, ' ') << string(level, ' ') << string(level, ' ') << string(level, ' ');
        }

        bool Printer::notScopeNode(AstNode *node) {
            return node->isCallNode() || node->isLoadNode() || node->isNativeCallNode() ||
                   node->isPrintNode() || node->isReturnNode() || node->isStoreNode();
        }

        void Printer::printScope(Scope *scope) {
            Scope::VarIterator varIterator(scope);
            while (varIterator.hasNext()) {
                makeLevel();
                AstVar *var = varIterator.next();
                out << typeToName(var->type()) << " " << var->name() << ";" << endl;
            }

            Scope::FunctionIterator funcIterator(scope);
            while (funcIterator.hasNext()) {
                funcIterator.next()->node()->visit(this);
            }
        }

        string Printer::escape(const string &str) {
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
    }
}
