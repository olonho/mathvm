#include "parser.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace mathvm;

class AstPrinter : public AstVisitor {

std::ostream& _out;
int _indent;

public:

    AstPrinter(std::ostream& out) : _out(out), _indent(-1) {
    }

    ~AstPrinter() { 
    }

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
        _out << "(";
        node->left()->visit(this);
        _out << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
        _out << ")";
    }

    virtual void visitUnaryOpNode(UnaryOpNode* node) {
        _out << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode* node) {
        std::string oldlit = node->literal();
        std::string lit;
        for(uint32_t index = 0; index < oldlit.size(); ++index) {
            switch(oldlit[index]) {
                case '\t':
                    lit += "\\t";
                    break;
                case '\r':
                    lit += "\\r";
                    break;
                case '\n':
                    lit += "\\n";
                    break;
                case '\\':
                    lit += "\\\\";
                    break;
                default:
                    lit += oldlit[index];
            }
        }
        _out << "'" << lit << "'";
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        _out << node->literal();
    }

    virtual void visitIntLiteralNode(IntLiteralNode* node) {
        _out << node->literal();
    }

    virtual void visitLoadNode(LoadNode* node) {
        _out << node->var()->name(); 
    }

    virtual void visitStoreNode(StoreNode* node) {
        makeIndent();
        _out << node->var()->name() 
             << " " 
             << tokenOp(node->op())
             << " ";

        node->visitChildren(this);
        _out << ";" << std::endl;
    }

    virtual void visitForNode(ForNode* node) {
        makeIndent();
        _out << "for(" 
             << node->var()->name()
             << " in ";
        node->inExpr()->visit(this);
        _out << ")" << std::endl;
        node->body()->visit(this);
    }

    virtual void visitWhileNode(WhileNode* node) {
        makeIndent();        
        _out << "while(";
        node->whileExpr()->visit(this);
        _out << ")" << std::endl;
        node->loopBlock()->visit(this);
    }

    virtual void visitIfNode(IfNode* node) {
        makeIndent();        
        _out << "if(";
        node->ifExpr()->visit(this);
        _out << ")" << std::endl;
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            makeIndent();
            _out << "else\n";
            node->elseBlock()->visit(this);
        }
    }

    virtual void visitBlockNode(BlockNode* node) {
        makeIndent();
        if (_indent != -1) _out << "{" << std::endl;

        ++_indent;

        Scope* scope = node->scope();

        Scope::VarIterator variter(scope);

        while(variter.hasNext()) {
            makeIndent();
            AstVar* var = variter.next();
            _out << typeToName(var->type()) 
                 << " " << var->name() 
                 << ";" << std::endl;
        }

        Scope::FunctionIterator funciter(scope);

        while(funciter.hasNext()) {
            makeIndent();
            AstFunction* func = funciter.next();
            func->node()->visit(this);
        }

        uint32_t size = node->nodes();
        for(uint32_t index = 0; index < size; ++index) {
            if (isPrimitiveExpr(node->nodeAt(index))) {
                makeIndent();
                node->nodeAt(index)->visit(this);
                _out << ";" << std::endl;
            } else {
                node->nodeAt(index)->visit(this);
            }
        }

        --_indent;
        makeIndent();
        if (_indent != -1) _out << "}" << std::endl;
    }

    virtual void visitFunctionNode(FunctionNode* node) {
        makeIndent();

        _out << "function " 
             << typeToName(node->returnType())
             << " "
             << node->name()
             << "(";

        uint32_t count = node->parametersNumber();
        for (uint32_t index = 0; index < count; ++index) {
            _out << typeToName(node->parameterType(index))
                 << " "
                 << node->parameterName(index);
            if (index != count - 1) {
                _out << ", ";
            }
        }

        _out << ")"; 
        uint32_t size = node->body()->nodes();
        if (size) {
            if (node->body()->nodeAt(0)->isNativeCallNode()) {
                _out << " native '"
                     << node->name()
                     << "';"
                     << std::endl;
            } else {
                _out << std::endl;
                node->visitChildren(this);
            } 
            return;
        }
        node->visitChildren(this);
    }

    virtual void visitReturnNode(ReturnNode* node) {
        AstNode* expr = node->returnExpr();
        if (expr) {
            makeIndent();
            _out << "return ";
            expr->visit(this);
            _out << ";" << std::endl;
        }
    }

    virtual void visitCallNode(CallNode* node) {
        _out << node->name() << "(";
        uint32_t size = node->parametersNumber();
        for (uint32_t index = 0; index < size; ++index) {
            node->parameterAt(index)->visit(this);
            if (index != size - 1) {
                _out << ", ";
            }
        }
        _out << ")";
    }

    virtual void visitPrintNode(PrintNode* node) {
        makeIndent();
        _out << "print(";
        uint32_t size = node->operands();
        for (uint32_t index = 0; index < size; ++index) {
            node->operandAt(index)->visit(this);
            if (index != size - 1) {
                _out << ", ";
            }
        }
        _out << ");" << std::endl;
    }

    void makeIndent() {
        for (int i = 0; i < _indent; ++i) {
            _out << "    ";
        }
    }

    bool isPrimitiveExpr(AstNode* node) {
        if (node->isLoadNode() || 
            node->isIntLiteralNode() ||
            node->isDoubleLiteralNode() ||
            node->isStringLiteralNode() ||
            node->isBinaryOpNode() ||
            node->isUnaryOpNode() ||
            node->isCallNode()) {

            return true;
        }
        return false;
    }

    void run(AstFunction* func) {
        func->node()->body()->visit(this);
    }
};

int main(int argc, char const *argv[]) {

    if (argc != 2) {
        std::cerr << "USAGE: <source filename>" << std::endl;
    }

    Parser parser;

    std::string filename = argv[1];

    std::ifstream input(filename.c_str());
    if (!input) {
        std::cerr << "File: " << filename << "  does not exist" 
                  << std::endl;
    }

    std::stringstream stream;
    stream << input.rdbuf();
    std::string source(stream.str());

    if (Status* s = parser.parseProgram(source)) {
        std::cout << "There is some error while parsing."  
                  << s->getError() << std::endl;
        return 1;
    }

    AstPrinter printer(std::cout);
    AstFunction* topFunc = parser.top();

    printer.run(topFunc);

    return 0;
}