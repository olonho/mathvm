#include "include/translator_impl.h"
#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include <iostream>

namespace mathvm {
    Status* AstPrintTranslator::translate(const string& program, Code** code) {
        Parser parser;
        Status* status = parser.parseProgram(program);
        if (status->isOk()) {
            AstPrintVisitor* printer = new AstPrintVisitor();
            parser.top()->node()->visitChildren(printer);
            delete printer;
        }
        return status;
    }

    void AstPrintVisitor::print(const string& str) {
        std::cout << str;
    }

    void AstPrintVisitor::visitBinaryOpNode(BinaryOpNode* node) {
        print("(");
        node->left()->visit(this);
        print(" ");
        print(tokenOp(node->kind()));
        print(" ");
        node->right()->visit(this);
        print(")");
    }

    void AstPrintVisitor::visitUnaryOpNode(UnaryOpNode* node) {
        print(tokenOp(node->kind()));
        node->operand()->visit(this);
    }

    string escapeSpecialSymbols(const string& str) {
        std::string resString = "";
        for (char c : str) {
            switch (c) {
                case '\'': resString += "\\'";
                    break;
                case '\"': resString += "\\\"";
                    break;
                case '\?': resString += "\\?";
                    break;
                case '\\': resString += "\\\\";
                    break;
                case '\a': resString += "\\a";
                    break;
                case '\b': resString += "\\b";
                    break;
                case '\f': resString += "\\f";
                    break;
                case '\n': resString += "\\n";
                    break;
                case '\r': resString += "\\r";
                    break;
                case '\t': resString += "\\t";
                    break;
                case '\v': resString += "\\v";
                    break;
                default: resString += c;
            }
        }
        return resString;
    }

    void AstPrintVisitor::visitStringLiteralNode(StringLiteralNode* node) {
        print("'");
        print(escapeSpecialSymbols(node->literal()));
        print("'");
    }

    void AstPrintVisitor::visitIntLiteralNode(IntLiteralNode* node) {
        print(std::to_string(node->literal()));
    }

    void AstPrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
        print(std::to_string(node->literal()));
    }

    void AstPrintVisitor::visitLoadNode(LoadNode* node) {
        print(node->var()->name());
    }

    void AstPrintVisitor::visitStoreNode(StoreNode* node) {
        print(node->var()->name());
        print(" ");
        print(tokenOp(node->op()));
        print(" ");
        node->value()->visit(this);
    }

    void AstPrintVisitor::printOffset() {
        for (int i = 0; i < currentOffsetNumber; i++) {
            cout << offset;
        }
    }

    bool isStatement(AstNode* node) {
        return dynamic_cast<StoreNode*>(node)      ||
               dynamic_cast<NativeCallNode*>(node) ||
               dynamic_cast<NativeCallNode*>(node) ||
               dynamic_cast<NativeCallNode*>(node) ||
               dynamic_cast<CallNode*>(node)       ||
               dynamic_cast<ReturnNode*>(node)     ||
               dynamic_cast<PrintNode*>(node);
    }

    void AstPrintVisitor::visitBlockNode(BlockNode* node) {
        Scope* scope = node->scope();
        if (scope->parent()->parent()) {
            currentOffsetNumber++;
        }
        Scope::VarIterator varIter = Scope::VarIterator(scope);
        while(varIter.hasNext()) {
            printOffset();
            AstVar* currVar = varIter.next();
            print(typeToName(currVar->type()));
            print(" ");
            print(currVar->name());
            print(";\n");
        }

        Scope::FunctionIterator funIter = Scope::FunctionIterator(scope);
        while(funIter.hasNext()) {
            printOffset();
            AstFunction* currFun = funIter.next();
            currFun->node()->visit(this);
            print("\n");
        }

        for (uint32_t nodeIdx = 0; nodeIdx < node->nodes(); nodeIdx++) {
            printOffset();
            node->nodeAt(nodeIdx)->visit(this);
            if (isStatement(node->nodeAt(nodeIdx))) {
                print(";");
            }
            if (nodeIdx != node->nodes() - 1) {
                print("\n");
            }
        }
        currentOffsetNumber--;
    }

    void AstPrintVisitor::visitNativeCallNode(NativeCallNode* node) {
        print("native ");
        print(node->nativeName());
        print(";\n");
    }

    void AstPrintVisitor::visitForNode(ForNode* node) {
        print("for (");
        print(node->var()->name());
        print(" in ");
        node->inExpr()->visit(this);
        print(") { \n");
        node->body()->visit(this);
        print("\n");
        printOffset();
        print("}");
    }

    void AstPrintVisitor::visitWhileNode(WhileNode* node) {
        print("while (");
        node->whileExpr()->visit(this);
        print(") {\n");
        node->loopBlock()->visit(this);
        print("\n");
        printOffset();
        print("}");
    }

    void AstPrintVisitor::visitIfNode(IfNode* node) {
        print("if (");
        node->ifExpr()->visit(this);
        print(") {\n");
        node->thenBlock()->visit(this);
        print("\n");
        printOffset();
        print("}");
        if (node->elseBlock()) {
            print(" else {\n");
            node->elseBlock()->visit(this);
            print("\n");
            printOffset();
            print("}");
        }
    }

    void AstPrintVisitor::visitReturnNode(ReturnNode* node) {
        print("return ");
        node->returnExpr()->visit(this);
    }

    void AstPrintVisitor::visitFunctionNode(FunctionNode* node) {
        printOffset();
        print("function ");
        print(typeToName(node->returnType()));
        print(" ");
        print(node->name());
		print(" (");
        for (uint32_t parIdx = 0; parIdx < node->parametersNumber(); parIdx++) {
            print(typeToName(node->parameterType(parIdx)));
            print(" ");
            print(node->parameterName(parIdx));
            if (parIdx != node->parametersNumber() - 1) {
                print(", ");
            }
        }
        print(") {\n");
        node->body()->visit(this);
        print("\n");
        printOffset();
        print("}");
    }

    void AstPrintVisitor::visitCallNode(CallNode* node) {
        print(node->name());
        print("(");
        for (uint32_t parIdx = 0; parIdx < node->parametersNumber(); parIdx++) {
            node->parameterAt(parIdx)->visit(this);
            if (parIdx != node->parametersNumber() - 1) {
                print(", ");
            }
        }
        print(")");
    }

    void AstPrintVisitor::visitPrintNode(PrintNode* node) {
        print("print(");
        for (uint32_t opIdx = 0; opIdx < node->operands(); opIdx++) {
            node->operandAt(opIdx)->visit(this);
            if (opIdx != node->operands() - 1) {
                print(", ");
            }
        }
        print(")");
    }

    Translator* Translator::create(const string& impl) {
        if (impl == "" || impl == "intepreter") {
            //return new BytecodeTranslatorImpl();
            return 0;
        } else if (impl == "printer") {
            return new AstPrintTranslator();
        } else if (impl == "jit") {
            //return new MachCodeTranslatorImpl();
        }
        assert(false);
        return 0;
    }
}

