#include <iostream>
#include <iomanip>
#include <string>
#include <regex>

#include "../vm/parser.h"
#include "translator_impl.h"


namespace mathvm {

    Translator* Translator::create(const string& impl) {
        if (impl == "" || impl == "intepreter") {
            // return new BytecodeTranslatorImpl();
            return 0;
        }

        if (impl == "jit") {
            // return new MachCodeTranslatorImpl();
            return 0;
        }

        if (impl == "printer") {
            return new AstPrinterTranslator();
        }

        assert(false);
        return 0;
    }

    Status* AstPrinterTranslator::translate(const std::string& program, Code* *code) {
        Parser parser;
        Status* parseStatus = parser.parseProgram(program);

        if (parseStatus->isError()) {
            return parseStatus;
        } else {
            delete parseStatus;

            AstPrinter printer(std::cout);
            parser.top()->node()->visitChildren(&printer);
        }

        return Status::Ok();
    }

    string typeToString(VarType type) {
        switch (type) {
            case VT_VOID   : return "void";
            case VT_DOUBLE : return "double";
            case VT_INT    : return "int";
            case VT_STRING : return "string";
            default        : return "INVALID";
        }
    }

    void AstPrinter::printScope(Scope* scope) {
        Scope::VarIterator varIt(scope);

        while (varIt.hasNext()) {
            AstVar* var = varIt.next();
            out << std::setw(indent) << ' '
                << typeToString(var->type())
                << ' ' << var->name()
                << ';' << std::endl;
        }

        Scope::FunctionIterator funcIt(scope);

        while (funcIt.hasNext()) {
            AstFunction* function = funcIt.next();
            function->node()->visit(this);
        }
    }

    void AstPrinter::increaseIndent() {
        indent += tab;
    }

    void AstPrinter::decreaseIndent() {
        indent -= tab;
    }

    void AstPrinter::visitBinaryOpNode(BinaryOpNode* node) {
        out << "((";
        node->left()->visit(this);
        out << ')';

        out << ' ' << tokenOp(node->kind()) << ' ';

        out << '(';
        node->right()->visit(this);
        out << "))";
    }

    void AstPrinter::visitUnaryOpNode(UnaryOpNode* node) {
        out << tokenOp(node->kind()) << '(';
        node->operand()->visit(this);
        out << ')';
    }

    void AstPrinter::visitStringLiteralNode(StringLiteralNode* node) {
        out << '"';

        std::string str = node->literal();
        const char* escape_symbols = "\a\b\f\n\r\t\v\\'\"";

        size_t prev_pos = 0;
        size_t pos = str.find_first_of(escape_symbols);
        while (pos != std::string::npos) {
            out << str.substr(prev_pos, pos - prev_pos);

            switch (str[pos]) {
                case '\a': out << "\\a";  break;
                case '\b': out << "\\b";  break;
                case '\f': out << "\\f";  break;
                case '\n': out << "\\n";  break;
                case '\r': out << "\\r";  break;
                case '\t': out << "\\t";  break;
                case '\v': out << "\\v";  break;
                case '\\': out << "\\\\"; break;
                case '"' : out << "\\\""; break;
                case '\'': out << "\\'";  break;
            }

            prev_pos = pos;
            pos = str.find_first_of(escape_symbols, pos + 1);
        }

        out << '"';
    }

