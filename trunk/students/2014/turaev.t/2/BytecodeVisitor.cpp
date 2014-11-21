#include "ast.h"
#include "BytecodeVisitor.hpp"

namespace mathvm {

    void BytecodeVisitor::visitForNode(ForNode *node) {
        LOG << "visitForNode" << endl;

        VariableInContextDescriptor variableDescriptor = context->getVariableDescriptor(node->var()->name());

        BinaryOpNode *innerExpression = (BinaryOpNode *) node->inExpr();

        if (innerExpression->kind() != tRANGE) {
            throw TranslationError(string("Incorrect binary operation in for-expression. Exptected: RANGE, got: ")
                    + tokenStr(innerExpression->kind()), node->position());
        }
        if (node->var()->type() != VT_INT) {
            throw TranslationError(string("Incorrect type of for-variable. Exptected: INT, got: ")
                    + typeToName(node->var()->type()), node->position());
        }

        innerExpression->left()->visit(this);

        bc()->addInsn(BC_ILOADM1);
        bc()->addInsn(BC_IADD);
        storeVariable(variableDescriptor, innerExpression);

        Label begin(bc());
        Label end(bc());
        {
            bc()->bind(begin);
            loadVariable(variableDescriptor, innerExpression);
            bc()->addInsn(BC_ILOAD1);
            bc()->addInsn(BC_IADD);
            storeVariable(variableDescriptor, innerExpression);

            //condition
            innerExpression->right()->visit(this);
            loadVariable(variableDescriptor, innerExpression);
            bc()->addInsn(BC_SWAP);

            // goto end if greater
            bc()->addBranch(BC_IFICMPG, end);

            node->body()->visit(this);

            bc()->addBranch(BC_JA, begin);
        }
        bc()->bind(end);
    }

