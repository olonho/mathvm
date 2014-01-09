#ifndef TRANSLATORVISITOR_HPP
#define TRANSLATORVISITOR_HPP

#include "visitors.h"
#include "codeimpl.hpp"

#include "context.hpp"

#include "logger.hpp"

namespace mathvm {

class TranslatorVisitor : public AstVisitor {
public:
    TranslatorVisitor(Context* context, BytecodeFunction* function = NULL);

    virtual void visitBinaryOpNode(BinaryOpNode *node);
    virtual void visitBlockNode(BlockNode *node);
    virtual void visitCallNode(CallNode *node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node);
    virtual void visitForNode(ForNode *node);
    virtual void visitFunctionNode(FunctionNode *node);
    virtual void visitIfNode(IfNode *node);
    virtual void visitIntLiteralNode(IntLiteralNode *node);
    virtual void visitLoadNode(LoadNode *node);
    virtual void visitNativeCallNode(NativeCallNode *node);
    virtual void visitPrintNode(PrintNode *node);
    virtual void visitReturnNode(ReturnNode *node);
    virtual void visitStoreNode(StoreNode *node);
    virtual void visitStringLiteralNode(StringLiteralNode *node);
    virtual void visitUnaryOpNode(UnaryOpNode *node);
    virtual void visitWhileNode(WhileNode *node);

    void run(AstFunction *node);

private:
    Bytecode* bc() const {
        return m_function->bytecode();
    }

    Context* m_context;
    BytecodeFunction* m_function;

    VarType m_last_type;
    VarType m_expected_return;


    void checkType(AstNode* node, VarType expected);


    // helper codegenerator functions
    void store_var(var_t varid, VarType type, AstNode* node) {
        if (varid.first != m_context->id()) { //store context var
            switch (type) {
            case VT_INT:
                bc()->addInsn(BC_STORECTXIVAR);
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_STORECTXDVAR);
                break;
            case VT_STRING:
                bc()->addInsn(BC_STORECTXSVAR);
                break;
            default:
                throw TranslationError("Couldn't store context var", node->position());
            }
            bc()->addUInt16(varid.first);
            bc()->addUInt16(varid.second);
        } else { //store non-context var
            switch (type) {
            case VT_INT:
                bc()->addInsn(BC_STOREIVAR);
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_STOREDVAR);
                break;
            case VT_STRING:
                bc()->addInsn(BC_STORESVAR);
                break;
            default:
                throw TranslationError("Couldn't store non-context var", node->position());
            }
            bc()->addUInt16(varid.second);
        }
    }

    VarType load_var(var_t varid, AstNode* node) {
        VarType vtype = m_context->var(varid.first, varid.second)->type();
        if (varid.first != m_context->id()) { //load context var
            switch (vtype) {
            case VT_INT:
                bc()->addInsn(BC_LOADCTXIVAR);
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_LOADCTXDVAR);
                break;
            case VT_STRING:
                bc()->addInsn(BC_LOADCTXSVAR);
                break;
            default:
                throw TranslationError("Couldn't load context var", node->position());
            }
            bc()->addUInt16(varid.first);
            bc()->addUInt16(varid.second);
        } else { //store non-context var
            switch (vtype) {
            case VT_INT:
                bc()->addInsn(BC_LOADIVAR);
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_LOADDVAR);
                break;
            case VT_STRING:
                bc()->addInsn(BC_LOADSVAR);
                break;
            default:
                throw TranslationError("Couldn't load non-context var", node->position());
            }
            bc()->addUInt16(varid.second);
        }
        return vtype;
    }

    VarType int_to_double(VarType leftt, VarType rightt, AstNode* node) {
        LOGGER << "int_to_double: " << typeToName(leftt) << ", " <<
                  typeToName(rightt) << std::endl;

        //TODO: check
        if (leftt == VT_INT && rightt == VT_INT)
            return VT_INT;

        if (leftt == VT_DOUBLE && rightt == VT_DOUBLE)
            return VT_DOUBLE;

        if (leftt == VT_INT && rightt == VT_DOUBLE) {
            bc()->addInsn(BC_I2D);
            return VT_DOUBLE;
        }

        if (leftt == VT_DOUBLE && rightt == VT_INT) {
            bc()->addInsn(BC_SWAP);
            bc()->addInsn(BC_I2D);
            bc()->addInsn(BC_SWAP);
            return VT_DOUBLE;
        }

        throw TranslationError("Wrong subexpression types", node->position());
    }

};

} //namespacnewe



#endif
