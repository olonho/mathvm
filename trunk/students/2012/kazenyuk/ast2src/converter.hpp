#ifndef AST2SRC_CONVERTER_HPP_
#define AST2SRC_CONVERTER_HPP_

#include <ostream>
#include <ios>

#include "mathvm.h"

#include "utils.hpp"

//#define _DEBUG_COMMENTS 1

namespace mathvm_ext {

using namespace mathvm;

class Ast2SrcConverter : public AstVisitor {
  public:
    Ast2SrcConverter(std::ostream& output = std::cout) : out(output) {
        out.setf(std::ios::showpoint);
    }
    virtual ~Ast2SrcConverter() {
    }

    std::ostream &operator()(AstFunction* main_func) {
        main_func->node()->body()->visit(this);
        return out;
    }

    std::ostream &operator()(AstNode* ast) {
        ast->visit(this);
        return out;
    }

  private:
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
        out << "\'" << escape_all(node->literal()) << "\'";
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
    }
    virtual void visitForNode(ForNode* node) {
        out << "for ( " << node->var()->name()
                        << " in ";
        node->inExpr()->visit(this);
        out << ") ";
        node->body()->visit(this);
    }
    virtual void visitWhileNode(WhileNode* node) {
        out << "while (";
        node->whileExpr()->visit(this);
        out << ") ";
        node->loopBlock()->visit(this);
    }
    virtual void visitIfNode(IfNode* node) {
        out << "if (";
        node->ifExpr()->visit(this);
        out << ") ";
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            out << "else ";
            node->elseBlock()->visit(this);
        }
    }
    virtual void visitBlockNode(BlockNode* node) {
        out << "{\n";
        convertScope(node->scope());

        for (uint32_t i = 0; i < node->nodes(); i++) {
            node->nodeAt(i)->visit(this);
            if (!node->nodeAt(i)->isBlockNode()
                && !node->nodeAt(i)->isIfNode()) {
                out << ";\n";
            }
        }
        out << "}\n";
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
        out << ") ";

        node->body()->visit(this);
    }
    virtual void visitReturnNode(ReturnNode* node) {
        out << "return";
        if (node->returnExpr()) {
            out << " ";
            node->returnExpr()->visit(this);
        }
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

        out << ")";
    }

    void convertScope(Scope* scope) {
#if defined(_DEBUG_COMMENTS)
        out << "// parent scope: " << scope->parent() << std::endl;
    
        out << "// scope variables: \n";
#endif 
        Scope::VarIterator var_iterator(scope);
        while (var_iterator.hasNext()) {
            AstVar* var = var_iterator.next();
            out << mathvm::typeToName(var->type()) << " "
                << var->name() << ";\n";
        }
#if defined(_DEBUG_COMMENTS)
        out << "// end of scope variables.\n";

        out << "// scope functions: \n";
#endif
        Scope::FunctionIterator function_iterator(scope);
        while (function_iterator.hasNext()) {
            AstFunction* function = function_iterator.next();
            (*this)(function->node());
        }
#if defined(_DEBUG_COMMENTS)
        out << "// end of scope functions.\n";
#endif
    }

  private:
    std::ostream& out;
};

}

#endif  // AST2SRC_CONVERTER_HPP_
