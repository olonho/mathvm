#ifndef BYTECODEEMITTINGASTVISITOR_H_
#define BYTECODEEMITTINGASTVISITOR_H_

#include "mathvm.h"
#include "ast.h"

#include "../ast2src/utils.hpp"
#include "BytecodeInstructionPrimitives.h"
#include <map>

// #define FUNCTION_ARGUMENTS_ON_STACK 1

namespace mathvm_ext {

using namespace mathvm;

class BytecodeEmittingAstVisitor : public AstVisitor {
  public:
    BytecodeEmittingAstVisitor(Code* output)
      : m_bytecode(0), m_code(output), m_scope(0) {
        m_var_storage.push_back("");
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
//            case tDOUBLE: return "";
//            case tINT: return "";
//            case tSTRING: return "";
//            case tCOMMA: return ",";
//            case tIDENT: return "";
//            case tERROR: return "";
//            case tUNDEF: return "";
//            default: return "";
//        }
    }

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
        node->right()->visit(this);
        VarType right_type = m_latest_type;
        node->left()->visit(this);
        VarType left_type = m_latest_type;

        switch (node->kind()) {
            case tADD:  // "+"
                m_latest_type = m_primitives.Add(m_bytecode, left_type, right_type);
                break;
            case tSUB:  // "-"
                m_latest_type = m_primitives.Sub(m_bytecode, left_type, right_type);
                break;
            case tMUL:  // "*"
                m_latest_type = m_primitives.Mul(m_bytecode, left_type, right_type);
                break;
            case tDIV:  // "/"
                m_latest_type = m_primitives.Div(m_bytecode, left_type, right_type);
                break;
            case tMOD:  // "%"
                m_latest_type = m_primitives.Mod(m_bytecode, left_type, right_type);
                break;
           case tAND:   // &&
                m_latest_type = m_primitives.And(m_bytecode, left_type, right_type);
                break;
            case tOR:    // "||"
                m_latest_type = m_primitives.Or(m_bytecode, left_type, right_type);
                break;
            case tEQ:   // "=="
                m_latest_type = m_primitives.CmpEq(m_bytecode, left_type, right_type);
                break;
            case tNEQ:  // "!=";
                m_latest_type = m_primitives.CmpNe(m_bytecode, left_type, right_type);
                break;
            case tGT:   // ">";
                m_latest_type = m_primitives.CmpGt(m_bytecode, left_type, right_type);
                break;
            case tGE:   // ">=";
                m_latest_type = m_primitives.CmpGe(m_bytecode, left_type, right_type);
                break;
            case tLT:   // "<";
                m_latest_type = m_primitives.CmpLt(m_bytecode, left_type, right_type);
                break;
            case tLE:   // "<=";
                m_latest_type = m_primitives.CmpLe(m_bytecode, left_type, right_type);
                break;
            case tRANGE:    // ".."
                std::cerr << "Error: Ranges are supported only as a FOR loop condition"
                          << std::endl;
                m_latest_type = m_primitives.Invalid(m_bytecode);
                break;
            default:
                m_latest_type = m_primitives.Invalid(m_bytecode);
                break;
        }
    }
    virtual void visitUnaryOpNode(UnaryOpNode* node) {
        node->operand()->visit(this);

        switch (node->kind()) {
            case tNOT:  // "!"
                m_latest_type = m_primitives.Not(m_bytecode, m_latest_type);
                break;
            case tSUB:  // "-"
                m_latest_type = m_primitives.Neg(m_bytecode, m_latest_type);
                break;
            default:
                std::cerr << "Error: Unknown AST node kind '"
                          << node->kind()
                          << "'"
                          << std::endl;
                m_latest_type = m_primitives.Invalid(m_bytecode);
        }
    }
    virtual void visitStringLiteralNode(StringLiteralNode* node) {
        uint16_t string_id = m_code->makeStringConstant(node->literal());
        m_latest_type = m_primitives.Load(m_bytecode, string_id, VT_STRING);
    }
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        m_latest_type = m_primitives.Load(m_bytecode, node->literal(), VT_DOUBLE);
    }
    virtual void visitIntLiteralNode(IntLiteralNode* node) {
        m_latest_type = m_primitives.Load(m_bytecode, node->literal(), VT_INT);
    }
    virtual void visitLoadNode(LoadNode* node) {
        // NOTE: nothing to visit - it's just a variable name

        // TODO: type checks, the following must not be possible (unless
        // we're doing implicit type casts):
        // int a; double b; a = b;
        // Now it is possible...

        m_latest_type = node->var()->type();
        uint16_t var_id = getVarStorage(node->var()->name());

         // load value from the VAR and push on the stack
        m_latest_type = m_primitives.LoadVar(m_bytecode, var_id, m_latest_type);
    }
    virtual void visitStoreNode(StoreNode* node) {
//        out << node->var()->name() << " = ";
        node->value()->visit(this);
        m_latest_type = node->var()->type();

        uint16_t var_id = getVarStorage(node->var()->name());

        switch (node->op()) {
            case tASSIGN:
                m_primitives.StoreVar(m_bytecode, var_id, m_latest_type);
                break;
            case tINCRSET:
                m_primitives.Inc(m_bytecode, var_id, m_latest_type);
                break;
            case tDECRSET:
                m_primitives.Dec(m_bytecode, var_id, m_latest_type);
                break;
            default:
                m_latest_type = m_primitives.Invalid(m_bytecode);
                break;
        }
    }
    virtual void visitForNode(ForNode* node) {
//        out << "for ( " << node->var()->name()
//                        << " in ";
//        node->inExpr()->visit(this);
//        out << ") ";
//        node->body()->visit(this);

        BinaryOpNode* range = (BinaryOpNode*) node->inExpr();
        assert(range->isBinaryOpNode() && range->kind() == tRANGE);

        addVarStorage(node->var()->name());
        range->left()->visit(this);
        VarType left_type = m_latest_type;

        uint16_t loop_var_id = getVarStorage(node->var()->name());
        m_primitives.StoreVar(m_bytecode, loop_var_id, left_type);

        Label entryLoopLabel(m_bytecode);
        Label exitLoopLabel(m_bytecode);

        // check loop condition
        m_bytecode->bind(entryLoopLabel);
        range->right()->visit(this);
        m_primitives.LoadVar(m_bytecode, loop_var_id, left_type);
        m_primitives.JmpGt(m_bytecode, exitLoopLabel);

        // execute loop body and increment the counter
        node->body()->visit(this);
        m_primitives.Load(m_bytecode, (int64_t) 1, left_type);
        m_primitives.Inc(m_bytecode, loop_var_id, left_type);
        m_primitives.Jmp(m_bytecode, entryLoopLabel);

        m_bytecode->bind(exitLoopLabel);
        m_var_storage.pop_back();
    }
    virtual void visitWhileNode(WhileNode* node) {
        Label entryLoopLabel(m_bytecode, m_bytecode->current());
        Label exitLoopLabel(m_bytecode);

        m_bytecode->addInsn(BC_ILOAD0);
        node->whileExpr()->visit(this);

        m_primitives.JmpEq(m_bytecode, exitLoopLabel);

        node->loopBlock()->visit(this);
        m_primitives.Jmp(m_bytecode, entryLoopLabel);

        m_bytecode->bind(exitLoopLabel);
    }
    virtual void visitIfNode(IfNode* node) {
        m_bytecode->addInsn(BC_ILOAD0);
        node->ifExpr()->visit(this);

        Label elseLabel(m_bytecode);
        Label ifExitLabel(m_bytecode);
        //jump if the condition evaluates to false
        m_primitives.JmpEq(m_bytecode, elseLabel);

        node->thenBlock()->visit(this);
        m_primitives.Jmp(m_bytecode, ifExitLabel);

        m_bytecode->bind(elseLabel);
        if (node->elseBlock()) {
            node->elseBlock()->visit(this);
        }

        m_bytecode->bind(ifExitLabel);
    }
    virtual void visitBlockNode(BlockNode* node) {
//        out << "{\n";
        Scope* old_scope = m_scope;
        m_scope = node->scope();

        convertScope(node->scope());

        for (uint32_t i = 0; i < node->nodes(); i++) {
            node->nodeAt(i)->visit(this);
        }

        convertScope2(node->scope());

        m_scope = old_scope;
//        out << "}\n";
    }

    std::vector<std::string> m_var_storage;

    void addVarStorage(const std::string &name) {
        m_var_storage.push_back(name);
    }

    uint16_t getVarStorage(const std::string &name) {
        for (size_t i = m_var_storage.size() - 1; i >= 0; i--) {
            if (m_var_storage[i] == name) {
                return i;
            }
        }
    }

    virtual void visitFunctionNode(FunctionNode* node) {
//        out << "function "
//            << mathvm::typeToName(node->returnType()) << " "
//            << node->name() << "(";

        BytecodeFunction *function = new BytecodeFunction(new AstFunction(node, node->body()->scope()));
        m_code->addFunction(function);

        Bytecode* old_bytecode = m_bytecode;
        m_bytecode = function->bytecode();

       // function's parameters
        uint32_t parameters_number = node->parametersNumber();
#ifdef FUNCTION_ARGUMENTS_ON_STACK
       if (parameters_number > 0) {
           uint32_t last_parameter = parameters_number - 1;
           for (uint32_t i = 0; i < last_parameter; ++i) {
               addVarStorage(node->parameterName(i));
           }
           addVarStorage(node->parameterName(last_parameter));
       }
#else
        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;
            for (uint32_t i = last_parameter; i > 0; --i) {
                addVarStorage(node->parameterName(i));
            }
            addVarStorage(node->parameterName(0));
        }
