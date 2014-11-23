#ifndef __BYTECODEVISITOR_HPP_
#define __BYTECODEVISITOR_HPP_

#include "ast.h"
#include "Context.hpp"
#include "Errors.hpp"
#include "logger.hpp"

namespace mathvm {
    class BytecodeVisitor : public AstVisitor {
    public:
        BytecodeVisitor(Context *context, BytecodeFunction *function = NULL) : context(context), function(function) {

        }

        virtual void visitForNode(ForNode *node) override;

        virtual void visitPrintNode(PrintNode *node) override;

        virtual void visitLoadNode(LoadNode *node) override;

        virtual void visitIfNode(IfNode *node) override;

        virtual void visitIntLiteralNode(IntLiteralNode *node) override;

        virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

        virtual void visitStringLiteralNode(StringLiteralNode *node) override;

        virtual void visitWhileNode(WhileNode *node) override;

        virtual void visitBlockNode(BlockNode *node) override;

        virtual void visitBinaryOpNode(BinaryOpNode *node) override;

        virtual void visitUnaryOpNode(UnaryOpNode *node) override;

        virtual void visitNativeCallNode(NativeCallNode *node) override;

        virtual void visitFunctionNode(FunctionNode *node) override;

        virtual void visitReturnNode(ReturnNode *node) override;

        virtual void visitStoreNode(StoreNode *node) override;

        virtual void visitCallNode(CallNode *node) override;

    private:
        Context *context;
        BytecodeFunction *function;
        VarType topOfStackType;

        Bytecode *bc() const {
            return function->bytecode();
        }

        void cast(VarType castToType, AstNode *node, string const &where) {
            if (castToType == topOfStackType) {
                return;
            }
            switch (topOfStackType) {
                case VT_STRING:
                    switch (castToType) {
                        case VT_INT:
                            bc()->add(BC_S2I);
                            break;
                        default:
                            throw TranslationError(string("Incorrect casting. Trying cast STRING to ") + typeToName(castToType) + ". Where: " + where, node->position());
                    }
                    break;
                case VT_DOUBLE:
                    switch (castToType) {
                        case VT_INT:
                            bc()->add(BC_D2I);
                            break;
                        default:
                            throw TranslationError(string("Incorrect casting. Trying cast DOUBLE to ") + typeToName(castToType) + ". Where: " + where, node->position());
                    }
                    break;
                case VT_INT:
                    switch (castToType) {
                        case VT_DOUBLE:
                            bc()->add(BC_I2D);
                            break;
                        default:
                            throw TranslationError(string("Incorrect casting. Trying cast INT to ") + typeToName(castToType) + ". Where: " + where, node->position());
                    }
                    break;
                default:
                    throw TranslationError("Incorrect storing variable operation", node->position());
            }
            topOfStackType = castToType;
        }

        VarType loadVariable(VariableInContextDescriptor variableDescriptor, AstNode *node) {
            VarType type = context->getVariableByID(variableDescriptor)->type();
            if (variableDescriptor.first != context->getContextID()) {
                switch (type) {
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
                        throw TranslationError("Incorrect loading context-variable type", node->position());
                }
                bc()->addUInt16(variableDescriptor.first);
                bc()->addUInt16(variableDescriptor.second);
            } else {
                switch (type) {
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
                        throw TranslationError("Incorrect loading noncontext-variable type", node->position());
                }
                bc()->addUInt16(variableDescriptor.second);
            }
            return type;
        }

        void storeVariable(VariableInContextDescriptor variableDescriptor, AstNode *node) {
            VarType type = context->getVariableByID(variableDescriptor)->type();
            if (variableDescriptor.first != context->getContextID()) {
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
                        throw TranslationError("Incorrect storing context-variable type", node->position());
                }
                bc()->addUInt16(variableDescriptor.first);
                bc()->addUInt16(variableDescriptor.second);
            } else {
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
                        throw TranslationError("Incorrect storing noncontext-variable type", node->position());
                }
                bc()->addUInt16(variableDescriptor.second);
            }
        }

        VarType equateTypes(VarType leftType, VarType rightType, AstNode *node) {
            if (leftType == rightType) {
                return leftType;
            }
            if (leftType == VT_INT && rightType == VT_DOUBLE) {
                bc()->addInsn(BC_SWAP);
                bc()->addInsn(BC_I2D);
                bc()->addInsn(BC_SWAP);
                return VT_DOUBLE;
            }

            if (leftType == VT_DOUBLE && rightType == VT_INT) {
                bc()->addInsn(BC_I2D);
                return VT_DOUBLE;
            }

            throw TranslationError("Wrong subexpression types (in equateTypes): ", node->position());
        }

        bool isNumericType(VarType type) const {
            return type == VT_DOUBLE || type == VT_INT;
        }
    };
}

#endif