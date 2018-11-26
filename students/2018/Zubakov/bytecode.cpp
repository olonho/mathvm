//
// Created by aleks on 25.11.18.
//

#include "bytecode.h"
#include "typeinference.h"

namespace mathvm {
#define get_info(node) (reinterpret_cast<Info*>(node->info()))

    BytecodeVisitor::BytecodeVisitor(Code *code) : _code(code) {}

    void BytecodeVisitor::visitBlockNode(BlockNode *node) {
        uint32_t nodeNum = node->nodes();
        for (uint32_t i = 0; i < nodeNum; ++i) {
            AstNode *pNode = node->nodeAt(i);
            pNode->visit(this);
        }
    }

    void BytecodeVisitor::visitFunctionNode(mathvm::FunctionNode *node) {
        AstFunction *tmp = _cur_function;


        cur_scope = node->body()->scope()->parent();
        _cur_function = new AstFunction(node, cur_scope);

        BytecodeFunction *function = new BytecodeFunction(_cur_function);
        _code->addFunction(function);

        Bytecode *bytecode1 = bytecode;
        bytecode = function->bytecode();

        node->body()->visit(this);

        bytecode = bytecode1;
        _cur_function = tmp;
    }

    void BytecodeVisitor::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
        int64_t val = node->literal();
        switch (val) {
            case 0:
                bytecode->add(BC_ILOAD0);
                break;
            case 1:
                bytecode->add(BC_ILOAD1);
                break;
            case -1:
                bytecode->add(BC_ILOADM1);
                break;
            default:
                bytecode->add(BC_ILOAD);
                bytecode->addInt64(val);
                break;
        }
    }


    void BytecodeVisitor::visitPrintNode(mathvm::PrintNode *node) {
        uint32_t opNum = node->operands();
        for (uint32_t i = 0; i < opNum; ++i) {
            AstNode *pNode = node->operandAt(i);
            pNode->visit(this);
            VarType type = get_info(pNode)->getType();
            switch (type) {
                case VT_INVALID:
                case VT_VOID:
                    break;
                case VT_INT:
                    bytecode->add(BC_IPRINT);
                    break;
                case VT_DOUBLE:
                    bytecode->add(BC_DPRINT);
                    break;
                case VT_STRING:
                    bytecode->add(BC_SPRINT);
                    break;
            }
        }
    }

    void BytecodeVisitor::visitCallNode(CallNode *node) {

        AstBaseVisitor::visitCallNode(node);
    }

    void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        double val = node->literal();
        if (val == 0) {
            bytecode->add(BC_DLOAD0);
        } else if (val == 1) {
            bytecode->add(BC_DLOAD1);
        } else if (val == -1) {
            bytecode->add(BC_DLOADM1);
        } else {
            bytecode->add(BC_DLOAD);
            bytecode->addDouble(val);
        }
    }

    void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        const string &literal = node->literal();
        if (literal.empty()) {
            bytecode->add(BC_SLOAD0);
        } else {
            bytecode->add(BC_SLOAD);
            uint16_t i = _code->makeStringConstant(literal);
            bytecode->addUInt16(i);
        }
    }

    void BytecodeVisitor::cast(VarType left, VarType right) {
        if (left == right) {
            return;
        }

        if (left == VT_DOUBLE && right == VT_INT) {
            bytecode->add(BC_D2I);
            return;
        }
        if (left == VT_INT && right == VT_DOUBLE) {
            bytecode->add(BC_I2D);
            return;
        }
        if (left == VT_STRING && right == VT_INT) {
            bytecode->add(BC_S2I);
            return;
        }
    }

    void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        node->left()->visit(this);
        node->right()->visit(this);

        VarType resultType = get_info(node)->getType();

        VarType left = get_info(node->left())->getType();
        cast(left, resultType);
        VarType right = get_info(node->right())->getType();
        cast(right, resultType);

        if (resultType == VT_DOUBLE) {
            generateDoubleArithmetic(node->kind());
        } else if (resultType == VT_INT) {
            generateIntArithmetic(node->kind());
        }
    }

    void BytecodeVisitor::generateIntArithmetic(mathvm::TokenKind tokenKind){
        switch (tokenKind) {
            case tADD:
                bytecode->add(BC_IADD);
                break;
            case tSUB:
                bytecode->add(BC_ISUB);
                break;
            case tDIV:
                bytecode->add(BC_IDIV);
                break;
            case tMUL:
                bytecode->add(BC_IMUL);
                break;
            case tMOD:
                bytecode->add(BC_IMOD);
                break;
            default:
                return;
        }

    };

    void BytecodeVisitor::generateDoubleArithmetic(mathvm::TokenKind tokenKind){
        switch (tokenKind) {
            case tADD:
                bytecode->add(BC_DADD);
            case tSUB:
                bytecode->add(BC_DSUB);
            case tDIV:
                bytecode->add(BC_DDIV);
            case tMUL:
                bytecode->add(BC_DMUL);
            default:
                return;
        }
    }

    void BytecodeVisitor::visitStoreNode(StoreNode *node) {
        node->position();
    }

    void BytecodeVisitor::visitLoadNode(LoadNode *node) {
        AstBaseVisitor::visitLoadNode(node);
    };

#undef get_info
}