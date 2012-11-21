#ifndef BYTECODEEMITTINGASTVISITOR_H_
#define BYTECODEEMITTINGASTVISITOR_H_

#include "mathvm.h"
#include "ast.h"

#include "../ast2src/utils.hpp"
#include "BytecodeInstructionPrimitives.h"

namespace mathvm_ext {

using namespace mathvm;

class BytecodeEmittingAstVisitor : public AstVisitor {
  public:
    BytecodeEmittingAstVisitor(Code* output)
      : m_bytecode(0), m_code(output), m_scope(0) {
    }
    virtual ~BytecodeEmittingAstVisitor() {
    }

    Code* operator()(AstFunction* main_func) {
        // we're doing
//        main_func->node()->body()->visit(this);
        // and not
         main_func->node()->visit(this);
        // because name of the top-level function in AST is not a valid
        // identifier in the language, so we output only function's body
        // (the global scope). Calls Visitor::visitBlockNode
        return m_code;
    }

    Code* operator()(AstNode* ast) {
        ast->visit(this);
        return m_code;
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
        VarType right_type = m_latest_type;
        node->left()->visit(this);
        VarType left_type = m_latest_type;

        if (right_type != left_type) {
            std::cerr << "Error: Type mismatch: left is "
                      << typeToName(left_type)
                      << " but right is"
                      << typeToName(right_type)
                      << std::endl;
        }

        switch (node->kind()) {
            case tADD: {    // "+"
                Instruction instr = BC_INVALID;
                switch (m_latest_type) {
                    case VT_INVALID:
                        instr = BC_INVALID;
                        std::cerr << "Error: Invalid AST var type '"
                                  << m_latest_type
                                  << "'"
                                  << std::endl;
                        break;
                    case VT_DOUBLE:
                        instr = BC_DADD;
                        break;
                    case VT_INT:
                        instr = BC_IADD;
                        break;
                    case VT_VOID:
                        instr = BC_INVALID;
                        break;
                    case VT_STRING:
                        instr = BC_INVALID;
                        break;
                    default:
                        instr = BC_INVALID;
                        std::cerr << "Error: Unknown AST var type '"
                                  << m_latest_type
                                  << "'"
                                  << std::endl;
                        break;
                }
                m_bytecode->addInsn(instr);
                break;
            }
            case tSUB: {    // "-"
                m_bytecode->addInsn(BC_DSUB);
                break;
            }
            case tMUL: {    // "*"
                m_bytecode->addInsn(BC_DMUL);
                break;
            }
            case tDIV: {    // "/"
                m_bytecode->addInsn(BC_DDIV);
                break;
            }
            case tMOD: {    // "%"
                m_bytecode->addInsn(BC_IMOD);
                break;
            }
//            case tAND: {    // &&
//                break;
//            }
//            case tOR: { // "||"
//                break;
//            }
            case tEQ: { // "=="
                m_primitives.CmpEq(m_bytecode);
                break;
            }
            case tNEQ: {    // "!=";
                m_primitives.CmpNeq(m_bytecode);
                break;
            }
            case tGT: { // ">";
                m_primitives.CmpGt(m_bytecode);
                break;
            }
            case tGE: { // ">=";
                m_primitives.CmpGe(m_bytecode);
                break;
            }
            case tLT: { // "<";
                m_primitives.CmpLt(m_bytecode);
                break;
            }
            case tLE: { // "<=";
                m_primitives.CmpLe(m_bytecode);
                break;
            }
            default: {
                m_bytecode->addInsn(BC_INVALID);
                break;
            }
        }
    }
    virtual void visitUnaryOpNode(UnaryOpNode* node) {
//        out << " " << tokenOp(node->kind()) << "(";
        node->operand()->visit(this);
//        out << ")";

        switch (node->kind()) {
            case tNOT: {    // "!"
                m_primitives.Not(m_bytecode);
                break;
            }

            default: {
                m_bytecode->addInsn(BC_INVALID);
                std::cerr << "Error: Unknown AST node kind '"
                          << node->kind()
                          << "'"
                          << std::endl;
            }
        }
    }
    virtual void visitStringLiteralNode(StringLiteralNode* node) {
//        uint16_t string_id = m_code->makeStringConstant(escape_all(node->literal()));
        uint16_t string_id = m_code->makeStringConstant(node->literal());

        m_bytecode->addInsn(BC_SLOAD);
        m_bytecode->addUInt16(string_id);
        m_latest_type = VT_STRING;
    }
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        m_bytecode->addInsn(BC_DLOAD);
        m_bytecode->addTyped(node->literal());
        m_latest_type = VT_DOUBLE;
    }
    virtual void visitIntLiteralNode(IntLiteralNode* node) {
        m_bytecode->addInsn(BC_ILOAD);
        m_bytecode->addTyped(node->literal());
        m_latest_type = VT_INT;
    }
    virtual void visitLoadNode(LoadNode* node) {
//        out << node->var()->name();

        // load value from the VAR and push on the stack
        Instruction instr;
        switch (node->var()->type()) {
            case VT_INVALID:
                instr = BC_INVALID;
                break;
            case VT_VOID:
                instr = BC_INVALID;
                break;
            case VT_DOUBLE:
                instr = BC_LOADDVAR;
                break;
            case VT_INT:
                instr = BC_LOADIVAR;
                break;
            case VT_STRING:
                instr = BC_LOADSVAR;
                break;
            default:
                instr = BC_INVALID;
                std::cerr << "Error: Unknown AST var type '"
                          << node->var()->type()
                          << "'"
                          << std::endl;
                break;
        }
        m_latest_type = node->var()->type();

        m_bytecode->addInsn(instr);
        uint16_t var_id = 0;    //TODO: get from the scope (by node->var()->name())
        m_bytecode->addUInt16(var_id);

    }
    virtual void visitStoreNode(StoreNode* node) {
//        out << node->var()->name() << " = ";
        node->value()->visit(this);

        // get value from the top of the stack and store in the VAR
        Instruction instr;
        switch (node->var()->type()) {
            case VT_INVALID:
                instr = BC_INVALID;
                break;
            case VT_VOID:
                instr = BC_INVALID;
                break;
            case VT_DOUBLE:
                instr = BC_STOREDVAR;
                break;
            case VT_INT:
                instr = BC_STOREIVAR;
                break;
            case VT_STRING:
                instr = BC_STORESVAR;
                break;
            default:
                instr = BC_INVALID;
                break;
        }
        m_latest_type = node->var()->type();

        m_bytecode->addInsn(instr);
        uint16_t var_id = 0;    //TODO: get from the scope (by node->var()->name())
        m_bytecode->addUInt16(var_id);
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
        m_bytecode->addInsn(BC_ILOAD0);
        node->ifExpr()->visit(this);

        // upper is 0, lower is the condition
        // jump if upper >= lower, i.e. jump if the condition evaluates to false
        m_bytecode->addInsn(BC_IFICMPGE);
        //TODO: add IFICMPGE argument (offset/dst to else block)

        node->thenBlock()->visit(this);

        if (node->elseBlock()) {
//            out << "else ";
            node->elseBlock()->visit(this);
        }
    }
    virtual void visitBlockNode(BlockNode* node) {
//        out << "{\n";
        Scope* old_scope = m_scope;
        m_scope = node->scope();

        convertScope(node->scope());

        for (uint32_t i = 0; i < node->nodes(); i++) {
            node->nodeAt(i)->visit(this);
        }

        m_scope = old_scope;
//        out << "}\n";
    }
    virtual void visitFunctionNode(FunctionNode* node) {
//        out << "function "
//            << mathvm::typeToName(node->returnType()) << " "
//            << node->name() << "(";

        // AstFunction ast_func(node, node->body()->scope());
        // BytecodeFunction *function = new BytecodeFunction(&ast_func);
        BytecodeFunction *function = new BytecodeFunction(new AstFunction(node, node->body()->scope()));
        m_code->addFunction(function);

        Bytecode* old_bytecode = m_bytecode;
        m_bytecode = function->bytecode();

//
       // function's parameters
       // uint32_t parameters_number = node->parametersNumber();
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
        node->body()->visit(this);

        m_bytecode = old_bytecode;
    }
    virtual void visitReturnNode(ReturnNode* node) {
//        out << "return";
       if (node->returnExpr()) {
           node->returnExpr()->visit(this);

           Instruction instr;
           switch (m_latest_type) {
               case VT_INVALID:
                   instr = BC_INVALID;
                   std::cerr << "Error: Invalid AST var type '"
                             << m_latest_type
                             << "'"
                             << std::endl;
                   break;
               case VT_VOID:
                   instr = BC_INVALID;
                   break;
               case VT_DOUBLE:
                   instr = BC_STOREDVAR0;
                   break;
               case VT_INT:
                   instr = BC_STOREIVAR0;
                   break;
               case VT_STRING:
                   instr = BC_STORESVAR0;
                   break;
               default:
                   instr = BC_INVALID;
                   std::cerr << "Error: Unknown AST var type '"
                             << m_latest_type
                             << "'"
                             << std::endl;
                   break;
           }
           m_bytecode->addInsn(instr);   // move return value to VAR0
       }
        // return address is on top the stack now
        m_bytecode->addInsn(BC_RETURN);
    }
    virtual void visitCallNode(CallNode* node) {
        // push arguments on the stack
        uint32_t parameters_number = node->parametersNumber();
        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;
            for (uint32_t i = last_parameter; i > 0; --i) {
                node->parameterAt(i)->visit(this);
            }
            node->parameterAt(0)->visit(this);
        }

