#include <ostream>

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

namespace mathvm {

struct AstPrinterVisitor: public AstVisitor {
    AstPrinterVisitor(std::ostream & os):
        m_os(os)
    {}

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
        m_os << "(";
        node->left()->visit(this);
        m_os << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
        m_os << ")";
    }

    virtual void visitUnaryOpNode(UnaryOpNode* node) {
        m_os << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode* node) {
        std::string const & str = node->literal();

        m_os << "'";
        for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
            switch (*it) {

            case '\'': m_os << "\\'";   break;
            case '\"': m_os << "\\\"";  break;
            case '\\': m_os << "\\\\";  break;
            case '\?': m_os << "\\?";   break;
            case '\a': m_os << "\\a";   break;
            case '\b': m_os << "\\b";   break;
            case '\f': m_os << "\\f";   break;
            case '\n': m_os << "\\n";   break;
            case '\r': m_os << "\\r";   break;
            case '\t': m_os << "\\t";   break;
            case '\v': m_os << "\\v";   break;
            default:    m_os << *it;

            }
        }
        m_os << "'";
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) { m_os << node->literal(); }

    virtual void visitIntLiteralNode(IntLiteralNode* node) { m_os << node->literal(); }

    virtual void visitLoadNode(LoadNode* node) { m_os << node->var()->name(); }

    virtual void visitStoreNode(StoreNode* node) {
        m_os << node->var()->name() << " " << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    virtual void visitForNode(ForNode* node) {
        m_os << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        m_os << ")" << std::endl;
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode* node) {
        m_os << "while (";
        node->whileExpr()->visit(this);
        m_os << ")" << std::endl;
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode* node) {
        m_os << "if (";
        node->ifExpr()->visit(this);
        m_os << ")" << std::endl;
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            m_os << "else" << std::endl;
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitBlockNode(BlockNode* node) {
        m_os << "{" << std::endl;
        visitBlockElements(node);
        m_os << "}" << std::endl;
    }

    virtual void visitFunctionNode(FunctionNode* node) {
        if (node->name() == AstFunction::top_name) {
            visitBlockElements(node->body());
            return;
        }
        m_os << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
        for (size_t i = 0; i < node->parametersNumber(); ++i) {
            if (i != 0) m_os << ", ";
            m_os << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        }
        m_os << ")" << std::endl;
        node->body()->visit(this);
    }

    virtual void visitReturnNode(ReturnNode* node) {
        m_os << "return";
        if (node->returnExpr()) {
            m_os << " ";
            node->returnExpr()->visit(this);
        }
    }

    virtual void visitCallNode(CallNode* node) {
        m_os << node->name() << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if (i != 0) m_os << ", ";
            node->parameterAt(i)->visit(this);
        }
        m_os << ")";
    }

    virtual void visitNativeCallNode(NativeCallNode* node) { m_os << "native '" << node->nativeName() << "'"; }

    virtual void visitPrintNode(PrintNode* node) {
        m_os << "print(";
        for (uint32_t i = 0; i < node->operands(); ++i) {
            if (i != 0) m_os << ", ";
            node->operandAt(i)->visit(this);
        }
        m_os << ")";
    }

private:
    std::ostream & m_os;

    void visitBlockElements(BlockNode* node) {
        Scope::VarIterator varIt(node->scope());
        while (varIt.hasNext()) {
            AstVar const * var = varIt.next();
            m_os << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
        }
        for (uint32_t i = 0; i < node->nodes(); ++i) {
            node->nodeAt(i)->visit(this);
            if (needSemicolon(node->nodeAt(i))) m_os << ";" << std::endl;
        }
    }

    bool needSemicolon(AstNode* node) {
        return !(node->isForNode() ||
                 node->isWhileNode() ||
                 node->isIfNode() ||
                 node->isBlockNode() ||
                 node->isFunctionNode());
    }
};

struct AstPrinter: public Translator {
    Status *translate(string const &program, Code* *code) {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status && status->isError()) return status;
        AstPrinterVisitor visitor(cout);
        parser.top()->node()->visit(&visitor);
        return new Status();
    }
};

Translator* Translator::create(const string& impl) {
    if (impl == "printer") {
        return new AstPrinter();
    }
    return 0;
}

}