#endif

#ifdef FUNCTION_ARGUMENTS_ON_STACK
        for (uint32_t i = 0; i < parameters_number; ++i) {
            m_primitives.StoreVar(m_bytecode, getVarStorage(node->parameterName(i)),
                                              node->parameterType(i));
        }
#endif

        node->body()->visit(this);

        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;
            for (uint32_t i = 0; i < last_parameter; ++i) {
                m_var_storage.pop_back();
            }
            m_var_storage.pop_back();
        }

        m_bytecode = old_bytecode;
    }
    virtual void visitReturnNode(ReturnNode* node) {
//        out << "return";
       if (node->returnExpr()) {
           node->returnExpr()->visit(this);

           // move return value to VAR0
           m_primitives.StoreVar(m_bytecode, 0, m_latest_type);
       }
        // return address is on top the stack now
        m_bytecode->addInsn(BC_RETURN);
    }
    virtual void visitCallNode(CallNode* node) {
        uint32_t parameters_number = node->parametersNumber();
        uint16_t var_id = m_var_storage.size();
        if (parameters_number > 0) {
            uint32_t last_parameter = parameters_number - 1;

            // uint16_t var_id = m_var_storage.size();
            for (uint32_t i = last_parameter; i > 0; --i) {
                node->parameterAt(i)->visit(this);
#ifndef FUNCTION_ARGUMENTS_ON_STACK
                m_primitives.StoreVar(m_bytecode, var_id++, m_latest_type);
#endif
            }
            node->parameterAt(0)->visit(this);
#ifndef FUNCTION_ARGUMENTS_ON_STACK
            m_primitives.StoreVar(m_bytecode, var_id++, m_latest_type);
#endif
        }

        // resolve function by name
        TranslatedFunction *function = m_code->functionByName(node->name());
        if (!function) {
            //TODO: better error-handling
            assert(function && "Unresolved function name\n");
        }
        m_latest_type = function->returnType();

        // call function
        uint16_t function_id = function->id();
        m_bytecode->addInsn(BC_CALL);  //BC_CALL must push return address on the stack
        m_bytecode->addUInt16(function_id);

#ifdef FUNCTION_ARGUMENTS_ON_STACK
        // remove function arguments from the stack (byte by byte)
        for (uint32_t i = 0; i < parameters_number; ++i) {
            // m_primitives.Pop(m_bytecode, function->parameterType(i));
            m_primitives.StoreVar(m_bytecode, var_id - 1, function->parameterType(i));
        }
#endif

        // move function return value from VAR0 to the top of the stack
        if (m_latest_type != VT_VOID) { // TODO: this should be, probably, checked by LoadVar
            m_latest_type = m_primitives.LoadVar(m_bytecode, 0, m_latest_type);
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
            m_primitives.Print(m_bytecode, parameter_types[parameters_number - i - 1]);
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
            AstVar* var = var_iterator.next();
//            std::cout << mathvm::typeToName(var->type()) << " "
//              << var->name() << ";\n";
            addVarStorage(std::string(var->name()));
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

    void convertScope2(Scope* scope) {
         Scope::VarIterator var_iterator(scope);
         while (var_iterator.hasNext()) {
            AstVar* var = var_iterator.next();
            var = var;  // suppress debugger's warning
//            std::cout << mathvm::typeToName(var->type()) << " "
//              << var->name() << ";\n";
            m_var_storage.pop_back();
         }
    }

  private:
    Bytecode* m_bytecode;
    Code* m_code;
    Scope* m_scope;
    VarType m_latest_type;

    BytecodeInstructionPrimitives m_primitives;
};

}   // namespace mathvm_ext

#endif /* BYTECODEEMITTINGASTVISITOR_H_ */
