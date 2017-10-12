#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "ast_translator_impl.h"

#include <iostream>
#include <stdarg.h>

namespace mathvm {

Translator* Translator::create(const string& impl) {
    if (impl == "printer" || impl == "")
        return new AstTranslatorImpl();

    std::cout << "Only AST translator supported for now, you asked for '" << impl << "'\n";
    assert(false);

    return nullptr;
}


Status* AstTranslatorImpl::translate(const string& program, Code* *code)
{
    Status *res;
    Parser parser;
    res = parser.parseProgram(program);

    if (res->isError())
        return res;

    AstFunction *top = parser.top();

    AstPrinter printer(std::cout);
    top->node()->visit(&printer);

    return res;
}


void AstPrinter::printIndent()
{
    if (this->_indent >= 0)
        _out << std::string(this->_indent * 4, ' ');
}

bool AstPrinter::isNative(FunctionNode *node)
{
    return (node->body()->nodes() > 0 && dynamic_cast<NativeCallNode *>(node->body()->nodeAt(0)));
}

bool AstPrinter::containsBlock(AstNode *node)
{
    return (dynamic_cast<FunctionNode *>(node) && isNative(dynamic_cast<FunctionNode *>(node)))
        || dynamic_cast<IfNode *>(node)
        || dynamic_cast<ForNode *>(node)
        || dynamic_cast<BlockNode *>(node)
        || dynamic_cast<WhileNode *>(node);
}

void AstPrinter::visitBinaryOpNode(BinaryOpNode *node)
{
    _out << "(";
    node->left()->visit(this);
    _out << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
    _out << ")";
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode *node)
{
    _out << tokenOp(node->kind());
    node->visitChildren(this);
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode *node)
{
    _out << "\'";
    for (int i = 0; i < (int)node->literal().size(); i++) {
        switch (node->literal()[i]) {
            case '\n':
                _out << "\\n";
                break;
            case '\t':
                _out << "\\t";
                break;
            case '\r':
                _out << "\\r";
                break;
            case '\'':
                _out << "\\'";
                break;
            case '\"':
                _out << "\\\"";
                break;
            case '\?':
                _out << "\\?";
                break;
            case '\\':
                _out << "\\\\";
                break;
            default:
                _out << node->literal()[i];
        }
    }
    _out << "\'";
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode *node)
{
    _out << node->literal();
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
    _out << node->literal();
}

void AstPrinter::visitLoadNode(LoadNode *node)
{
    _out << node->var()->name();
}

void AstPrinter::visitStoreNode(StoreNode *node)
{
    _out << node->var()->name() << " " << tokenOp(node->op()) << " ";
    node->visitChildren(this);
}

void AstPrinter::visitBlockNode(BlockNode *node)
{
    // don't print brackets for main
    if (this->_indent != -1)
        _out << "{\n";

    this->_indent++;

    // declare variables
    for (Scope::VarIterator var_it(node->scope()); var_it.hasNext();) {
        AstVar *var = var_it.next();
        printIndent();
        _out << typeToName(var->type()) << " " << var->name() << ";\n";
    }

    // I just personally like to do so
    if (node->scope()->variablesCount() > 0)
        _out << "\n";

    // print functions
    for (Scope::FunctionIterator fun_it(node->scope()); fun_it.hasNext();) {
        AstFunction *fun = fun_it.next();
        printIndent();
        fun->node()->visit(this);
        if (isNative(fun->node())) {
            _out << ";";
        } else {
            if (fun_it.hasNext())
                _out << "\n";
        }
        _out << "\n";
    }

    _out << "\n";

    bool emptyLine = true;

    for (int i = 0; i < (int)node->nodes(); i++) {
        AstNode *child = node->nodeAt(i);

        // I just personally like to do so
        if (containsBlock(child) && !emptyLine)
            _out << "\n";

        printIndent();
        child->visit(this);

        // I just personally like to do so
        if (!containsBlock(child)) {
            _out << ";";
            emptyLine = false;
        } else {
            _out << "\n";
            emptyLine = true;
        }

        _out << "\n";
    }

    this->_indent--;

    // don't print brackets for main
    if (this->_indent != -1) {
        printIndent();
        _out << "}";
    }
}

void AstPrinter::visitNativeCallNode(NativeCallNode *node)
{
    _out << "native '" << node->nativeName() << "'";
}

void AstPrinter::visitForNode(ForNode *node)
{
    _out << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    _out << ") ";

    node->body()->visit(this);
}

void AstPrinter::visitWhileNode(WhileNode *node)
{
    _out << "while (";
    node->whileExpr()->visit(this);
    _out << ")";

    node->loopBlock()->visit(this);
}

void AstPrinter::visitIfNode(IfNode *node)
{
    _out << "if (";
    node->ifExpr()->visit(this);
    _out << ") ";

    node->thenBlock()->visit(this);

    if (node->elseBlock() != NULL) {
        _out << " else ";
        node->elseBlock()->visit(this);
    }
}

void AstPrinter::visitReturnNode(ReturnNode *node)
{
    _out << "return ";
    node->visitChildren(this);
}

void AstPrinter::visitFunctionNode(FunctionNode *node)
{
    if (node->name() != AstFunction::top_name) {
        _out << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
        for (int i = 0; i < (int)node->parametersNumber(); i++) {
            _out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
            if (i != (int)node->parametersNumber() - 1)
                _out << ", ";
        }
        _out << ")";
    }

    if (isNative(node)) {
        _out << " ";
        node->body()->nodeAt(0)->visit(this);
    } else {
        _out << "\n";
        printIndent();
        node->visitChildren(this);
    }
}

void AstPrinter::visitCallNode(CallNode *node)
{
    _out << node->name() << "(";

    for (int i = 0; i < (int)node->parametersNumber(); i++) {
        AstNode *child = node->parameterAt(i);
        child->visit(this);
        if (i != (int)node->parametersNumber() - 1)
            _out << ", ";
    }

    _out << ")";
}

void AstPrinter::visitPrintNode(PrintNode *node)
{
    _out << "print(";

    for (int i = 0; i < (int)node->operands(); i++) {
        AstNode *child = node->operandAt(i);
        child->visit(this);
        if (i != (int)node->operands() - 1)
            _out << ", ";
    }

    _out << ")";
}

}
