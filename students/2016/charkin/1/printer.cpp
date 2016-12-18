#include "printer.h"
#include "ast.h"

void mathvm::Printer::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    _out_stream << ' ';
    _out_stream << tokenOp(node->kind());
    _out_stream << ' ';
    node->right()->visit(this);
}

std::string mathvm::Printer::indent() {
    return std::string(_depth * 4, ' ');
}

std::string mathvm::Printer::escape(char c) {
    switch(c) {
    case '\n':
        return "\\n";
    case '\t':
        return "\\t";
    case '\\':
        return "\\\\";
    case '\r':
        return "\\r";
    default:
        return c + "";
    }
}

void mathvm::Printer::visitBlockNode(BlockNode *node) {
    if (_depth >= 0) {
        _out_stream << " {" << std::endl;
    }
    _depth++;

    if (node->scope()->variablesCount() > 0) {
        Scope::VarIterator var_iterator = Scope::VarIterator(node->scope());
        AstVar * var;
        while (var_iterator.hasNext()) {
            var = var_iterator.next();
            _out_stream << indent() << typeToName(var->type()) << ' ' << var->name() << ';' << std::endl;
        }
    }

    if (node->scope()->functionsCount() > 0) {
        Scope::FunctionIterator fun_iterator = Scope::FunctionIterator(node->scope());
        AstFunction * fun;
        while (fun_iterator.hasNext()) {
            fun = fun_iterator.next();
            _out_stream << indent();
            fun->node()->visit(this);
        }
    }

    for (uint32_t i = 0; i < node->nodes(); i++) {
        _out_stream << indent();
        AstNode * cur_node =  node->nodeAt(i);
        cur_node->visit(this);
        if (!(cur_node->isForNode()
              || cur_node->isIfNode()
              || cur_node->isWhileNode()
              || cur_node->isBlockNode()
              || cur_node->isFunctionNode())) {
            _out_stream << ';' << std::endl;
        }
    }

    _depth--;
    if (_depth > 0) {
        _out_stream << indent() << '}' << std::endl;;
    }
}


void mathvm::Printer::visitCallNode(CallNode *node) {
    _out_stream << node->name();
    _out_stream << '(';
    if (node->parametersNumber() > 0) {
        node->parameterAt(0)->visit(this);
        for (uint32_t i = 1; i < node->parametersNumber(); i++) {
            _out_stream << ", ";
            node->parameterAt(i)->visit(this);
        }
    }
    _out_stream << ')';
}


void mathvm::Printer::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    _out_stream << node->literal();
}


void mathvm::Printer::visitForNode(ForNode *node) {
    _out_stream << "for (";
    node->var()->name();
    _out_stream << " in ";
    node->inExpr()->visit(this);
    _out_stream << ')';
    node->body()->visit(this);
}

void mathvm::Printer::visitFunctionNode(FunctionNode *node) {
    _out_stream << "function " << typeToName(node->returnType()) << ' ' << node->name() << '(';
    if (node->parametersNumber() > 0) {
        _out_stream << typeToName(node->parameterType(0)) << ' ' << node->parameterName(0);
        for (uint32_t i = 1; i < node->parametersNumber(); i++) {
            _out_stream << ", " << typeToName(node->parameterType(i)) << ' ' << node->parameterName(i);
        }
    }
    _out_stream << ')';

    node->body()->visit(this);
}


void mathvm::Printer::visitIfNode(IfNode *node) {
    _out_stream << "if (";
    node->ifExpr()->visit(this);
    _out_stream << ')';

    node->thenBlock()->visit(this);

    if (node->elseBlock()) {
        _out_stream << indent() << "else ";
        node->elseBlock()->visit(this);
    }
}


void mathvm::Printer::visitIntLiteralNode(IntLiteralNode *node) {
    _out_stream << node->literal();
}


void mathvm::Printer::visitLoadNode(LoadNode *node) {
    _out_stream << node->var()->name();
}


void mathvm::Printer::visitNativeCallNode(NativeCallNode *node) {
    _out_stream << "native '" << node->nativeName() << '\'';
}

void mathvm::Printer::visitPrintNode(PrintNode *node) {
    _out_stream << "ptint(";
    if (node->operands() > 0) {
        node->operandAt(0)->visit(this);
        for (uint32_t i = 1; i < node->operands(); i++) {
            _out_stream << ", ";
            node->operandAt(i)->visit(this);
        }
    }
    _out_stream << ')';
}

void mathvm::Printer::visitReturnNode(ReturnNode *node) {
    _out_stream << "return";
    if (node->returnExpr()) {
        _out_stream << ' ';
        node->returnExpr()->visit(this);
    }
}


void mathvm::Printer::visitStoreNode(StoreNode *node) {
    _out_stream << node->var()->name() << ' ' << tokenOp(node->op()) << ' ';
    node->value()->visit(this);
}


void mathvm::Printer::visitStringLiteralNode(StringLiteralNode *node) {
    _out_stream << '\'';
    for (uint32_t i = 0; i < node->literal().length(); i++){
        _out_stream << escape(node->literal()[i]);
    }
    _out_stream << '\'';
}


void mathvm::Printer::visitUnaryOpNode(UnaryOpNode *node) {
    _out_stream << tokenOp(node->kind());
    node->operand()->visit(this);
}


void mathvm::Printer::visitWhileNode(WhileNode *node) {
    _out_stream << "while (";
    node->whileExpr()->visit(this);
    _out_stream << ')';

    node->loopBlock();
}
