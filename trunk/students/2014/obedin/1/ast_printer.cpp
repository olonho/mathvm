#include <iostream>
#include <ast.h>
#include <visitors.h>
#include <mathvm.h>
#include <parser.h>

using namespace mathvm;
using std::endl;


#define FOR_NODES_WITHOUT_BODY(DO)              \
            DO(BinaryOpNode, "binary")          \
            DO(UnaryOpNode, "unary")            \
            DO(StringLiteralNode, "string literal") \
            DO(DoubleLiteralNode, "double literal") \
            DO(IntLiteralNode, "int literal")   \
            DO(LoadNode, "loadval")             \
            DO(StoreNode, "storeval")           \
            DO(ReturnNode, "return")            \
            DO(CallNode, "call")                \
            DO(NativeCallNode, "native call")   \
            DO(PrintNode, "print")


string escape(const string& str) {
    string escaped;
    for (size_t i = 0; i < str.size(); i++) {
        switch (str[i]) {
        case '\n':
            escaped += "\\n"; break;
        case '\r':
            escaped += "\\r"; break;
        case '\\':
            escaped += "\\\\"; break;
        case '\t':
            escaped += "\\t"; break;
        default:
            escaped += str.substr(i, 1);
        }
    }
    return escaped;
}

class AstPrinterVisitor: public AstVisitor {
public:
    AstPrinterVisitor(std::ostream &out = std::cout)
        : m_out(out) {}

    void printVariables(Scope* scope) {
        Scope::VarIterator it(scope);
        while (it.hasNext()) {
            AstVar *var = it.next();
            m_out << typeToName(var->type()) << " "
                  << var->name() << ";" << endl;
        }
    }

    void printFunctions(Scope* scope) {
        Scope::FunctionIterator it(scope);
        while (it.hasNext())
            it.next()->node()->visit(this);
    }

    void printBlock(BlockNode *node) {
        for (uint32_t i = 0; i < node->nodes(); i++) {
            node->nodeAt(i)->visit(this);
            if (
#define TEST_TYPE(type, name) \
                    node->nodeAt(i)->is##type() ||
                FOR_NODES_WITHOUT_BODY(TEST_TYPE)
#undef TEST_TYPE
                    false) {
                m_out << ";" << endl;
            }
        }
    }

    virtual void visitBinaryOpNode(BinaryOpNode *node) {
        m_out << "(";
        node->left()->visit(this);
        m_out << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
        m_out << ")";
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node) {
        m_out << tokenOp(node->kind()) << " ";
        node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node) {
        m_out << "'" << escape(node->literal()) << "'";
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        m_out << node->literal();
    }

    virtual void visitIntLiteralNode(IntLiteralNode *node) {
        m_out << node->literal();
    }

    virtual void visitLoadNode(LoadNode *node) {
        m_out << node->var()->name();
    }

    virtual void visitStoreNode(StoreNode *node) {
        m_out << node->var()->name() << " "
              << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    virtual void visitForNode(ForNode *node) {
        m_out << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        m_out << ") ";
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode *node) {
        m_out << "while (";
        node->whileExpr()->visit(this);
        m_out << ") ";
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode *node) {
        m_out << "if (";
        node->ifExpr()->visit(this);
        m_out << ") ";
        node->thenBlock()->visit(this);
        if (node->elseBlock() != 0) {
            m_out << "else ";
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitBlockNode(BlockNode *node) {
        m_out << "{" << endl;
        printVariables(node->scope());
        printFunctions(node->scope());
        printBlock(node);
        m_out << "}" << endl;
    }

    virtual void visitFunctionNode(FunctionNode *node) {
        m_out << "function " << typeToName(node->returnType())
              << " " << node->name() << "(";

        uint32_t paramNum = node->parametersNumber();
        for (uint32_t i = 0; paramNum > 0 && i < paramNum-1; i++)
            m_out << typeToName(node->parameterType(i)) << " "
                  << node->parameterName(i) << ", ";
        if (paramNum > 0)
            m_out << typeToName(node->parameterType(paramNum-1)) << " "
                  << node->parameterName(paramNum-1);

        m_out << ") ";

        if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode())
            node->body()->nodeAt(0)->visit(this);
        else
            node->body()->visit(this);
    }

    virtual void visitReturnNode(ReturnNode *node) {
        m_out << "return";
        if (node->returnExpr() != 0) {
            m_out << " ";
            node->returnExpr()->visit(this);
        }
    }

    virtual void visitCallNode(CallNode *node) {
        m_out << node->name() << "(";

        uint32_t paramNum = node->parametersNumber();
        for (uint32_t i = 0; paramNum > 0 && i < paramNum-1; i++) {
            node->parameterAt(i)->visit(this);
            m_out << ", ";
        }
        if (paramNum > 0)
            node->parameterAt(paramNum-1)->visit(this);

        m_out << ")";
    }

    virtual void visitNativeCallNode(NativeCallNode *node) {
        m_out << " native '" << node->nativeName() << "';" << endl;
    }

    virtual void visitPrintNode(PrintNode *node) {
        m_out << "print(";

        uint32_t operandsNum = node->operands();
        for (uint32_t i = 0; operandsNum > 0 && i < operandsNum-1; i++) {
            node->operandAt(i)->visit(this);
            m_out << ", ";
        }
        if (operandsNum > 0)
            node->operandAt(operandsNum-1)->visit(this);

        m_out << ")";
    }

private:
    std::ostream &m_out;
};

class AstPrinter: public Translator {
public:
    virtual Status* translate(const string& program, Code** code) {
        Parser parser;
        Status* status = parser.parseProgram(program);
        if (status->isError())
            return status;

        printProgram(parser.top());
        return Status::Ok();
    }

    void printProgram(AstFunction *top) {
        AstPrinterVisitor visitor;
        Scope *topScope = top->scope()->childScopeAt(0);

        visitor.printVariables(topScope);
        visitor.printFunctions(topScope);
        visitor.printBlock(top->node()->body());
    }
};

Translator* Translator::create(const string& impl) {
    return new AstPrinter();
}
