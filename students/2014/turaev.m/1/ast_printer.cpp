#include "visitors.h"
#include "parser.h"

using namespace mathvm;

#define BASE_VISITOR(type) AstBaseVisitor::visit##type(node);

class PrinterVisitor : public AstBaseVisitor {
public:
    PrinterVisitor(std::ostream &output): output(output), tabDepth(0) { }

    virtual void visitBinaryOpNode(BinaryOpNode *node) {
        output << "(";
        node->left()->visit(this);
        output << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
        output << ")";
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node) {
        output << tokenOp(node->kind()) << " ";
        BASE_VISITOR(UnaryOpNode);
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node) {
        output << "'" << escape(node->literal()) << "'";
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        output << node->literal();
    }

    virtual void visitIntLiteralNode(IntLiteralNode *node) {
        output << node->literal();
    }

    virtual void visitLoadNode(LoadNode *node) {
        output << node->var()->name();
    }

    virtual void visitStoreNode(StoreNode *node) {
        output << node->var()->name() << " " << tokenOp(node->op()) << " ";
        BASE_VISITOR(StoreNode);
    }

    virtual void visitForNode(ForNode *node) {
        output << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        output << ") ";
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode *node) {
        output << "while (";
        node->whileExpr()->visit(this);
        output << ") ";
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode *node) {
        output << "if (";
        node->ifExpr()->visit(this);
        output << ") ";
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            output << "else ";
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitBlockNode(BlockNode *node) {
        output << "{" << std::endl;
        tab();

        printVariables(node->scope());
        printFunctions(node->scope());
        printBlock(node);

        unTab();
        tabify();
        output << "}" << std::endl;
    }

    virtual void visitFunctionNode(FunctionNode *node) {
        output << "function "
               << typeToName(node->returnType())
               << " "
               << node->name() << "(";

        uint32_t paramsNumber = node->parametersNumber();
        for (uint32_t i = 0; i < paramsNumber; i++) {
            output << typeToName(node->parameterType(i))
                   << " "
                   << node->parameterName(i)
                   << (i < paramsNumber - 1 ? ", " : "");
        }
        output << ") ";

        if (node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->visit(this);
        } else {
            BASE_VISITOR(FunctionNode);
        }
    }

    virtual void visitReturnNode(ReturnNode *node) {
        output << "return";
        if (node->returnExpr() != 0) {
            output << " ";
            BASE_VISITOR(ReturnNode);
        }
    }

    virtual void visitCallNode(CallNode *node) {
        output << node->name() << "(";
        uint32_t paramsNumber = node->parametersNumber();
        for (uint32_t i = 0; i < paramsNumber; i++) {
            node->parameterAt(i)->visit(this);
            output << (i < paramsNumber - 1 ? ", " : "");
        }
        output << ")";
    }

    virtual void visitNativeCallNode(NativeCallNode *node) {
        output << "native '" << node->nativeName() << "';" << std::endl;
    }

    virtual void visitPrintNode(PrintNode *node) {
        output << "print(";
        uint32_t paramsNumber = node->operands();
        for (uint32_t i = 0; i < paramsNumber; i++) {
            node->operandAt(i)->visit(this);
            output << (i < paramsNumber - 1 ? ", " : "");
        }
        output << ")";
    }

    void printVariables(Scope *scope) {
        Scope::VarIterator it(scope);
        while (it.hasNext()) {
            AstVar *var = it.next();
            tabify();
            output << typeToName(var->type()) << " " << var->name() << ";" << endl;
        }
    }

    void printFunctions(Scope *scope) {
        Scope::FunctionIterator it(scope);
        while (it.hasNext()) {
            tabify();
            it.next()->node()->visit(this);
        }
    }

    void printBlock(BlockNode *node) {
        for (uint32_t i = 0; i < node->nodes(); i++) {
            AstNode *currentNode = node->nodeAt(i);
            tabify();
            currentNode->visit(this);
            if (currentNode->isLoadNode() || currentNode->isStoreNode() ||
                    currentNode->isReturnNode() || currentNode->isCallNode() ||
                    currentNode->isNativeCallNode() || currentNode->isPrintNode()) {
                output << ";" << std::endl;
            }
        }
    }
private:
    std::ostream &output;
    size_t tabDepth;

    void tab() {
        tabDepth++;
    }

    void unTab() {
        tabDepth--;
    }

    void tabify() {
        for (int i = 0; i < tabDepth; ++i) {
            output << '\t';
        }
    }

    string escape(const string &str) {
        string result;
        for (size_t i = 0; i < str.size(); i++) {
            switch (str[i]) {
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                result += str[i];
            }
        }
        return result;
    }
};

class AstPrinter : public Translator {
public:
    virtual Status *translate(const string &program, Code **code)  {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status && status->isError()) {
            return status;
        }

        PrinterVisitor printer(std::cout);
        printer.printVariables(parser.top()->node()->body()->scope());
        printer.printFunctions(parser.top()->node()->body()->scope());
        printer.printBlock(parser.top()->node()->body());
        
        return Status::Ok();
    }
};

Translator *Translator::create(const string &impl) {
    if (impl == "printer") {
        return new AstPrinter();
    } else {
        return NULL;
    }
}