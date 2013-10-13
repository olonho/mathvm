#include <iostream>
#include <memory>

#include "mathvm.h"
#include "parser.h"
#include "ast.h"

using namespace mathvm;

class PrintVisitor : public AstVisitor {
private:
    
    
    uint32_t indentationLevel;
    std::ostream& out;

    std::string indent() {
        return std::string(indentationLevel * 4, ' ');
    }
    
public:
    
    explicit PrintVisitor(std::ostream& out) : indentationLevel(0), out(out) {}
    
    ~PrintVisitor() {}
    
    PrintVisitor(const PrintVisitor& other) : out(other.out) {}
    
    void visitBinaryOpNode(BinaryOpNode* node) {
        out << "(";
        node->left()->visit(this);
        out << ")";
        out << " " << tokenOp(node->kind()) << " ";
        out << "(";
        node->right()->visit(this);
        out << ")";
    }

    void visitUnaryOpNode(UnaryOpNode* node) {
        out << tokenOp(node->kind());
        node->operand()->visit(this);
    }
    
    void visitStringLiteralNode(StringLiteralNode* node) {
        out << "'";
        for (auto c : node->literal()) {
            switch(c) {
                case '\n':
                    out << "\\n";
                    break;
                case '\t':
                    out << "\\t";
                    break;
                case '\f':
                    out << "\\f";
                    break;
                case '\r':
                    out << "\\r";
                    break;
                default:
                    out << c;
            }
        }
        out << "'";
    }

    void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        out << std::fixed << node->literal();
    } 
    
    void visitIntLiteralNode(IntLiteralNode* node) {
        out << node->literal();
    }

    void visitLoadNode(LoadNode* node) {
        out << node->var()->name();
    }

    void visitStoreNode(StoreNode* node) {
        out << node->var()->name() 
            << " "
            << tokenOp(node->op())
            << " ";
        node->visitChildren(this);
    }

    void visitForNode(ForNode* node) {
        out << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        out << ") ";
        node->body()->visit(this);
    }

    void visitWhileNode(WhileNode* node) {
        out << "while (";
        node->whileExpr()->visit(this);
        out << ") ";
        node->loopBlock()->visit(this);
    }
    
    void visitIfNode(IfNode* node) {
        out << "if (";
        node->ifExpr()->visit(this);
        out << ") ";
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            out << "else ";
            node->elseBlock()->visit(this);
        }
    }

    void visitBlockNode(BlockNode* node) {
        out << "{" << std::endl;
        ++indentationLevel;
        visitBlockNodeNoIndent(node);
        --indentationLevel;
        out << indent();
        out << "}" << std::endl;
    }

    void visitBlockNodeNoIndent(BlockNode* node) {
        for (auto it = Scope::VarIterator(node->scope()); it.hasNext();) {
            AstVar* variable = it.next();
            out << indent();
            out << typeToName(variable->type()) 
                << " " 
                << variable->name() 
                << ";" 
                << std::endl;
        }
        out << std::endl;
        for (auto it = Scope::FunctionIterator(node->scope()); it.hasNext();) {
            it.next()->node()->visit(this);
        }
        for (uint32_t i = 0; i < node->nodes(); ++i) {
            out << indent();
            node->nodeAt(i)->visit(this); 
            out << ";" << std::endl;
        }
    }

    void visitFunctionNode(FunctionNode* node) {
        out << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if (i != 0) {
                out << ", ";
            }
            out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        }
        out << ")";
        if (node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->visit(this);
        } else {
            node->body()->visit(this);
        }
        out << std::endl;
    }

    void visitReturnNode(ReturnNode* node) {
        out << "return";
        if (node->returnExpr()) {
            out << " ";
            node->returnExpr()->visit(this);
        }
    }

    void visitCallNode(CallNode* node) {
        out << node->name() << "(";
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if (i != 0) {
                out << ", ";
            }
            node->parameterAt(i)->visit(this);
        }
        out << ")";
    } 

    void visitNativeCallNode(NativeCallNode* node) {
        out << " native '" << node->nativeName() << "';";
    }
    
    void visitPrintNode(PrintNode* node) {
        out << "print(";
        for (uint32_t i = 0; i < node->operands(); ++i) {
            if (i != 0) {
                out << ",";
            }
            node->operandAt(i)->visit(this);
        }
        out << ")";
    }

};


int main(int argc, char** argv) {

    const char* program = 0;

    if (argc == 2) {
        program = argv[1];
    } else {
        std::cout << "Usage: " << argv[0] << " program.mvm" << std::endl;
        return 0;
    }
    
    const char* text = loadFile(program);
    
    Parser parser;
    Status* status = parser.parseProgram(text);
    
    if (status != 0 && status->isError()) {
        std::cerr << "Parse error: " << status->getError() << std::endl;
        return 1;
    }
    
    PrintVisitor printVisitor(std::cout);
    printVisitor.visitBlockNodeNoIndent(parser.top()->node()->body());
    return 0;
}

