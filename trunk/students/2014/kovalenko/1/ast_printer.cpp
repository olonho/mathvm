#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "ast.h"

namespace mathvm {

class PrinterVisitor : public AstVisitor {
public:

    virtual void visitBinaryOpNode(BinaryOpNode *node) {
        out << "(";
        node -> left() -> visit(this);
        out << " " << tokenOp(node -> kind()) << " ";
        node -> right() -> visit(this);
        out << ")";
    }


    virtual void visitUnaryOpNode(UnaryOpNode *node) {
        out << tokenOp(node -> kind());
        node -> operand() -> visit(this);
    }


    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) {
        out << showpoint << node -> literal();
    }


    virtual void visitIntLiteralNode(IntLiteralNode *node) {
      out << node -> literal();
    }


    virtual void visitLoadNode(LoadNode *node) {
      out << node -> var() -> name();
    }


    virtual void visitStringLiteralNode(StringLiteralNode *node) {
        out << "'" << escapeString(node -> literal()) << "'";
    }


    virtual void visitNativeCallNode(NativeCallNode *node) {
        out << " native '" << node -> nativeName() << "';" << endl;
    }


    virtual void visitStoreNode(StoreNode *node) {
        out << node -> var() -> name() << " " << tokenOp(node -> op()) << " ";
        node -> value() -> visit(this);
    }


    virtual void visitIfNode(IfNode *node) {
        out << "if (";
        node -> ifExpr() -> visit(this);
        out << ") ";
        node -> thenBlock() -> visit(this);
        if (node -> elseBlock()) {
            out << "else ";
            node -> elseBlock() -> visit(this);
        }
    }


    virtual void visitForNode(ForNode *node) {
        out << "for (" << node -> var() -> name() << " in ";
        node -> inExpr() -> visit(this);
        out << ") ";
        node -> body() -> visit(this);
    }


    virtual void visitWhileNode(WhileNode *node) {
        out << "while (";
        node -> whileExpr() -> visit(this);
        out << ") ";
        node -> loopBlock() -> visit(this);
    }


    virtual void visitBlockNode(BlockNode *node) {
        bool isTopLevelNode = (node -> scope() -> parent() -> parent() == NULL);
        if (!isTopLevelNode) {out << "{" << endl;}
        printVariableDefinitions(node -> scope());

        for (uint32_t i = 0; i < node -> nodes(); i++) {
            AstNode *curNode = node -> nodeAt(i);
            curNode -> visit(this);
            if (curNode -> isLoadNode() || curNode -> isStoreNode() ||
                curNode -> isNativeCallNode() || curNode -> isPrintNode() ||
                curNode -> isReturnNode() || curNode -> isCallNode()){
                  out << ";" << endl;
                }
        }

        if (!isTopLevelNode) {out << "}" << endl;}
    }


    virtual void visitFunctionNode(FunctionNode *node) {
        if (node -> name() != AstFunction::top_name) {
            out << "function " << typeToName(node -> returnType()) << " " << node -> name() << "(";

            for (uint32_t i = 0; i < node -> parametersNumber(); i++) {
                out << typeToName(node -> parameterType(i)) << " " << node -> parameterName(i);
                if (i + 1 < node -> parametersNumber()) {
                    out << ", ";
                }
            }

            out << ") ";
        }
        node -> body() -> visit(this);
    }


    virtual void visitCallNode(CallNode *node) {
        out << node -> name() << "(";
        for (uint32_t i = 0; i < node -> parametersNumber(); i++) {
            node -> parameterAt(i) -> visit(this);
            if (i + 1 < node -> parametersNumber()) {
                out << ", ";
            }
        }
        out << ")";
    }


    virtual void visitReturnNode(ReturnNode *node) {
        out << "return ";
        if (node -> returnExpr()) {
            node -> returnExpr() -> visit(this);
        }
    }


    virtual void visitPrintNode(PrintNode *node) {
        out << "print(";
        for (uint32_t i = 0; i < node -> operands(); i++) {
            node -> operandAt(i) -> visit(this);
            if (i + 1 < node -> operands()) {
                out << ", ";
            }
        }
        out << ")";
    }


    PrinterVisitor(ostream &stream) : out(stream) {}
    

private:

    std::ostream &out;

    std::string escapeString(std::string const &str) {
        std::string res;
        for (size_t i = 0; i < str.length(); ++i) {
            switch (str[i]) {
                case '\n': res.append("\\n"); break;
                case '\r': res.append("\\r"); break;
                case '\t': res.append("\\t"); break;
                default: res+=str[i]; break;
            }
        }
        return res;
    }

    void printVariableDefinitions(Scope *scope){
          Scope::VarIterator iter(scope);
          while (iter.hasNext()) {
              AstVar *var = iter.next();
              out << typeToName(var -> type()) << " " << var -> name() << ";" << endl;
          }
    }
};

class AstPrinter : public Translator {
public:
    virtual Status *translate(const string &program, Code **)
    {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status && status -> isError()) {
            return status;
        }
        PrinterVisitor pVisitor(cout);
        parser.top() -> node() -> visit(&pVisitor);
        return new Status();
    }
};

Translator *Translator::create(const string &impl)
{
    if (impl == "printer") {
        return new AstPrinter();
    } else {
        return NULL;
    }
}
}