    void BytecodeVisitor::visitPrintNode(PrintNode *node) {
        LOG << "visitPrintNode" << endl;
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            switch (topOfStackType) {
                case VT_INT:
                    bc()->addInsn(BC_IPRINT);
                    break;
                case VT_DOUBLE:
                    bc()->addInsn(BC_DPRINT);
                    break;
                case VT_STRING:
                    bc()->addInsn(BC_SPRINT);
                    break;
                default:
                    throw TranslationError("Incorrect printing variable type", node->position());
            }
        }
    }

    void BytecodeVisitor::visitLoadNode(LoadNode *node) {
        LOG << "visitLoadNode" << endl;
        VariableInContextDescriptor variableDescriptor = context->getVariableDescriptor(node->var()->name());
        topOfStackType = loadVariable(variableDescriptor, node);
    }

    void BytecodeVisitor::visitIfNode(IfNode *node) {
        LOG << "visitIfNode" << endl;

        Label _else(bc());
        Label end(bc());

        node->ifExpr()->visit(this);

        bc()->addInsn(BC_ILOAD0);
        bc()->addBranch(BC_IFICMPE, _else);

        node->thenBlock()->visit(this);

        bc()->addBranch(BC_JA, end);

        bc()->bind(_else);
        if (node->elseBlock()) {
            node->elseBlock()->visit(this);
        }

        bc()->bind(end);

    }

    void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        LOG << "visitIntLiteralNode" << endl;

        bc()->addInsn(BC_ILOAD);
        bc()->addInt64(node->literal());
        topOfStackType = VT_INT;
    }

    void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        LOG << "visitDoubleLiteralNode" << endl;

        bc()->addInsn(BC_DLOAD);
        bc()->addDouble(node->literal());
        topOfStackType = VT_DOUBLE;
    }

    void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        LOG << "visitStringLiteralNode" << endl;

        uint16_t id = context->introduceStringConst(node->literal());
        bc()->addInsn(BC_SLOAD);
        bc()->addUInt16(id);
        topOfStackType = VT_STRING;
    }

    void BytecodeVisitor::visitWhileNode(WhileNode *node) {
        LOG << "visitWhileNode" << endl;

        Label start = bc()->currentLabel();
        Label end(bc());

        node->whileExpr()->visit(this);

        bc()->addInsn(BC_ILOAD0);
        bc()->addBranch(BC_IFICMPE, end);

        node->loopBlock()->visit(this);

        bc()->addBranch(BC_JA, start);
        bc()->bind(end);
    }

    void BytecodeVisitor::visitBlockNode(BlockNode *node) {
        LOG << "visitBlockNode" << endl;

        Scope::VarIterator variableIterator(node->scope());
        while (variableIterator.hasNext()) {
            AstVar *var = variableIterator.next();
            context->introduceVariable(var->type(), var->name());
        }
        Scope::FunctionIterator functionIterator(node->scope());
        while (functionIterator.hasNext()) {
            context->introduceFunction(new BytecodeFunction(functionIterator.next()));
        }

        node->visitChildren(this);

        functionIterator = Scope::FunctionIterator(node->scope());
        while (functionIterator.hasNext()) {
            visitFunctionNode(functionIterator.next()->node());
        }
    }

    void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
        LOG << "visitBinaryOpNode" << endl;

        node->left()->visit(this);
        VarType leftType = topOfStackType;
        node->right()->visit(this);
        VarType rightType = topOfStackType;

        switch (node->kind()) {
            case tOR:
            case tAND:
            case tAOR:
            case tAAND:
            case tAXOR:
            case tMOD: {
                switch (node->kind()) {
                    case tAND:
                    case tAAND:
                        bc()->addInsn(BC_IAAND);
                        break;
                    case tOR:
                    case tAOR:
                        bc()->addInsn(BC_IAOR);
                        break;
                    case tAXOR:
                        bc()->addInsn(BC_IAXOR);
                        break;
                    case tMOD:
                        bc()->addInsn(BC_IMOD);
                        break;
                    default:;
                }
                topOfStackType = VT_INT;
                break;
            }

            case tEQ:
            case tNEQ:
            case tGT:
            case tGE:
            case tLT:
            case tLE: {
                VarType varType = equateTypes(leftType, rightType, node);
                if (!isNumericType(varType)) {
                    throw TranslationError(string("Incorrect type for COMPARE inside binary-node: ") + typeToName(varType), node->position());
                }
                if (varType == VT_DOUBLE) {
                    bc()->addInsn(BC_DCMP);
                    bc()->addInsn(BC_ILOAD0);
                }
                Label _else(bc());
                Label end(bc());
                switch (node->kind()) {
                    case tEQ:
                        bc()->addBranch(BC_IFICMPE, _else);
                        break;
                    case tNEQ:
                        bc()->addBranch(BC_IFICMPNE, _else);
                        break;
                    case tGT:
                        bc()->addBranch(BC_IFICMPG, _else);
                        break;
                    case tGE:
                        bc()->addBranch(BC_IFICMPGE, _else);
                        break;
                    case tLT:
                        bc()->addBranch(BC_IFICMPL, _else);
                        break;
                    case tLE:
                        bc()->addBranch(BC_IFICMPLE, _else);
                        break;
                    default:;
                        break;
                }
                bc()->addInsn(BC_ILOAD0);
                bc()->addBranch(BC_JA, end);
                bc()->bind(_else);
                bc()->addInsn(BC_ILOAD1);
                bc()->bind(end);
                topOfStackType = VT_INT;
                break;
            }
            case tINCRSET:
            case tDECRSET:
                break;

            case tADD:
            case tSUB:
            case tMUL:
            case tDIV: {
                VarType varType = equateTypes(leftType, rightType, node);
                if (!isNumericType(varType)) {
                    throw TranslationError(string("Incorrect type for ARITHMETIC inside binary-node: ") + typeToName(varType), node->position());
                }
                switch (node->kind()) {
                    case tADD:
                        bc()->addInsn(varType == VT_DOUBLE ? BC_DADD : BC_IADD);
                        break;
                    case tSUB:
                        bc()->addInsn(BC_SWAP);
                        bc()->addInsn(varType == VT_DOUBLE ? BC_DSUB : BC_ISUB);
                        break;
                    case tMUL:
                        bc()->addInsn(varType == VT_DOUBLE ? BC_DMUL : BC_IMUL);
                        break;
                    case tDIV:
                        bc()->addInsn(BC_SWAP);
                        bc()->addInsn(varType == VT_DOUBLE ? BC_DDIV : BC_IDIV);
                        break;
                    default:
                        assert(false);
                }
                topOfStackType = varType;
                break;
            }
            default:
                throw TranslationError(string("Unknown binary operation token: ") + tokenStr(node->kind()), node->position());
        }
    }

    void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        LOG << "visitUnaryOpNode" << endl;

        node->operand()->visit(this);
        switch (node->kind()) {
            case tSUB: {
                if (!isNumericType(topOfStackType)) {
                    throw TranslationError(string("Incorrect type for SUB inside unary-node: ") + typeToName(topOfStackType), node->position());
                }
                bc()->addInsn(topOfStackType == VT_DOUBLE ? BC_DNEG : BC_INEG);
                break;
            }
            case tNOT: {
                if (topOfStackType != VT_INT) {
                    throw TranslationError(string("Incorrect type for NOT inside unary-node: ") + typeToName(topOfStackType), node->position());
                }
                bc()->addInsn(BC_INEG);
                break;
            }
            default:
                throw TranslationError(string("Unknown unary operation token: ") + tokenStr(node->kind()), node->position());
        }
    }

    void BytecodeVisitor::visitNativeCallNode(NativeCallNode *node) {
        // TODO
        LOG << "visitNativeCallNode TODO" << std::endl;
        throw TranslationError("NativeCallNode not implemented", node->position());
    }

    void BytecodeVisitor::visitFunctionNode(FunctionNode *node) {
        LOG << "visitFunctionNode" << endl;
        if (function == NULL) {
            function = context->getFunction(node->name());
            visitBlockNode(node->body());
            context->getCode()->setBytecode(context->getFunction(node->name())->bytecode());
            return;
        }
        Context *child = context->addChildContext();
        BytecodeVisitor visitor(child, context->getFunction(node->name()));
        for (uint32_t i = 0; i < node->parametersNumber(); i++) {
            uint16_t id = child->introduceVariable(node->parameterType(i), node->parameterName(i));
            switch (node->parameterType(i)) {
                case VT_DOUBLE:
                    visitor.bc()->addInsn(BC_STOREDVAR);
                    break;
                case VT_INT:
                    visitor.bc()->addInsn(BC_STOREIVAR);
                    break;
                case VT_STRING:
                    visitor.bc()->addInsn(BC_STORESVAR);
                    break;
                default:
                    throw TranslationError("Incorrect storing variable type", node->position());
            }
            visitor.bc()->addUInt16(id);
        }
        topOfStackType = node->returnType();
        visitor.visitBlockNode(node->body());
    }

    void BytecodeVisitor::visitStoreNode(StoreNode *node) {
        LOG << "visitStoreNode" << endl;

        VariableInContextDescriptor variableDescriptor = context->getVariableDescriptor(node->var()->name());

        if (node->op() == tINCRSET || node->op() == tDECRSET) {
            loadVariable(variableDescriptor, node);
        }
        node->value()->visit(this);

        VarType varType = context->getVariableByID(variableDescriptor)->type();
        cast(varType, node, "storing node");

        switch (node->op()) {
            case tINCRSET:
                bc()->addInsn(varType == VT_DOUBLE ? BC_DADD : BC_IADD);
                break;
            case tDECRSET:
                bc()->addInsn(BC_SWAP);
                bc()->addInsn(varType == VT_DOUBLE ? BC_DSUB : BC_ISUB);
                break;
            case tASSIGN:
                break;
            default:
                throw TranslationError("Incorrect storing variable operation", node->position());
        }
        storeVariable(variableDescriptor, node);
    }

    void BytecodeVisitor::visitCallNode(CallNode *node) {
        LOG << "visitCallNode" << endl;

        BytecodeFunction *calledFunction = context->getFunction(node->name());
        if (node->parametersNumber() != calledFunction->parametersNumber()) {
            throw TranslationError("Incorrect number of parameters at calling function " + calledFunction->name(), node->position());
        }

        for (int32_t i = node->parametersNumber() - 1; i >= 0; --i) {
            uint32_t j = (uint32_t) i;
            node->parameterAt(j)->visit(this);
            cast(calledFunction->parameterType(j), node, "casting call-node parameters");
        }

        uint16_t functionID = calledFunction->id();
        bc()->addInsn(BC_CALL);
        bc()->addUInt16(functionID);
        topOfStackType = calledFunction->returnType();
    }

    void BytecodeVisitor::visitReturnNode(ReturnNode *node) {
        LOG << "visitReturnNode" << endl;

        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
            cast(function->returnType(), node, "casting return-node");
        }
        bc()->addInsn(BC_RETURN);
    }
}