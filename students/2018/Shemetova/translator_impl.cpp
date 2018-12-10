#include <mathvm.h>
#include "../../../vm/parser.h"
#include <iostream>
#include "interpreter/translator_helper.h"
#include "bytecode_translator.h"
#include "interpreter/typer.h"
//#include "ast_to_bytecode.h"

//http://blog.jamesdbloom.com/JavaCodeToByteCode_PartOne.html

#define INDENT "    "

namespace mathvm {

    class AstToSourceVisitor : public AstVisitor {
        std::string currentIndent = "";

        std::string getType(int type) {
            std::string type_name;
            switch (type) {
                case 1: type_name = "void";
                    break;
                case 2: type_name = "double";
                    break;
                case 3: type_name = "int";
                    break;
                case 4: type_name = "string";
                    break;
                default: type_name = ""; //maybe throw exception?
            }
            return type_name;
        }

        void visitFunctionHeader(FunctionNode* node) {
            std::cout << std::endl << currentIndent << "function "
                    << getType(node->returnType())
                    << " "
                    << node->name() << "(";
            uint32_t n = node->parametersNumber();
            for (uint32_t i = 0; i < n; i++) {
                std::cout << getType(node->parameterType(i))
                        << " " << node->parameterName(i);
                if (i < n - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << ") ";
        }

        void visitBlockNoIndent(BlockNode* node) {
            Scope* sc = node->scope();
            Scope::VarIterator it = Scope::VarIterator(sc);
            bool flag;
            if (it.hasNext()) flag = true;
            while (it.hasNext()) {
                AstVar* var = it.next();
                std::cout << currentIndent << getType(var->type())
                        << " "
                        << var->name();
                std::cout << ";" << std::endl;

            }
            flag = false;
            Scope::FunctionIterator itf = Scope::FunctionIterator(sc);
            if (itf.hasNext()) flag = true;
            while (itf.hasNext()) {
                AstFunction* func = itf.next();
                BlockNode* body = func->node()->body();
                if (body->nodes() == 2 &&
                        (body->nodeAt(0))->isNativeCallNode()) {
                    visitFunctionHeader(func->node());
                    std::cout << " ";
                    (body->nodeAt(0))->visit(this);
                    std::cout << ";";
                } else {
                    (func->node())->visit(this);
                }
            }
            if (flag) std::cout << std::endl;
            uint32_t n = node->nodes();
            for (uint32_t i = 0; i < n; i++) {
                if (!(node->nodeAt(i)->isIfNode()) && !(node->nodeAt(i)->isReturnNode())) {
                    std::cout << currentIndent;
                    (node->nodeAt(i))->visit(this);
                    std::cout << ";" << std::endl;
                } else if (node->nodeAt(i)->isIfNode()) {
                    std::cout << currentIndent;
                    (node->nodeAt(i))->visit(this);
                    std::cout << std::endl;
                } else {
                    (node->nodeAt(i))->visit(this);
                }

            }
        }


    public:

        AstToSourceVisitor() {
        }

        virtual ~AstToSourceVisitor() {
        }

        virtual void visitIntLiteralNode(IntLiteralNode* node) {
            std::cout << node->literal();
        }

        virtual void visitStringLiteralNode(StringLiteralNode* node) {
            std::string literal = node->literal();
            string::size_type pos = 0;
            while ((pos = literal.find('\n', pos)) != string::npos) {
                literal.replace(pos, 1, "\\n");
                pos += 1;
            }
            std::cout << "'" << literal << "'";
        }

        virtual void visitAstVar(AstVar* node) {
            std::cout << node->name();
        }

        virtual void visitLoadNode(LoadNode* node) {
            std::cout << (node->var())->name();
        }

        virtual void visitBinaryOpNode(BinaryOpNode* node) {
            (node->left())->visit(this);
            std::cout << " " << tokenOp(node->kind()) << " ";
            (node->right())->visit(this);
        }

        virtual void visitUnaryOpNode(UnaryOpNode* node) {
            std::cout << tokenOp(node->kind());
            (node->operand())->visit(this);

        }

        virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
            std::printf("%.1f", node->literal());
        }

        virtual void visitPrintNode(PrintNode* node) {
            std::cout << "print (";
            uint32_t n = node->operands();
            for (uint32_t i = 0; i < n; i++) {
                (node->operandAt(i))->visit(this);
                if (i < n - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << ")";
        }

        virtual void visitCallNode(CallNode* node) {
            std::cout << node->name() << "(";
            uint32_t n = node->parametersNumber();
            for (uint32_t i = 0; i < n; i++) {
                (node->parameterAt(i))->visit(this);
                if (i < n - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << ")";
        }

        virtual void visitFunctionNode(FunctionNode* node) {
            if (node->name() == "<top>") {
                visitBlockNoIndent(node->body());
            } else {
                visitFunctionHeader(node);
                std::cout << "{" << std::endl;
                (node->body())->visit(this);
                std::cout << currentIndent << "}" << std::endl;
            }
        }

        virtual void visitBlockNode(BlockNode* node) {
            std::string oldIndent = currentIndent;
            currentIndent += INDENT;
            visitBlockNoIndent(node);
            currentIndent = oldIndent;
        }

        virtual void visitStoreNode(StoreNode* node) {
            std::cout << (node->var())->name() << " "
                    << tokenOp(node->op()) << " ";
            (node->value())->visit(this);
        }

        virtual void visitForNode(ForNode* node) {
            std::cout << "for (" << node->var()->name()
                    << " in ";
            (node->inExpr())->visit(this);
            std::cout << ") {";
            (node->body())->visit(this);
            std::cout << currentIndent << "}";
        }

        virtual void visitIfNode(IfNode* node) {
            std::cout << "if (";
            (node->ifExpr())->visit(this);
            std::cout << ") {" << std::endl;
            (node->thenBlock())->visit(this);
            std::cout << currentIndent << "}";
            if (node->elseBlock() != 0) {
                std::cout << " else {" << std::endl;
                (node->elseBlock())->visit(this);
                std::cout << currentIndent << "}";
            }
        }

        virtual void visitWhileNode(WhileNode* node) {
            std::cout << "while (" << std::endl;
            (node->whileExpr())->visit(this);
            std::cout << ") {";
            (node->loopBlock())->visit(this);
            std::cout << currentIndent << "}";
        }

        virtual void visitReturnNode(ReturnNode* node) {
            if (node->returnExpr() != 0) {
                std::cout << currentIndent;
                std::cout << "return ";
                (node->returnExpr())->visit(this);
                std::cout << ";" << std::endl;
            }
        }

        virtual void visitNativeCallNode(NativeCallNode* node) {
            std::cout << "native " << "'" << node->nativeName() << "'";
        }

    };

    class SourceTranslatorImpl : public Translator {

        virtual Status* translate(const string& program, Code* *code) {
            Parser parser = Parser();
            Status* parse_status = parser.parseProgram(program);
            const AstFunction* top = parser.top();

            FunctionNode* mainFunction = top->node();
            AstToSourceVisitor* a = new AstToSourceVisitor();
            mainFunction->visit(a);
            return parse_status;
        }

    };

    // Implement me!

    Translator* Translator::create(const string& impl) {
        if (impl == "" || impl == "intepreter") {
            return new BytecodeTranslatorImpl();
        }
        if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }
        if (impl == "printer") {
            return new SourceTranslatorImpl();
        }
        //cout << impl << endl;
        return new BytecodeTranslatorImpl();
    }

} // namespace mathvm 