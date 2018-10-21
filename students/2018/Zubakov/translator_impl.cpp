#include <mathvm.h>
#include <visitors.h>
#include <parser.h>

namespace mathvm {

    class PrintVisitor : public AstBaseVisitor {
    private:
        ostream &os;
        const uint16_t _step;
        uint16_t _indent;

        void printSignatureElement(SignatureElement const &sign) {
            os << typeToName(sign.first) << " " << sign.second;
        }

        void printIndent() {
            for (uint16_t i = 0; i < _indent; ++i) {
                os << " ";
            }
        }

        void printString(string const &st) {
            for (auto ch : st) {
                switch (ch) {
                    case '\n':
                        std::cout << "\\n";
                        break;
                    case '\r':
                        std::cout << "\\r";
                        break;
                    case '\t':
                        std::cout << "\\t";
                        break;
                    case '\b':
                        std::cout << "\\b";
                        break;
                    default:
                        std::cout << ch;
                        break;
                }
            }
        }

    public:
        PrintVisitor(ostream &os, uint16_t _step, uint16_t _indend) : os(os), _step(_step), _indent(_indend) {}

        ~PrintVisitor() override {

        }

        void visitForNode(ForNode *node) override {
            os << "for (" << node->var()->name() << " in ";
            node->inExpr()->visit(this);
            os << ") ";
            node->body()->visit(this);
        }

        void visitPrintNode(PrintNode *node) override {
            os << "print (";
            uint32_t numOfParams = node->operands();
            if (numOfParams == 0) {
                os << ")";
                return;
            }

            for (uint32_t i = 0; i < numOfParams - 1; ++i) {
                node->operandAt(i)->visit(this);
                os << ", ";
            }

            node->operandAt(numOfParams - 1)->visit(this);
            os << ")";
        }

        void visitLoadNode(LoadNode *node) override {
            os << node->var()->name();
        }

        void visitIfNode(IfNode *node) override {
            os << " if (";
            node->ifExpr()->visit(this);
            os << ")";
            node->thenBlock()->visit(this);
            if (node->elseBlock() != nullptr) {
                os << " else ";
                node->elseBlock()->visit(this);
            }

        }

        void visitBinaryOpNode(BinaryOpNode *node) override {
            os << "(";
            node->left()->visit(this);
            os << ")";
            os << tokenOp(node->kind());
            os << "(";
            node->right()->visit(this);
            os << ")";
        }

        void visitCallNode(CallNode *node) override {
            os << node->name() << "(";
            uint32_t numOfParams = node->parametersNumber();
            if (numOfParams == 0) {
                os << ")";
                return;
            }

            for (uint32_t i = 0; i < numOfParams - 1; ++i) {
                node->parameterAt(i)->visit(this);
                os << ", ";
            }

            node->parameterAt(numOfParams - 1)->visit(this);
            os << ")";
        }

        void visitDoubleLiteralNode(DoubleLiteralNode *node) override {
            os << node->literal();
        }

        void visitStoreNode(StoreNode *node) override {
            os << node->var()->name() << " " << tokenOp(node->op()) << " ";
            node->value()->visit(this);
        }

        void visitStringLiteralNode(StringLiteralNode *node) override {
            os << "'";
            printString(node->literal());
            os << "'";
        }

        void visitWhileNode(WhileNode *node) override {
            os << " while (";
            node->whileExpr()->visit(this);
            os << ")";
            node->loopBlock()->visit(this);
        }

        void visitIntLiteralNode(IntLiteralNode *node) override {
            os << node->literal();
        }

        void visitUnaryOpNode(UnaryOpNode *node) override {
            os << tokenOp(node->kind()) << "(";
            node->operand()->visit(this);
            os << ")";
        }

        void visitNativeCallNode(NativeCallNode *node) override {
            std::cout << " native '" << node->nativeName() << "'";
        }

        void visitBlockNode(BlockNode *node) override {
            os << "{" << endl;

            _indent += _step;
            printBlockWithoutBraces(node);
            _indent -= _step;
            os << endl;

            printIndent();
            os << "}";
        }

        void visitFunctionNode(FunctionNode *node) override {
            const Signature &sign = node->signature();
            os << "function " << typeToName(sign[0].first) << " " << node->name() << "(";

            for (uint32_t i = 1; i < sign.size() - 1; ++i) {
                printSignatureElement(sign[i]);
                os << ", ";
            }
            if (sign.size() > 1) {
                printSignatureElement(sign[sign.size() - 1]);
            }
            os << ")";

            if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
                printBlockWithoutBraces(node->body());
            } else {
                os << " ";
                node->visitChildren(this);
            }
        }

        void visitReturnNode(ReturnNode *node) override {
            os << "return";
            AstNode *returnExpr = node->returnExpr();
            if (returnExpr != nullptr) {
                os << " ";
                returnExpr->visit(this);
            }
        }

        void printBlockWithoutBraces(BlockNode *node) {
            Scope *scope = node->scope();

            Scope::VarIterator varIt(scope);
            while (varIt.hasNext()) {
                AstVar *curVar = varIt.next();
                printIndent();
                os << typeToName(curVar->type()) << " "
                   << curVar->name() << ";" << endl;
            }

            Scope::FunctionIterator funIt(scope);
            while (funIt.hasNext()) {
                funIt.next()->node()->visit(this);
                os << endl;
            }

            for (uint32_t i = 0; i < node->nodes(); ++i) {
                AstNode *curNode = node->nodeAt(i);
                if (curNode->isReturnNode() && curNode->asReturnNode()->returnExpr() == nullptr)
                    continue;

                printIndent();
                curNode->visit(this);
                os << ";" << endl;
            }

        }

    };


    class SourceTranslatorImpl : public Translator {
    public:
        ~SourceTranslatorImpl() override {

        }

        Status *translate(const string &program, Code **) override {
            Parser parser;
            Status *pStatus = parser.parseProgram(program);
            if (pStatus->isError()) {
                return pStatus;
            }

            AstFunction *pFunction = parser.top();
            PrintVisitor visitor = PrintVisitor(std::cout, 2, 0);
            visitor.printBlockWithoutBraces(pFunction->node()->body());

            return pStatus;
        }
    };

    // Implement me!
    Translator *Translator::create(const string &impl) {
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
