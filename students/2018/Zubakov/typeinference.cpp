//
// Created by aleks on 24.11.18.
//

#include <visitors.h>
#include "typeinference.h"

namespace mathvm {
    TypeInferenceVisitor::TypeInferenceVisitor(vector<std::unique_ptr<Info>> &ptrs) :
            ptrs(ptrs) {};

    Info *TypeInferenceVisitor::createInfo(const mathvm::VarType &type) {
        Info *info = new Info(type);
        ptrs.emplace_back(info);
        return info;
    }

    void TypeInferenceVisitor::visitLoadNode(LoadNode *node) {
        node->setInfo(new Info(node->var()->type()));
    }

    void TypeInferenceVisitor::visitCallNode(CallNode *node) {
        AstFunction *pFunction = currentScope->lookupFunction(node->name());
        VarType type = pFunction->returnType();
        node->setInfo(createInfo(type));
        AstBaseVisitor::visitCallNode(node);
    }

    void TypeInferenceVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        node->setInfo(createInfo(VarType::VT_DOUBLE));
    }

    void TypeInferenceVisitor::visitStoreNode(StoreNode *node) {
        node->setInfo(createInfo(node->var()->type()));
        node->value()->visit(this);
    }

    void TypeInferenceVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        node->setInfo(createInfo(VarType::VT_STRING));
    }

    void TypeInferenceVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        node->setInfo(createInfo(VarType::VT_INT));
    }

    void TypeInferenceVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        node->operand()->visit(this);
        node->setInfo(node->operand()->info());
    }

    void TypeInferenceVisitor::visitNativeCallNode(NativeCallNode *node) {
        AstBaseVisitor::visitNativeCallNode(node); // TODO: ???
    }

    void TypeInferenceVisitor::visitReturnNode(ReturnNode *node) {
        node->returnExpr()->visit(this);
        node->setInfo(node->returnExpr()->info());
    }

    void TypeInferenceVisitor::visitFunctionNode(FunctionNode *node) {
        node->setInfo(createInfo(node->returnType()));
        node->body()->visit(this);
        AstBaseVisitor::visitFunctionNode(node);
    }

    void TypeInferenceVisitor::visitBlockNode(BlockNode *node) {
        Scope *oldScope = currentScope;
        currentScope = node->scope();

//        Scope::VarIterator varIt(currentScope);
//        while (varIt.hasNext()) {
//            AstVar *curVar = varIt.next();
//
//        }

        Scope::FunctionIterator funIt(currentScope);
        while (funIt.hasNext()) {
            funIt.next()->node()->visit(this);
        }

        for (uint32_t i = 0; i < node->nodes(); ++i) {

            AstNode *curNode = node->nodeAt(i);
            if (curNode->isReturnNode() && curNode->asReturnNode()->returnExpr() == nullptr)
                continue;

            curNode->visit(this);
        }


        currentScope = oldScope;
    };

#define get_info(node) (reinterpret_cast<Info*>(node->info()))

    void TypeInferenceVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        node->left()->visit(this);
        node->right()->visit(this);

        VarType leftType = get_info(node->left())->getType();
        VarType rightType = get_info(node->right())->getType();

        VarType result = infer(leftType, rightType);
        assert(result != VT_INVALID);
        node->setInfo(createInfo(result));
    }

#undef get_info

    VarType TypeInferenceVisitor::infer(VarType &left, VarType &right) {
        if (left == right) {
            return left;
        }


        if (left == VT_INVALID || right == VT_INVALID
            || left == VT_VOID || right == VT_VOID) {
            return VT_INVALID;
        }

        if (left == VT_INT) {
            if (right == VT_DOUBLE) {
                return VT_DOUBLE;
            }

            if (right == VT_STRING) {
                return VT_INT;
            }
        }

        if (left == VT_DOUBLE) {
            if (right == VT_INT) {
                return VT_DOUBLE;
            }

            if (right == VT_STRING) {
                return VT_DOUBLE;
            }
        }

        if (left == VT_STRING) {
            if (right == VT_DOUBLE) {
                return VT_DOUBLE;
            }

            if (right == VT_INT) {
                return VT_INT;
            }
        }
        return VT_INVALID; // right is VT_VOID or VT_STRING


        return VT_INVALID;
    }


}