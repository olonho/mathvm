#include "mathvm.h"
#include "parser.h"
#include "ast_printer.h"

namespace mathvm{

class AstPrinterVisitor : public AstVisitor {
public:
    AstPrinterVisitor(std::ostream& os) : indentationLevel(-1), os(os) {
    }
    virtual ~AstPrinterVisitor() {
    }

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
        os << "(";
        node->left()->visit(this);
        os << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
        os << ")";
    }

    virtual void visitUnaryOpNode(UnaryOpNode* node) {
        os << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode* node) {
        os << "\'" << escape(node->literal()) << "\'";
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        os << node->literal();
    }

    virtual void visitIntLiteralNode(IntLiteralNode* node) {
        os << node->literal();
    }

    virtual void visitLoadNode(LoadNode* node) {
        os << node->var()->name();
    }

    virtual void visitStoreNode(StoreNode* node) {
        os << node->var()->name();
        os << " " << tokenOp(node->op()) << " ";
        
        node->value()->visit(this);
    }

    virtual void visitForNode(ForNode* node) {
        os << "for (";
        os << node->var()->name();
        os << " in ";
        node->inExpr()->visit(this);
        os << ") ";
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode* node) {
        os << "while (";
        node->whileExpr()->visit(this);
        os << ") ";
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode* node) {
        os << "if (";
        node->ifExpr()->visit(this);
        os << ") ";
        node->thenBlock()->visit(this); 
        
        if(node->elseBlock()) {
            os << " else ";
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitBlockNode(BlockNode* node) {
        if(indentationLevel >= 0){
            os << "{" << endl;
        }
        indentationLevel++;
        
        printScope(node->scope());

        for(uint32_t i = 0; i < node->nodes(); ++i) {
            os << indent();
            node->nodeAt(i)->visit(this);

            if(needSemicolon(node->nodeAt(i))) {
                os << ';';
            }

            os << endl;
        }
        
        indentationLevel--;
        if(indentationLevel >= 0) {
            os << indent() << "}" << endl;
        }
    }

    virtual void visitFunctionNode(FunctionNode* node) {
        os << "function";
        os << " " << typeToName(node->returnType()) << " ";
        os << node->name();

        os << "(";
        for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if(i != 0) {
                os << ", ";
            }

            os << typeToName(node->parameterType(i)) << " ";
            os << node->parameterName(i);
        }
        os << ") ";
        
        if(node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->visit(this); 
        } else {
            node->body()->visit(this);
        }
    }

    virtual void visitReturnNode(ReturnNode* node) {
        os << "return";

        if(node->returnExpr()) {
            os << " ";
            node->returnExpr()->visit(this);
        } 
    }

    virtual void visitCallNode(CallNode* node) {
        os << node->name() << "(";

        for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if(i != 0) {
                os << ", ";
            }

            node->parameterAt(i)->visit(this);
        }

        os << ")";
    }

    virtual void visitNativeCallNode(NativeCallNode* node) {
        os << "native \'" << node->nativeName() << "\';" << endl;
    }

    virtual void visitPrintNode(PrintNode* node) {
        os << "print (";
        
        for(uint32_t i = 0; i < node->operands(); ++i) {
            if(i != 0) {
                os << ", ";
            }

            node->operandAt(i)->visit(this);
        }

        os << ")";
    }

    void printScope(Scope * scope) {
        Scope::VarIterator varIt(scope); 

        while(varIt.hasNext()) {
            AstVar* var = varIt.next();

            os << indent();
            os << typeToName(var->type()) << " " << var->name() << ";" << endl;
        }

        Scope::FunctionIterator funcIt(scope);
        while(funcIt.hasNext()) {
            AstFunction* func = funcIt.next();
            
            os << indent();
            func->node()->visit(this);
        }
    }

private:
    int indentationLevel;
    std::ostream& os;

    string indent() {
        return string(indentationLevel, '\t');
    }

    bool needSemicolon(AstNode * node) {
        bool endsWithBraces = 
            node->isForNode() ||
            node->isWhileNode() ||
            node->isIfNode() ||
            node->isBlockNode() ||
            node->isFunctionNode();

        return !endsWithBraces;
    }

    string escape(string const & str) {
        string res = "";

        for(uint32_t i = 0; i < str.size(); ++i) {
            switch(str[i]) {
                case '\'':
                    res += "\\'"; break;
                case '\"':
                    res += "\\\""; break;
                case '\?':
                    res += "\\?"; break;
                case '\\':
                    res += "\\\\"; break;
                case '\a':
                    res += "\\a"; break;
                case '\b':
                    res += "\\b"; break;
                case '\f':
                    res += "\\f"; break;
                case '\n':
                    res += "\\n"; break;
                case '\r':
                    res += "\\r"; break;
                case '\t':
                    res += "\\t"; break;
                case '\v':
                    res += "\\v"; break;
                default:
                    res += str[i];
            }
        }

        return res;
    }
};

Status* AstPrinter::translate(const string& program, Code* *code) {
    Parser parser;
    Status* status = parser.parseProgram(program);

    if(status->isError()) {
        return status;
    }

    AstPrinterVisitor visitor(std::cout);
    
    FunctionNode* root = parser.top()->node();
    root->body()->visit(&visitor);

    return Status::Ok();
}       

}