        // resolve function by name
        TranslatedFunction *function = m_code->functionByName(node->name());
        if (!function) {
            //TODO: better error-handling
            assert(function && "Unresolved function name\n");
        }

        // call function
        uint16_t function_id = function->id();
        m_bytecode->addInsn(BC_CALL);  //BC_CALL must push return address on the stack
        m_bytecode->addUInt16(function_id);

        const size_t DatatypeSize[] = {0,   // VT_INVALID
                                       0,   // VT_VOID
                                       sizeof(double),  // VT_DOUBLE
                                       sizeof(int64_t), // VT_INT
                                       sizeof(uint16_t) // VT_STRING
        };

        // remove function arguments from the stack (byte by byte)
        for (uint32_t i = 0; i < parameters_number; ++i) {
            for (uint32_t j = 0; j < DatatypeSize[function->parameterType(i)]; ++j) {
                m_bytecode->addInsn(BC_POP);
            }
        }

        // move function return value from VAR0 to the top of the stack
        switch (function->returnType()) {
            case VT_INVALID:
                m_bytecode->addInsn(BC_INVALID);
                break;
            case VT_VOID:
                // do nothing
                break;
            case VT_DOUBLE:
                m_bytecode->addInsn(BC_LOADDVAR0);
                break;
            case VT_INT:
                m_bytecode->addInsn(BC_LOADIVAR0);
                break;
            case VT_STRING:
                m_bytecode->addInsn(BC_LOADSVAR0);
                break;
            default:
                m_bytecode->addInsn(BC_INVALID);
                break;
        }
    }
    virtual void visitNativeCallNode(NativeCallNode* node) {
        //TODO:
        // move function return value from VAR0 on TOS
//        m_bytecode->addInsn(BC_LOADDVAR0);
    }
    virtual void visitPrintNode(PrintNode* node) {
        // push operands on top the stack in the right-to-left order
        uint32_t parameters_number = node->operands();
        std::vector<VarType> parameter_types;
        parameter_types.reserve(parameters_number);
        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;
            for (uint32_t i = last_parameter; i > 0; --i) {
                node->operandAt(i)->visit(this);
                parameter_types.push_back(m_latest_type);
            }
            node->operandAt(0)->visit(this);
            parameter_types.push_back(m_latest_type);
        }
        // emit needed number of print instructions
        for (uint32_t i = 0; i < parameters_number; ++i) {
//            m_bytecode->addInsn(BC_IPRINT);
            switch (parameter_types[parameters_number - i - 1]) {
                case VT_INVALID:
                    m_bytecode->addInsn(BC_INVALID);
                    break;
                case VT_VOID:
                    // do nothing
                    break;
                case VT_DOUBLE:
                    m_bytecode->addInsn(BC_DPRINT);
                    break;
                case VT_INT:
                    m_bytecode->addInsn(BC_IPRINT);
                    break;
                case VT_STRING:
                    m_bytecode->addInsn(BC_SPRINT);
                    break;
                default:
                    m_bytecode->addInsn(BC_INVALID);
                    break;
            }
        }
    }

    void convertScope(Scope* scope) {
//#if defined(_DEBUG_COMMENTS)
//        out << "// parent scope: " << scope->parent() << std::endl;
//
//        out << "// scope variables: \n";
//#endif
         Scope::VarIterator var_iterator(scope);
         while (var_iterator.hasNext()) {
             // AstVar* var = var_iterator.next();
             // out << mathvm::typeToName(var->type()) << " "
             //     << var->name() << ";\n";
         }
//#if defined(_DEBUG_COMMENTS)
//        out << "// end of scope variables.\n";
//
//        out << "// scope functions: \n";
//#endif
        Scope::FunctionIterator function_iterator(scope);
        while (function_iterator.hasNext()) {
            AstFunction* function = function_iterator.next();
            (*this)(function->node());
        }
//#if defined(_DEBUG_COMMENTS)
//        out << "// end of scope functions.\n";
//#endif
    }

  private:
    Bytecode* m_bytecode;
    Code* m_code;
    Scope* m_scope;
    VarType m_latest_type;

    static const uint8_t InsnSize[];

    BytecodeInstructionPrimitives m_primitives;
};

}   // namespace mathvm_ext

#endif /* BYTECODEEMITTINGASTVISITOR_H_ */
