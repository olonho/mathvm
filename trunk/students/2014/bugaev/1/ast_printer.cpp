#include "ast.h"
#include "parser.h"

#include <iomanip>
#include <cctype>


namespace mathvm
{

class AstPrinterVisitor: public AstVisitor
{
public:
    AstPrinterVisitor(ostream &out):
        m_nestingLevel(0),
        m_out(out)
    {
    }

    void visitBinaryOpNode(BinaryOpNode *node)
    {
        visitWithParens(node->left());
        m_out << " " << tokenOp(node->kind()) << " ";
        visitWithParens(node->right());
    }

    void visitUnaryOpNode(UnaryOpNode *node)
    {
        m_out << tokenOp(node->kind());
        visitWithParens(node->operand());
    }

    void visitStringLiteralNode(StringLiteralNode *node)
    {
        string const &s = node->literal();
        m_out << "'";
        for (size_t i = 0; i < s.size(); ++i) {
            int const c = s[i];
            switch (c) {
                case '\a': m_out << "\\a";  continue;
                case '\b': m_out << "\\b";  continue;
                case '\f': m_out << "\\f";  continue;
                case '\n': m_out << "\\n";  continue;
                case '\r': m_out << "\\r";  continue;
                case '\t': m_out << "\\t";  continue;
                case '\v': m_out << "\\v";  continue;
                case '\\': m_out << "\\\\"; continue;
                case '\'': m_out << "\\'";  continue;
            }
            if (!isprint(c)) {
                m_out << "\\";
                m_out << setfill('0') << setw(3) << oct << noshowbase << c;
                continue;
            }
            m_out << s[i];
        }
        m_out << "'";
    }

    void visitDoubleLiteralNode(DoubleLiteralNode *node)
    {
        m_out << node->literal();
    }

    void visitIntLiteralNode(IntLiteralNode *node)
    {
        m_out << node->literal();
    }

    void visitLoadNode(LoadNode *node)
    {
        m_out << node->var()->name();
    }

    void visitStoreNode(StoreNode *node)
    {
        m_out << node->var()->name() << " " << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    void visitForNode(ForNode *node)
    {
        m_out << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        m_out << ")\n";
        node->body()->visit(this);
    }

    void visitWhileNode(WhileNode *node)
    {
        m_out << "while (";
        node->whileExpr()->visit(this);
        m_out << ")\n";
        node->loopBlock()->visit(this);
    }

    void visitIfNode(IfNode *node)
    {
        m_out << "if (";
        node->ifExpr()->visit(this);
        m_out << ")\n";

        node->thenBlock()->visit(this);

        if (node->elseBlock()) {
            printTabs();
            m_out << "else\n";
            node->elseBlock()->visit(this);
        }
    }

    void visitBlockNode(BlockNode *node)
    {
        printTabs();
        m_out << "{\n";

        ++m_nestingLevel;
        visitBlockMembers(node);
        --m_nestingLevel;

        printTabs();
        m_out << "}\n";
    }

    void visitFunctionNode(FunctionNode *node)
    {
        if (node->name() == AstFunction::top_name) {
            visitBlockMembers(node->body());
            return;
        }

        m_out << "function " << typeToName(node->returnType()) << " "
              << node->name() << "(";
        for (size_t i = 0; i < node->parametersNumber(); ++i) {
            if (i != 0) m_out << ", ";
            m_out << typeToName(node->parameterType(i)) << " "
                  << node->parameterName(i);
        }
        m_out << ")\n";
        node->body()->visit(this);
    }

    void visitReturnNode(ReturnNode *node)
    {
        m_out << "return";
        if (node->returnExpr()) {
            m_out << " ";
            node->returnExpr()->visit(this);
        }
    }

    void visitCallNode(CallNode *node)
    {
        m_out << node->name() << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if (i != 0) m_out << ", ";
            node->parameterAt(i)->visit(this);
        }
        m_out << ")";
    }

    void visitNativeCallNode(NativeCallNode *node)
    {
        m_out << "native '" << node->nativeName() << "'";
    }

    void visitPrintNode(PrintNode *node)
    {
        m_out << "print(";
        for (uint32_t i = 0; i < node->operands(); ++i) {
            if (i != 0) m_out << ", ";
            node->operandAt(i)->visit(this);
        }
        m_out << ")";
    }

private:
    void visitWithParens(AstNode *node)
    {
        bool parens = node->isBinaryOpNode() || node->isStoreNode();
        if (parens) m_out << "(";
        node->visit(this);
        if (parens) m_out << ")";
    }

    void visitBlockMembers(BlockNode *node)
    {
        Scope::VarIterator varIter(node->scope());
        while (varIter.hasNext()) {
            AstVar const *astVar = varIter.next();

            printTabs();
            m_out << typeToName(astVar->type()) << " "
                  << astVar->name() << ";\n";
        }

        for (uint32_t i = 0; i < node->nodes(); ++i) {
            printTabs();
            node->nodeAt(i)->visit(this);
            printSemicolon(node->nodeAt(i));
        }
    }

    void printTabs()
    {
        m_out << string(m_nestingLevel, '\t');
    }

    void printSemicolon(AstNode *node)
    {
        bool skip = node->isForNode() || node->isWhileNode()
                || node->isIfNode() || node->isBlockNode()
                || node->isFunctionNode();
        if (!skip) m_out << ";\n";
    }

private:
    int m_nestingLevel;
    ostream &m_out;
};


class AstPrinter: public Translator
{
public:
    Status *translate(string const &program, Code* *code)
    {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status && status->isError()) return status;

        AstPrinterVisitor visitor(cout);
        parser.top()->node()->visit(&visitor);

        return Status::Ok();
    }
};


Translator *Translator::create(string const &impl)
{
    if (impl == "printer")
        return new AstPrinter();
    return 0;
}

}
