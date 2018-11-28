#include "ast_printer.h"
#include "../../../../vm/parser.h"
#include "mathvm.h"

namespace mathvm {

    const char* type_as_string(VarType const & type);
    void print_raw(char const ch);

    uint32_t indent = 0;
    void printIndent();

    Status* source_translator_impl::translate(const string& program, Code* *code) {
        Parser parser;
        Status* status = parser.parseProgram(program);
        if (status->isOk()) {
            ast_printer_visitor visitor;
            visitor.visit_topmost_function(parser.top());
        }
        return status;
    }

    void ast_printer_visitor::visitBinaryOpNode(BinaryOpNode *node) {
        if (node->left()->isBinaryOpNode())
            std::cout << "(";
        node->left()->visit(this);
        if (node->left()->isBinaryOpNode())
            std::cout << ")";
        if (node->kind() != tRANGE)
            std::cout << " ";
        std::cout << tokenOp(node->kind());
        if (node->kind() != tRANGE)
            std::cout << " ";
        if (node->right()->isBinaryOpNode())
            std::cout << "(";
        node->right()->visit(this);
        if (node->right()->isBinaryOpNode())
            std::cout << ")";
    }

    void ast_printer_visitor::visitUnaryOpNode(UnaryOpNode* node) {
        std::cout << tokenOp(node->kind());
        if (!(node->operand()->isIntLiteralNode() || node->operand()->isDoubleLiteralNode()))
            std::cout << "(";
        node->visitChildren(this);
        if (!(node->operand()->isIntLiteralNode() || node->operand()->isDoubleLiteralNode()))
            std::cout << ")";
    }

    void ast_printer_visitor::visitStringLiteralNode(StringLiteralNode* node) {
        string const s = node->literal();
        std::cout << "\'";
        for (char const ch : s) {
            print_raw(ch);
        }
        std::cout << "\'";
    }

    void ast_printer_visitor::visitIntLiteralNode(IntLiteralNode* node) {
        std::cout << node->literal();
    }

    void ast_printer_visitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        std::cout << node->literal();
    }

    void ast_printer_visitor::visitLoadNode(LoadNode *node) {
        std::cout << node->var()->name();
    }

    void ast_printer_visitor::visitStoreNode(StoreNode *node) {
        std::cout << node->var()->name() << " " << tokenOp(node->op()) << " ";
        node->visitChildren(this);
    }

    void ast_printer_visitor::visitBlockNode(BlockNode *node) {
        if (node->scope()->parent()->parent() != 0 && !(node->nodes() > 0 && node->nodeAt(0)->isNativeCallNode())) {
            std::cout << " {\n";
            ++indent;
        }
        Scope* scope = node->scope();
        Scope::VarIterator var_iterator(scope);
        while (var_iterator.hasNext()) {
            AstVar* var = var_iterator.next();
            printIndent();
            std::cout << type_as_string(var->type()) << " " << var->name() << ";\n";
        }
        Scope::FunctionIterator function_iterator(scope);
        while (function_iterator.hasNext()) {
            AstFunction* function = function_iterator.next();
            printIndent();
            function->node()->visit(this);
        }
        for (uint32_t i = 0; i < node->nodes(); i++) {
            AstNode* _node = node->nodeAt(i);
            if (_node->isReturnNode() && _node->asReturnNode()->returnExpr() == 0)
                continue;
            printIndent();
            _node->visit(this);
            if (!(_node->isBlockNode() || _node->isIfNode() || _node->isWhileNode() || _node->isForNode() || _node->isFunctionNode()))
                std::cout << ";\n";
        }
        if (node->scope()->parent()->parent() != 0 && !(node->nodes() > 0 && node->nodeAt(0)->isNativeCallNode())) {
            --indent;
            printIndent();
            std::cout << "}\n";
        }
    }

    void ast_printer_visitor::visitNativeCallNode(NativeCallNode *node) {
        std::cout << " native \'" << node->nativeName() << "\'";
    }

    void ast_printer_visitor::visitForNode(ForNode *node) {
        std::cout << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        std::cout << ")";
        node->body()->visit(this);
    }

    void ast_printer_visitor::visitWhileNode(WhileNode *node) {
        std::cout << "while (";
        node->whileExpr()->visit(this);
        std::cout << ")";
        node->loopBlock()->visit(this);
    }

    void ast_printer_visitor::visitIfNode(IfNode *node) {
        std::cout << "if (";
        node->ifExpr()->visit(this);
        std::cout << ")";
        node->thenBlock()->visit(this);
        if (node->elseBlock() != 0) {
            printIndent();
            std::cout << "else";
            node->elseBlock()->visit(this);
        }
    }

    void ast_printer_visitor::visitReturnNode(ReturnNode *node) {
        std::cout << "return ";
        node->returnExpr()->visit(this);
    }

    void ast_printer_visitor::visitFunctionNode(FunctionNode *node) {
        if (node->name() == "<top>") {
            node->body()->visit(this);
            return;
        }
        std::cout << "function " << type_as_string(node->returnType()) << " " << node->name() << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if (i > 0)
                std::cout << ", ";
            std::cout << type_as_string(node->parameterType(i)) << " " << node->parameterName(i);
        }
        std::cout << ")";
        node->body()->visit(this);
    }

    void ast_printer_visitor::visitCallNode(CallNode *node) {
        std::cout << node->name() << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if (i > 0)
                std::cout << ", ";
            node->parameterAt(i)->visit(this);
        }
        std::cout << ")";
    }

    void ast_printer_visitor::visitPrintNode(PrintNode *node) {
        std::cout << "print(";
        for (uint32_t i = 0; i < node->operands(); ++i) {
            if (i > 0)
                std::cout << ", ";
            node->operandAt(i)->visit(this);
        }
        std::cout << ")";
    }

    void ast_printer_visitor::visit_topmost_function(AstFunction *top_function) {
        Scope* scope = top_function->scope();
        Scope::VarIterator var_iterator(scope);
        while (var_iterator.hasNext()) {
            AstVar* var = var_iterator.next();
            std::cout << type_as_string(var->type()) << " " << var->name() << ";\n";
        }
        Scope::FunctionIterator function_iterator(scope);
        while (function_iterator.hasNext()) {
            AstFunction* function = function_iterator.next();
            function->node()->visit(this);
        }
    }

    const char* type_as_string(VarType const & type) {
        if (type == VT_VOID)
            return "void";
        else if (type == VT_DOUBLE)
            return "double";
        else if (type == VT_INT)
            return "int";
        else if (type == VT_STRING)
            return "string";
        else return "<unknown>";
    }

    void print_raw(char const ch) {
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

    void printIndent() {
        for (uint32_t i = 0; i < indent; ++i)
            std::cout << "    ";
    }
} // namespace mathvm