    void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode* node) {
        out << node->literal();
    }

    void AstPrinter::visitIntLiteralNode(IntLiteralNode* node) {
        out << node->literal();
    }

    void AstPrinter::visitLoadNode(LoadNode* node) {
        out << node->var()->name();
    }

    void AstPrinter::visitStoreNode(StoreNode* node) {
        out << node->var()->name() << ' ' << tokenOp(node->op()) << ' ';
        node->value()->visit(this);
    }

    void AstPrinter::visitForNode(ForNode* node) {
        out << "for ("
            << node->var()->name() << " in ";

        node->inExpr()->visit(this);

        out << ") {" << endl;

        increaseIndent();
        node->body()->visit(this);
        decreaseIndent();

        out << setw(indent) << ' ' << '}' << endl;
    }

    void AstPrinter::visitWhileNode(WhileNode* node) {
        out << "while (";
        node->whileExpr()->visit(this);
        out << ") {" << endl;

        increaseIndent();
        node->loopBlock()->visit(this);
        decreaseIndent();

        out << setw(indent) << ' ' << '}' << endl;
    }

    void AstPrinter::visitIfNode(IfNode* node) {
        out << "if (";
        node->ifExpr()->visit(this);
        out << ") {" << endl;

        increaseIndent();
        node->thenBlock()->visit(this);
        decreaseIndent();

        if (node->elseBlock() == nullptr) {
            out << setw(indent) << ' ' << '}' << endl;
            return;
        }

        out << setw(indent) << ' ' << "} else {" << endl;

        increaseIndent();
        node->elseBlock()->visit(this);
        decreaseIndent();

        out << setw(indent) << ' ' << '}' << endl;
    }

    bool isStatement(AstNode* node) {
        IfNode* ifNode = dynamic_cast<IfNode*>(node);
        ForNode* forNode = dynamic_cast<ForNode*>(node);
        WhileNode* whileNode = dynamic_cast<WhileNode*>(node);
        ReturnNode* returnNode = dynamic_cast<ReturnNode*>(node);

        return (ifNode == nullptr) &&
                (forNode == nullptr) &&
                (whileNode == nullptr) &&
                (returnNode == nullptr);
    }

    void AstPrinter::visitBlockNode(BlockNode* node) {
        AstPrinter::printScope(node->scope());

        for (uint32_t i = 0; i < node->nodes(); ++i) {
            out << setw(indent) << ' ';
            node->nodeAt(i)->visit(this);

            if (isStatement(node->nodeAt(i))) {
                out << ';';
                out << endl;
            }
        }
    }

    void AstPrinter::visitFunctionNode(FunctionNode* node) {
        out << std::setw(indent) << ' '
            << "function " << typeToString(node->returnType())
            << ' ' << node->name() << '(';

        if (node->parametersNumber() > 0) {
            out << typeToString(node->parameterType(0)) << ' '
                << node->parameterName(0);

            for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
                out << ", "
                    << typeToString(node->parameterType(i)) << ' '
                    << node->parameterName(i);
            }
        }

        if (dynamic_cast<NativeCallNode*>(node->body()->nodeAt(0))) {
            out << ")";
            node->body()->nodeAt(0)->visit(this);
            return;
        }

        out << ") {" << endl;

        if (node->body() != nullptr) {
            increaseIndent();
            node->body()->visit(this);
            decreaseIndent();
        }

        out << setw(indent) << ' ' << '}' << endl;
    }

    void AstPrinter::visitReturnNode(ReturnNode* node) {
        if (node->returnExpr() == nullptr) {
            out << endl;
            return;
        }

        out << "return ";
        node->returnExpr()->visit(this);
        out << ';' << endl;
    }

    void AstPrinter::visitCallNode(CallNode* node) {
        out << node->name() << '(';

        if (node->parametersNumber() > 0) {
            node->parameterAt(0)->visit(this);

            for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
                out << ", ";
                node->parameterAt(i)->visit(this);
            }
        }

        out << ")";
    }

    void AstPrinter::visitNativeCallNode(NativeCallNode* node) {
        out << " native '"<< node->nativeName() << "';" << endl;
    }

    void AstPrinter::visitPrintNode(PrintNode* node) {
        out << "print(";

        if (node->operands() > 0) {
            node->operandAt(0)->visit(this);

            for (uint32_t i = 1; i < node->operands(); ++i) {
                out << ", ";
                node->operandAt(i)->visit(this);
            }
        }

        out << ")";
    }
}
