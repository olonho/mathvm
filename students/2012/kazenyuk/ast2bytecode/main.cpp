#include <iostream>
#include <string>
#include "mathvm.h"
#include "ast.h"
#include "../ast2src/utils.hpp"

namespace mathvm {

class InterpreterCodeImpl : public Code {
  public:
    virtual Status* execute(std::vector<mathvm::Var*>&) {
      return new Status("InterpreterCodeImpl: Unimplemented");
    }
};

Status* BytecodeTranslatorImpl::translateBytecode(const string& program, InterpreterCodeImpl* *code) {
    return new Status("BytecodeTranslatorImpl: Unimplemented");
}

Status* BytecodeTranslatorImpl::translate(const std::string& program, Code* *code) {
    return new Status("BytecodeTranslatorImpl: Unimplemented");
}

}

namespace mathvm_ext {

using namespace mathvm;

class BytecodeEmittingAstVisitor : public AstVisitor {
  public:
    BytecodeEmittingAstVisitor(Bytecode *output) : out(output) {
    }
    virtual ~BytecodeEmittingAstVisitor() {
    }

    Bytecode *operator()(AstFunction* main_func) {
        main_func->node()->body()->visit(this);
        return out;
    }

    Bytecode *operator()(AstNode* ast) {
        ast->visit(this);
        return out;
    }

  private:

    void bytecodeByTokenKind(TokenKind token) {
//        switch (token) {
//        case tEOF: return "";
//            case tLPAREN: return "(";
//            case tRPAREN: return ")";
//            case tLBRACE: return "{";
//            case tRBRACE: return "}";
//            case tASSIGN: return "=";
//            case tOR: {
////                return "||";
//
//            }
//            case tAND: {
////                return "&&";
//            }
//            case tNOT: return "!";
//            case tEQ: return "==";
//            case tNEQ: return "!=";
//            case tGT: return ">";
//            case tGE: return ">=";
//            case tLT: return "<";
//            case tLE: return "<=";
//            case tRANGE: return "..";
//            case tINCRSET: return "+=";
//            case tDECRSET: return "-=";
//            case tDOUBLE: return "";
//            case tINT: return "";
//            case tSTRING: return "";
//            case tCOMMA: return ",";
//            case tSEMICOLON: return ";";
//            case tIDENT: return "";
//            case tERROR: return "";
//            case tUNDEF: return "";
//            default: return "";
//        }
    }

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
//        out << "(";
//        node->left()->visit(this);
//        out << " " << tokenOp(node->kind()) << " ";
//        node->right()->visit(this);
//        out << ")";
        node->right()->visit(this);
        node->left()->visit(this);
        switch (node->kind()) {
            case tADD: {
                out->addInsn(BC_DADD);
            //                return "+";
            }
            case tSUB: {
                out->addInsn(BC_DSUB);
            //                return "-";
            }
            case tMUL: {
                out->addInsn(BC_DMUL);
            //                return "*";
            }
            case tDIV: {
                out->addInsn(BC_DDIV);
//                return "/";
            }
            case tMOD: {
                out->addInsn(BC_IMOD);
//                return "%";
            }
            default: {
                out->addInsn(BC_INVALID);
            }
        }
    }
    virtual void visitUnaryOpNode(UnaryOpNode* node) {
//        out << " " << tokenOp(node->kind()) << "(";
//        node->operand()->visit(this);
//        out << ")";
    }
    virtual void visitStringLiteralNode(StringLiteralNode* node) {
//        out << "\'" << escape_all(node->literal()) << "\'";

        out->addInsn(BC_SLOAD);
        uint16_t string_id = out_code->makeStringConstant(escape_all(node->literal()));
        out->addUInt16(string_id);
    }
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
//        out << node->literal();
        out->addInsn(BC_DLOAD);
        out->addDouble(node->literal());
    }
    virtual void visitIntLiteralNode(IntLiteralNode* node) {
//        out << node->literal();
        out->addInsn(BC_ILOAD);
        out->addDouble(node->literal());
    }
    virtual void visitLoadNode(LoadNode* node) {
//        out << node->var()->name();
    }
    virtual void visitStoreNode(StoreNode* node) {
//        out << node->var()->name() << " = ";
//        node->value()->visit(this);
    }
    virtual void visitForNode(ForNode* node) {
//        out << "for ( " << node->var()->name()
//                        << " in ";
//        node->inExpr()->visit(this);
//        out << ") ";
//        node->body()->visit(this);
    }
    virtual void visitWhileNode(WhileNode* node) {
//        out << "while (";
//        node->whileExpr()->visit(this);
//        out << ") ";
//        node->loopBlock()->visit(this);
    }
    virtual void visitIfNode(IfNode* node) {
//        out << "if (";
//        node->ifExpr()->visit(this);
//        out << ") ";
//        node->thenBlock()->visit(this);
//        if (node->elseBlock()) {
//            out << "else ";
//            node->elseBlock()->visit(this);
//        }
    }
    virtual void visitBlockNode(BlockNode* node) {
//        out << "{\n";
//        convertScope(node->scope());
//
//        for (uint32_t i = 0; i < node->nodes(); i++) {
//            node->nodeAt(i)->visit(this);
//            if (!node->nodeAt(i)->isBlockNode()
//                && !node->nodeAt(i)->isIfNode()) {
//                out << ";\n";
//            }
//        }
//        out << "}\n";
    }
    virtual void visitFunctionNode(FunctionNode* node) {
//        out << "function "
//            << mathvm::typeToName(node->returnType()) << " "
//            << node->name() << "(";
//
//        // function's parameters
//        uint32_t parameters_number = node->parametersNumber();
//        if (parameters_number > 0) {
//            uint32_t last_parameter = parameters_number - 1;
//            for (uint32_t i = 0; i < last_parameter; ++i) {
//                out << mathvm::typeToName(node->parameterType(i)) << " "
//                    << node->parameterName(i) << ", ";
//            }
//            out << mathvm::typeToName(node->parameterType(last_parameter)) << " "
//                << node->parameterName(last_parameter);
//        }
//        out << ") ";
//
//        node->body()->visit(this);
    }
    virtual void visitReturnNode(ReturnNode* node) {
//        out << "return";
//        if (node->returnExpr()) {
//            out << " ";
//            node->returnExpr()->visit(this);
//        }
        out->addInsn(BC_RETURN);
    }
    virtual void visitCallNode(CallNode* node) {
//        out << node->name() << "(";
//
//        uint32_t parameters_number = node->parametersNumber();
//        if (parameters_number > 0) {
//            uint32_t last_parameter = parameters_number - 1;
//            for (uint32_t i = 0; i < last_parameter; ++i) {
//                node->parameterAt(i)->visit(this);
//                out << ", ";
//            }
//            node->parameterAt(last_parameter)->visit(this);
//        }
//
//        out << ")";
    }
    virtual void visitNativeCallNode(NativeCallNode* node) {
//        std::string name = node->nativeName();
//        Signature signature = node->nativeSignature();
//        VarType return_type = signature[0].first;
//        out << "function "
//            << mathvm::typeToName(return_type) << " "
//            << name << "(";
//
//        uint32_t parameters_number = signature.size() - 1;
//        if (parameters_number > 0) {
//            uint32_t last_parameter = parameters_number - 1;
//            for (uint32_t i = 1; i < last_parameter; ++i) {
//                out << mathvm::typeToName(signature[i].first) << " "
//                    << signature[i].second << ", ";
//            }
//            out << mathvm::typeToName(signature[last_parameter].first) << " "
//                << signature[last_parameter].second << ", ";
//        }
//
//        out << ") native " << "\'" << name << "\';";
    }
    virtual void visitPrintNode(PrintNode* node) {
//        out << "print(";
//
//        uint32_t parameters_number = node->operands();
//        if (parameters_number > 0) {
//            uint32_t last_parameter = parameters_number - 1;
//            for (uint32_t i = 0; i < last_parameter; ++i) {
//                node->operandAt(i)->visit(this);
//                out << ", ";
//            }
//            node->operandAt(last_parameter)->visit(this);
//        }
//
//        out << ")";

        // push operands on top the stack in the right-to-left order
        // poping and printing them immediately
        uint32_t parameters_number = node->operands();
        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;
            for (uint32_t i = last_parameter; i > 0; --i) {
                node->operandAt(i)->visit(this);
            }
            node->operandAt(0)->visit(this);
            out->addInsn(BC_DPRINT);
        }
    }

    void convertScope(Scope* scope) {
//#if defined(_DEBUG_COMMENTS)
//        out << "// parent scope: " << scope->parent() << std::endl;
//
//        out << "// scope variables: \n";
//#endif
//        Scope::VarIterator var_iterator(scope);
//        while (var_iterator.hasNext()) {
//            AstVar* var = var_iterator.next();
//            out << mathvm::typeToName(var->type()) << " "
//                << var->name() << ";\n";
//        }
//#if defined(_DEBUG_COMMENTS)
//        out << "// end of scope variables.\n";
//
//        out << "// scope functions: \n";
//#endif
//        Scope::FunctionIterator function_iterator(scope);
//        while (function_iterator.hasNext()) {
//            AstFunction* function = function_iterator.next();
//            (*this)(function->node());
//        }
//#if defined(_DEBUG_COMMENTS)
//        out << "// end of scope functions.\n";
//#endif
    }

  private:
    Bytecode* out;
    Code* out_code;
};

