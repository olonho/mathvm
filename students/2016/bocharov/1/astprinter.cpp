#include "astprinter.h"
#include "ast.h"

namespace mathvm {

AstPrinter::AstPrinter(std::ostream & out)
    : m_out(out)
    , m_depth(0)
{}


// literals
void AstPrinter::visitIntLiteralNode(IntLiteralNode* node) {
    m_out << node->literal();
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    m_out << node->literal();
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode* node) {
    m_out << '\'';
    printEscaped(node->literal());
    m_out << '\'';
}

// load/store
void AstPrinter::visitLoadNode(LoadNode* node) {
    m_out << node->var()->name();
}

void AstPrinter::visitStoreNode(StoreNode* node) {
    m_out << node->var()->name();
    m_out << " " << tokenOp(node->op()) << " ";
    node->value()->visit(this);
}

// operations
void AstPrinter::visitUnaryOpNode(UnaryOpNode* node) {
    m_out << tokenOp(node->kind());
    node->visitChildren(this);
}

void AstPrinter::visitBinaryOpNode(BinaryOpNode* node) {
    m_out << tokenOp(tLPAREN);

    node->left()->visit(this);
    m_out << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);

    m_out << tokenOp(tRPAREN);
}

// control flow
void AstPrinter::visitIfNode(IfNode* node) {
    m_out << "if " << tokenOp(tLPAREN);
    node->ifExpr()->visit(this);
    m_out << tokenOp(tRPAREN) << " ";
    node->thenBlock()->visit(this);

    BlockNode* elseBlock = node->elseBlock();
    if (elseBlock) {
        m_out << " else";
        elseBlock->visit(this);
    }
}

void AstPrinter::visitForNode(ForNode* node) {
    m_out << "for " << tokenOp(tLPAREN);

    m_out << node->var()->name() << " in ";
    node->inExpr()->visit(this);

    m_out << tokenOp(tRPAREN);

    node->body()->visit(this);
}

void AstPrinter::visitWhileNode(WhileNode* node) {
    m_out << "while " << tokenOp(tLPAREN);
    node->whileExpr()->visit(this);
    m_out << tokenOp(tRPAREN);
    node->loopBlock()->visit(this);
}

void AstPrinter::visitBlockNode(BlockNode* node) {
    ++m_depth;
    if(m_depth - 1 > 0) {
        m_out << tokenOp(tLBRACE);
        printNewline();
    }

    Scope::VarIterator vars(node->scope());
    Scope::FunctionIterator functions(node->scope());
    uint32_t childCount = node->nodes();

    while (vars.hasNext()) {
        AstVar* variable = vars.next();
        m_out << typeToName(variable->type()) << " " << variable->name() << tokenOp(tSEMICOLON);
        if (vars.hasNext() || functions.hasNext() || childCount > 0) {
            printNewline();
        }
    }

    while (functions.hasNext()) {
        functions.next()->node()->visit(this);
        if (functions.hasNext() || childCount > 0) {
            printNewline();
        }
    }

    uint32_t i = 0;
    for ( ; i < childCount; ++i) {
        node->nodeAt(i)->visit(this);
        printNewline();
    }
    if (i != childCount) {
        node->nodeAt(i)->visit(this);
    }

    --m_depth;
    printNewline();
    if (m_depth > 0) {
        m_out << tokenOp(tRBRACE);
    }
}

void AstPrinter::visitReturnNode(ReturnNode* node) {
    m_out << "return";
    if (node->returnExpr()) {
        m_out << " ";
        node->returnExpr()->visit(this);
    }
    m_out << tokenOp(tSEMICOLON);
}

// functions, calls
void AstPrinter::visitFunctionNode(FunctionNode* node) {
    m_out << "function " << typeToName(node->returnType());
    m_out << " " << node->name() << tokenOp(tLPAREN);

    uint32_t paramCount = node->parametersNumber();
    uint32_t i = 0;
    for ( ; i + 1 < paramCount; ++i) {
        m_out << typeToName(node->parameterType(i)) << " ";
        m_out << node->parameterName(i) << tokenOp(tCOMMA) << " ";
    }
    if (i != paramCount) {
        m_out << typeToName(node->parameterType(i)) << " ";
        m_out << node->parameterName(i);
    }

    m_out << tokenOp(tRPAREN);

    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->visitChildren(this);
    } else {
        node->body()->visit(this);
    }
}

void AstPrinter::visitCallNode(CallNode* node) {
    m_out << node->name() << tokenOp(tLPAREN);

    uint32_t paramCount = node->parametersNumber();
    uint32_t i = 0;
    for ( ; i + 1 < paramCount; ++i) {
        node->parameterAt(i)->visit(this);
        m_out << tokenOp(tCOMMA) << " ";
    }
    if (i != paramCount) {
        node->parameterAt(i)->visit(this);
    }

    m_out << tokenOp(tRPAREN);
}

void AstPrinter::visitNativeCallNode(NativeCallNode* node) {
    m_out << " native '" << node->nativeName() << "'" << tokenOp(tSEMICOLON);
}

void AstPrinter::visitPrintNode(PrintNode* node) {
    m_out << "print" << tokenOp(tLPAREN);

    uint32_t operandCount = node->operands();
    uint32_t i = 0;
    for ( ; i + 1 < operandCount; ++i) {
        node->operandAt(i)->visit(this);
        m_out << tokenOp(tCOMMA) << " ";
    }
    if (i != operandCount) {
        node->operandAt(i)->visit(this);
    }

    m_out << tokenOp(tRPAREN) << tokenOp(tSEMICOLON);
}


// private
void AstPrinter::printEscaped(std::string const& str) {
    for (char c: str) {
        switch (c) {
            case '\n': m_out << "\\n"; break;
            case '\t': m_out << "\\t"; break;
            case '\r': m_out << "\\r"; break;
            case '\\': m_out << "\\";  break;
            case '\'': m_out << "\\'"; break;
            default  : m_out << c;     break;
        }
    }
}

void AstPrinter::printNewline() {
    m_out << std::endl;
    for (size_t i = 0; i + 1 < m_depth; ++i) {
        m_out << "  ";
    }
}

}   // namespace mathvm
