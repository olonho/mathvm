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
    BytecodeEmittingAstVisitor(Code* output);
    virtual ~BytecodeEmittingAstVisitor();

    Code* operator()(AstFunction* main_func);
    Code* operator()(AstNode* ast);

  private:

    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);
    
    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    
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
        removeLatestVarStorage();
    }
    
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);

    virtual void visitBlockNode(BlockNode* node) {
        Scope* old_scope = m_scope;
        m_scope = node->scope();

        enterScope(m_scope);

        for (uint32_t i = 0; i < node->nodes(); i++) {
            node->nodeAt(i)->visit(this);
        }

        leaveScope(m_scope);

        m_scope = old_scope;
    }

    virtual void visitFunctionNode(FunctionNode* node) {
//        out << "function "
//            << mathvm::typeToName(node->returnType()) << " "
//            << node->name() << "(";

        BytecodeFunction *function = new BytecodeFunction(new AstFunction(node, node->body()->scope()));
        m_code->addFunction(function);

        Bytecode* old_bytecode = m_bytecode;
        m_bytecode = function->bytecode();

        // function's parameters passing (right-to-left)
        uint32_t parameters_number = node->parametersNumber();
        uint32_t last_parameter = parameters_number - 1;
        for (uint32_t i = 0; i < parameters_number; ++i) {
            addVarStorage(node->parameterName(last_parameter - i));
        }

        node->body()->visit(this);

        for (uint32_t i = 0; i < parameters_number; ++i) {
            removeLatestVarStorage();
        }

        m_bytecode = old_bytecode;
    }

    virtual void visitReturnNode(ReturnNode* node);

    virtual void visitCallNode(CallNode* node) {
        uint32_t parameters_number = node->parametersNumber();
        uint32_t last_parameter = parameters_number - 1;

        uint16_t var_id = getFirstUnusedVarId();

        for (uint32_t i = 0; i < parameters_number; ++i) {
            node->parameterAt(last_parameter - i)->visit(this);
            m_primitives.StoreVar(m_bytecode, var_id++, m_latest_type);
        }

        // resolve function by name
        TranslatedFunction *function = m_code->functionByName(node->name());
        assert(function && "Unresolved function name\n");
        m_latest_type = function->returnType();

        // call function
        uint16_t function_id = function->id();
        m_bytecode->addInsn(BC_CALL);  //BC_CALL must push return address on the stack
        m_bytecode->addUInt16(function_id);

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

    virtual void visitPrintNode(PrintNode* node);

    void enterScope(Scope* scope) {
        // reserve storage for all scope variables
        Scope::VarIterator var_iterator(scope);
        while (var_iterator.hasNext()) {
            AstVar* var = var_iterator.next();
            // std::cout << mathvm::typeToName(var->type()) << " "
            //           << var->name() << ";\n";
            addVarStorage(std::string(var->name()));
        }
        // generate bytecode for scope functions
        Scope::FunctionIterator function_iterator(scope);
        while (function_iterator.hasNext()) {
            AstFunction* function = function_iterator.next();
            (*this)(function->node());
        }
    }

    void leaveScope(Scope* scope) {
        // unregister scope variables
        Scope::VarIterator var_iterator(scope);
        while (var_iterator.hasNext()) {
            var_iterator.next();
            // std::cout << mathvm::typeToName(var->type()) << " "
            //           << var->name() << ";\n";
            removeLatestVarStorage();
        }
    }

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

    void removeLatestVarStorage() {
        m_var_storage.pop_back();
    }

    uint16_t getFirstUnusedVarId() {
        return m_var_storage.size();
    }

  private:
    Bytecode* m_bytecode;
    Code* m_code;
    Scope* m_scope;
    VarType m_latest_type;

    std::vector<std::string> m_var_storage;

    BytecodeInstructionPrimitives m_primitives;
};

}   // namespace mathvm_ext

#endif /* BYTECODEEMITTINGASTVISITOR_H_ */