class BytecodeGenerator : public BytecodeTranslatorImpl {
//    Status* translateBytecode(const std::string& program, InterpreterCodeImpl* *code) {
//        return new Status("BytecodeTranslatorImpl: Unimplemented");
//    }
  public:
    virtual Status* translate(const std::string& program, Code* *code) {
      if (*code == 0) {
          *code = new InterpreterCodeImpl();
      }
      Code *c = *code;
      FunctionNode *function_ast_node = 0;
      BytecodeFunction *func = new BytecodeFunction(new AstFunction(function_ast_node, 0/*function_ast_node->body()->scope()->parent()*/));
//      Bytecode *func_bytecode = func->bytecode();

      c->addFunction(func);
      return new Status("BytecodeTranslatorImpl: Unimplemented");
    }
};

}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 2) {
        std::cout << "Usage: " << argv[0] << " <program source code file>"
                  << std::endl;
        return 1;
    }

    char* text_buffer = mathvm::loadFile(argv[1]);
    if (!text_buffer) {
        std::cout << "Can't read file '" << argv[0] << "'"
                  << std::endl;
        return 2;
    }

    const std::string text(text_buffer);
    mathvm::Code* code = 0;

    mathvm::Translator* translator = new mathvm_ext::BytecodeGenerator();
    mathvm::Status* translateStatus = translator->translate(text, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        mathvm::positionToLineOffset(text, position, line, offset);
        std::cerr << "Cannot translate expression: expression at "
                  << line << "," << offset
                  << "; error '"
                  << translateStatus->getError().c_str() << "'"
                  << std::endl;
        delete translateStatus;
        delete translator;
        return 1;
    }
    delete translateStatus;
    delete translator;

    return 0;
}
