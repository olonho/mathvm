#include <mathvm.h>
#include <iostream>
#include "../../../vm/parser.h"
using namespace std;

namespace mathvm {

const char* typeToName(VarType type);

class PrintVisitor : public AstVisitor {
    private:
        ostream& os;
        uint8_t indent = 0;
        uint8_t indentSize;

        void printSignature(const Signature& sig, const string& name) {
            os << "function " << typeToName(sig[0].first) << " " <<
                name << "(";
            uint32_t i;
            for (i = 1; i < sig.size() - 1; ++i) {
                os << typeToName(sig[i].first) << " " << sig[i].second << ", ";
            }
            if (i < sig.size()) {
                os << typeToName(sig[i].first) << " " << sig[i].second;
            }
            os << ")";
        }

        void printString(const string& str) {
            os << "'";
            for (auto ch : str) {
                switch (ch) {
                    case '\'':
                        os << "\\'";
                        break;
                    case '\n':
                        os << "\\n";
                        break;
                    case '\r':
                        os << "\\r";
                        break;
                    case '\t':
                        os << "\\t";
                        break;
                    case '\b':
                        os << "\\b";
                        break;
                    default:
                        os << ch;
                }
            }
            os << "'";
        }

        const string* getNativeName(FunctionNode* node) {
            BlockNode* body = node->body();
            if (body->nodes() != 2) {
                return nullptr;
            }
            NativeCallNode* native = dynamic_cast<NativeCallNode*>
                (body->nodeAt(0));
            if (!native) {
                return nullptr;
            }
            return &native->nativeName();
        }

    public:
        PrintVisitor(ostream& os, uint8_t indentSize):
        os(os), indentSize(indentSize) {}

        void visitBinaryOpNode(BinaryOpNode* node) {
            os << "(";
            node->left()->visit(this);
            os << ") ";
            os << tokenOp(node->kind());
            os << " (";
            node->right()->visit(this);
            os << ")";
        }

        void visitUnaryOpNode(UnaryOpNode* node) {
            os << tokenOp(node->kind()) << "(";
            node->operand()->visit(this);
            os << ")";
        }

        void visitStringLiteralNode(StringLiteralNode* node) {
            printString(node->literal());
        }

        void visitDoubleLiteralNode(DoubleLiteralNode* node) {
            os << node->literal();
        }

        void visitIntLiteralNode(IntLiteralNode* node) {
            os << node->literal();
        }

        void visitLoadNode(LoadNode* node) {
            os << node->var()->name();
        }

        void visitStoreNode(StoreNode* node) {
            os << node->var()->name() << " = ";
            node->value()->visit(this);
        }

        void visitForNode(ForNode* node) {
            os << "for (" << node->var()->name() << " in ";
            node->inExpr()->visit(this);
            os << ") ";
            node->body()->visit(this);
        }

        void visitWhileNode(WhileNode* node) {
            os << "while (";
            node->whileExpr()->visit(this);
            os << ") ";
            node->loopBlock()->visit(this);
        }

        void visitIfNode(IfNode* node) {
            os << "if (";
            node->ifExpr()->visit(this);
            os << ") ";
            node->thenBlock()->visit(this);
            BlockNode* el = node->elseBlock();
            if (el) {
                os << " else ";
                el->visit(this);
            }
        }

        void visitBlockNodeBraceless(BlockNode* node) {
            Scope* scope = node->scope();
            string ind = string(indent, ' ');

            Scope::VarIterator it(scope);
            while (it.hasNext()) {
                AstVar *next = it.next();
                os << ind << typeToName(next->type()) << " "
                    << next->name() << ";" << endl;
            }

            Scope::FunctionIterator fit(scope);
            while (fit.hasNext()) {
                AstFunction *next = fit.next();
                next->node()->visit(this);
                os << endl;
            }

            for (uint32_t i = 0; i < node->nodes(); ++i) {
                os << ind;
                node->nodeAt(i)->visit(this);
                os << ";" << endl;
            }
        }

        void visitBlockNode(BlockNode* node) {
            os << "{" << endl;
            indent += indentSize;
            visitBlockNodeBraceless(node);
            indent -= indentSize;
            os << endl;
            os << std::string(indent, ' ') << "}";
        }

        void visitFunctionNode(FunctionNode* node) {
            printSignature(node->signature(), node->name());
            const string *nativeName = getNativeName(node);
            if (nativeName) {
                os << " native ";
                printString(*nativeName);
                os << ";";
            } else {
                os << " ";
                node->visitChildren(this);
            }
        }

        void visitReturnNode(ReturnNode* node) {
            os << "return";
            AstNode *expr = node->returnExpr();
            if (expr) {
                os << " ";
                expr->visit(this);
            }
        }

        void visitNativeCallNode(NativeCallNode* node) {
        }

#define PRINT_CALL(fnName, paramCnt, ithParam) \
            os << fnName << "("; \
            if (paramCnt >= 1) { \
                for (uint32_t i = 0; i < paramCnt - 1; ++i) { \
                    ithParam(i)->visit(this); \
                    os << ", "; \
                } \
                ithParam(paramCnt-1)->visit(this); \
            } \
            os << ")";

        void visitCallNode(CallNode* node) {
            PRINT_CALL(node->name(), node->parametersNumber(), node->parameterAt)
        }

        void visitPrintNode(PrintNode* node) {
            PRINT_CALL("print", node->operands(), node->operandAt)
        }

#undef PRINT_CALL

};

class SourceTranslatorImpl : public Translator {
  public:
    Status* translate(const string& program, Code* *code);
};

Status* SourceTranslatorImpl::translate(const string& program, Code* *) {
    Parser parser;
    parser.parseProgram(program);
    AstFunction *node = parser.top();
    PrintVisitor visitor(cout, 4);
    visitor.visitBlockNodeBraceless(node->node()->body());

    return Status::Ok();
}

// Implement me!
Translator* Translator::create(const string& impl) {
   if (impl == "" || impl == "intepreter") {
       //return new BytecodeTranslatorImpl();
   }
   if (impl == "jit") {
       //return new MachCodeTranslatorImpl();
   }
   if (impl == "printer") {
       return new SourceTranslatorImpl();
   }
   return nullptr;
}

}  // namespace mathvm
