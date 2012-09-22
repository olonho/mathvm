#include <iostream>
#include <ostream>
#include <ios>

#include "mathvm.h"
#include "parser.h"

namespace mathvm {

void reconstruct_scope(mathvm::Scope* scope, std::ostream& out) {
    mathvm::Scope::VarIterator var_iterator(scope);
    while (var_iterator.hasNext()) {
        AstVar* var = var_iterator.next();
        out << mathvm::typeToName(var->type()) << " "
            << var->name() << ";\n";
    }
    // mathvm::Scope::FunctionIterator function_iterator(scope);
    // while (function_iterator.hasNext()) {
    //     mathvm::AstFunction* function = function_iterator.next();
    // }
}

class Ast2SrcConverter : public AstVisitor {
  public:
    Ast2SrcConverter() : out(std::cout) {
        out.setf(std::ios::showpoint);
    }
    virtual ~Ast2SrcConverter() {
    }

    void convert(AstNode* ast) {
        ast->visit(this);
    }

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
        out << "(";
        node->left()->visit(this);
        out << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
        out << ")";
    }
    virtual void visitUnaryOpNode(UnaryOpNode* node) {
        out << " " << tokenOp(node->kind()) << "(";
        node->operand()->visit(this);
        out << ")";
    }
    virtual void visitStringLiteralNode(StringLiteralNode* node) {
        out << node->literal();
    }
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        out << node->literal();
    }
    virtual void visitIntLiteralNode(IntLiteralNode* node) {
        out << node->literal();
    }
    virtual void visitLoadNode(LoadNode* node) {
        out << node->var()->name();
    }
    virtual void visitStoreNode(StoreNode* node) {
        out << node->var()->name() << " = ";
        node->value()->visit(this);
        out << ";\n";
    }
    virtual void visitForNode(ForNode* node) {
        out << "for ( " << node->var()
                        << " in ";
        node->inExpr()->visit(this);
        out << ") {\n";
        node->body()->visit(this);
        out << "}\n";
    }
    virtual void visitWhileNode(WhileNode* node) {
        out << "while (";
        node->whileExpr()->visit(this);
        out << ") {\n";
        node->loopBlock()->visit(this);
        out << "}\n";
    }
    virtual void visitIfNode(IfNode* node) {
        out << "if (";
        node->ifExpr()->visit(this);
        out << ") {\n";
        node->thenBlock()->visit(this);
        out << "}";
        if (node->elseBlock()) {
            out << " else {\n";
            node->elseBlock()->visit(this);
            out << "}";
        }
        out << "\n";
    }
    virtual void visitBlockNode(BlockNode* node) {
        reconstruct_scope(node->scope(), out);
        node->visitChildren(this);
    }
    virtual void visitFunctionNode(FunctionNode* node) {
        out << "function "
            << mathvm::typeToName(node->returnType()) << " "
            << node->name() << "(";

        // function's parameters
        uint32_t parameters_number = node->parametersNumber();
        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;
            for (uint32_t i = 0; i < last_parameter; ++i) {
                out << mathvm::typeToName(node->parameterType(i)) << " "
                    << node->parameterName(i) << ", ";
            }
            out << mathvm::typeToName(node->parameterType(last_parameter)) << " "
                << node->parameterName(last_parameter);
        }
        out << ") {\n";

        node->body()->visit(this);

        out << "}\n";
    }
    virtual void visitReturnNode(ReturnNode* node) {
        out << "return";
        if (node->returnExpr()) {
            out << " ";
            node->returnExpr()->visit(this);
        }
        out << ";";
    }
    virtual void visitCallNode(CallNode* node) {
        out << node->name() << "(";

        uint32_t parameters_number = node->parametersNumber();
        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;
            for (uint32_t i = 0; i < last_parameter; ++i) {
                node->parameterAt(i)->visit(this);
                out << ", ";
            }
            node->parameterAt(last_parameter)->visit(this);
        }

        out << ")";
    }
    virtual void visitNativeCallNode(NativeCallNode* node) {
        std::string name = node->nativeName();
        Signature signature = node->nativeSignature();
        VarType return_type = signature[0].first;
        out << "function "
            << mathvm::typeToName(return_type) << " "
            << name << "(";

        uint32_t parameters_number = signature.size() - 1;
        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;
            for (uint32_t i = 1; i < last_parameter; ++i) {
                out << mathvm::typeToName(signature[i].first) << " "
                    << signature[i].second << ", ";
            }
            out << mathvm::typeToName(signature[last_parameter].first) << " "
                                << signature[last_parameter].second << ", ";
        }

        out << ") native " << "\'" << name << "\';";
    }
    virtual void visitPrintNode(PrintNode* node) {
        out << "print(";

        uint32_t parameters_number = node->operands();
        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;
            for (uint32_t i = 0; i < last_parameter; ++i) {
                node->operandAt(i)->visit(this);
                out << ", ";
            }
            node->operandAt(last_parameter)->visit(this);
        }

        out << ");\n";
    }
  private:
    std::ostream& out;
};

}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 2) {
        std::cout << "Usage: " << argv[0] << " <program source code file>"
                  << std::endl;
        return 1;
    }

    mathvm::Parser parser;
    const std::string code(mathvm::loadFile(argv[1]));

    mathvm::Status* status = parser.parseProgram(code);
    if (status && status->isError()) {
        uint32_t line = 0;
        uint32_t offset = 0;
        mathvm::positionToLineOffset(code, status->getPosition(), line, offset);
        std::cerr << status->getError() << " at (" << line << ":" << offset << ")";
        return 2;
    }

    mathvm::AstFunction* function = parser.top();
    mathvm::AstNode* functions_ast = function->node();
    mathvm::Ast2SrcConverter reconstructor;
    reconstructor.convert(functions_ast);

    return 0;
}
