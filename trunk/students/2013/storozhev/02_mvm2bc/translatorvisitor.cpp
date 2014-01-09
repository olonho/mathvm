#include "translatorvisitor.hpp"

namespace mathvm {

TranslatorVisitor::TranslatorVisitor(Context *context, BytecodeFunction *function) :
    m_context(context)
  , m_function(function)
{   }


//visitor functions

void TranslatorVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    LOGGER << "visitBinaryOpNode" << std::endl;

    node->left()->visit(this);
    VarType leftt = m_last_type;
    node->right()->visit(this);
    VarType rightt = m_last_type;

 //   bc()->add(BC_SWAP);

    switch (node->kind()) {
    case tOR:
    case tAND:
    case tAOR:
    case tAAND:
    case tAXOR:
    case tMOD: {
        //TODO: cast 2 args to int

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
        default:
            ;
        }
        m_last_type = VT_INT;
        break;
    }
    case tEQ:
    case tNEQ:
    case tGT:
    case tGE:
    case tLT:
    case tLE: {
        VarType vt = int_to_double(leftt, rightt, node);
//        bc()->addInsn(BC_SWAP);
        if (vt == VT_DOUBLE) {
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
        default:
            ; //error
        }
        bc()->addInsn(BC_ILOAD0);
        bc()->addBranch(BC_JA, end);
        bc()->bind(_else);
        bc()->addInsn(BC_ILOAD1);
        bc()->bind(end);
        m_last_type = VT_INT;
        break;
    }
    case tADD:
    case tSUB:
    case tMUL:
    case tDIV: {
        VarType type = int_to_double(leftt, rightt, node);
        switch (node->kind()) {
        case tADD:
            bc()->addInsn(type == VT_DOUBLE ? BC_DADD : BC_IADD);
            break;
        case tSUB:
            bc()->addInsn(BC_SWAP);
            bc()->addInsn(type == VT_DOUBLE ? BC_DSUB : BC_ISUB);
            break;
        case tMUL:
            bc()->addInsn(type == VT_DOUBLE ? BC_DMUL : BC_IMUL);
            break;
        case tDIV:
            bc()->addInsn(BC_SWAP);
            bc()->addInsn(type == VT_DOUBLE ? BC_DDIV : BC_IDIV);
            break;
        default:
            throw TranslationError("Wrong operaion kind", node->position());
        }
        m_last_type = type;
        break;
    }
    case tINCRSET:
    case tDECRSET: {
        break;
    }
    default:
        throw TranslationError("Unknown operator", node->position());
    }

}

void TranslatorVisitor::visitBlockNode(BlockNode *node) {
    LOGGER << "visitBlockNode" << std::endl;

    Scope::VarIterator varit(node->scope());
    while (varit.hasNext()) {
        AstVar* v = varit.next();
        m_context->introduceVar(v->name(), v->type());
    }

    Scope::FunctionIterator fit(node->scope());
    while (fit.hasNext()) {
        m_context->addFunction(new BytecodeFunction(fit.next()));
    }

    node->visitChildren(this);

    Scope::FunctionIterator it(node->scope());
    while (it.hasNext()) {
        visitFunctionNode(it.next()->node());
    }
}

void TranslatorVisitor::visitCallNode(CallNode *node) {
    LOGGER << "visitCallNode" << std::endl;

    //TODO: check parameters count
    for (size_t i = node->parametersNumber(); i > 0; --i) {
        node->parameterAt(i-1)->visit(this);
    }
    uint16_t fid = m_context->functionId(node->name());
    if (fid < MAXID) {
        bc()->addInsn(BC_CALL);
        bc()->addUInt16(fid);
        if (m_context->function(node->name())->returnType() != VT_VOID) {
            m_last_type = m_context->function(node->name())->returnType();
        }
    } else
        throw TranslationError("Couldn't find function", node->position());

}

void TranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    LOGGER << "visitDoubleLiteralNode" << std::endl;

    bc()->addInsn(BC_DLOAD);
    bc()->addDouble(node->literal());
    m_last_type = VT_DOUBLE;
}

void TranslatorVisitor::visitForNode(ForNode *node) {
    LOGGER << "visitForNode" << std::endl;

    var_t vart = m_context->varId(node->var()->name());
    if (!checkVart(vart)) {
        throw TranslationError("Unresolved variable in loop", node->position());
    }

    BinaryOpNode* innerExpr = (BinaryOpNode*)node->inExpr();
    if (innerExpr->kind() != tRANGE) {
        throw TranslationError("Error in range", node->position());
    }

    VarType vtype = m_context->var(vart.first, vart.second)->type();

    innerExpr->left()->visit(this);
    //TODO: add cast to int
    bc()->addInsn(BC_ILOADM1);
    bc()->addInsn(BC_IADD);
    //TODO: add cast
    store_var(vart, vtype, innerExpr->left());

    Label begin(bc());
    Label end(bc());

    bc()->bind(begin);

    //new value for loop var
    load_var(vart, innerExpr);
    //TODO: cast to int
    bc()->addInsn(BC_ILOAD1);
    bc()->addInsn(BC_IADD);
    //TODO: cast
    store_var(vart, vtype, innerExpr->left());

    //condition
    innerExpr->right()->visit(this);
    //TODO: cast to int
    load_var(vart, innerExpr);
    //TODO: cast to int
    bc()->addInsn(BC_SWAP);
    bc()->addBranch(BC_IFICMPG, end);

    node->body()->visit(this);
    bc()->addBranch(BC_JA, begin);

    bc()->bind(end);
}

void TranslatorVisitor::visitFunctionNode(FunctionNode *node) {
    LOGGER << "visitFunctionNode" << std::endl;

    //XXX: dirty fix
//    if (!m_function) {
//        m_function = m_context->function(node->name());
//        visitBlockNode(node->body());
//        m_context->codeImpl()->setBytecode(m_context->function(node->name())->bytecode());
//    } else {
        Context* ctx = m_context->addChildContext();
        TranslatorVisitor visitor(ctx, m_context->function(node->name()));
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            uint16_t id = ctx->introduceVar(node->parameterName(i),
                                            node->parameterType(i));

            switch (node->parameterType(i)) {
            case VT_INT:
                visitor.bc()->addInsn(BC_STOREIVAR);
                break;
            case VT_DOUBLE:
                visitor.bc()->addInsn(BC_STOREDVAR);
                break;
            case VT_STRING:
                visitor.bc()->addInsn(BC_STORESVAR);
                break;
            default:
                throw TranslationError("Couldn't store var of unknown type", node->position());
            }
            visitor.bc()->addUInt16(id);
        }

        m_last_type = node->returnType();
        visitor.visitBlockNode(node->body());
//    }
}

void TranslatorVisitor::visitIfNode(IfNode *node) {
    LOGGER << "visitIfNode" << std::endl;

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

void TranslatorVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    LOGGER << "visitIntLiteralNode" << std::endl;

    bc()->addInsn(BC_ILOAD);
    bc()->addInt64(node->literal());
    m_last_type = VT_INT;
}

void TranslatorVisitor::visitLoadNode(LoadNode *node) {
    LOGGER << "visitLoadNode" << std::endl;

    var_t vart = m_context->varId(node->var()->name());
    if (checkVart(vart)) {
        m_last_type = load_var(vart, node);
    } else {
        throw TranslationError("Unresolved variable", node->position());
    }

}

void TranslatorVisitor::visitNativeCallNode(NativeCallNode *node) {
    LOGGER << "visitNativeCallNode" << std::endl;

    throw TranslationError("Native calls are not implemented", node->position());
}

void TranslatorVisitor::visitPrintNode(PrintNode *node) {
    LOGGER << "visitPrintNode" << std::endl;

    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        switch (m_last_type) {
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
            throw TranslationError("Couldn't print expression of unknown type", node->position());
        }
    }
}

void TranslatorVisitor::visitReturnNode(ReturnNode *node) {
    LOGGER << "visitReturnNode" << std::endl;

    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
        //add cast
        switch (m_function->returnType()) {
        case VT_INT:
            switch (m_last_type) {
            case VT_INT:
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_D2I);
                break;
            default:
                ;
            }

            break;
        case VT_DOUBLE:
            break;
        default:
            ;
        }
    }

    bc()->addInsn(BC_RETURN);
}

void TranslatorVisitor::visitStoreNode(StoreNode *node) {
    LOGGER << "visitStoreNode" << std::endl;

    var_t vart = m_context->varId(node->var()->name());
    if (!checkVart(vart)) {
        throw TranslationError("Unresolved variable", node->position());
    }

    if (node->op() == tINCRSET || node->op() == tDECRSET) {
        load_var(vart, node);
    }

    node->value()->visit(this);
    VarType vt = m_context->var(vart.first, vart.second)->type();
    checkType(node, vt);

    switch(node->op()) {
    case tASSIGN:
        break;
    case tINCRSET:
        bc()->addInsn(vt == VT_DOUBLE ? BC_DADD : BC_IADD);
        break;
    case tDECRSET:
        bc()->addInsn(BC_SWAP);
        bc()->addInsn(vt == VT_DOUBLE ? BC_DSUB : BC_ISUB);
        break;
    default:
        throw TranslationError("unknown operator", node->position());
    }

    //store var
    store_var(vart, node->var()->type(), node);
}

void TranslatorVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    LOGGER << "visitStringLiteralNode" << std::endl;

    uint16_t id = m_context->introduceStringConst(node->literal());
    bc()->addInsn(BC_SLOAD);
    bc()->addUInt16(id);
    m_last_type = VT_STRING;
}

void TranslatorVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    LOGGER << "visitUnaryOpNode" << std::endl;

    node->operand()->visit(this);
    VarType type = m_last_type;
    switch (node->kind()) {
    case tNOT: {
        switch (m_last_type) {
        case VT_INT:
            bc()->addInsn(BC_INEG);
            break;
        case VT_DOUBLE:
            bc()->addInsn(BC_DNEG);
            break;
        default:
            ;
        }

        break;
    }
    case tSUB: {
        //TODO: assert
        bc()->addInsn(type == VT_DOUBLE ? BC_DNEG : BC_INEG);
        break;
    }
    default:
        throw TranslationError("Unknown operator", node->position());
    }

}

void TranslatorVisitor::visitWhileNode(WhileNode *node) {
    LOGGER << "visitWhileNode" << std::endl;

    Label start = bc()->currentLabel();
    Label end(bc());

    node->whileExpr()->visit(this);

    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, end);

    node->loopBlock()->visit(this);

    bc()->addBranch(BC_JA, start);
    bc()->bind(end);
}

void TranslatorVisitor::checkType(AstNode *node, VarType expected) {
    if (expected != m_last_type)
        throw TranslationError(std::string("Type error: expected ") + typeToName(expected) +
                               ", got " + typeToName(m_last_type),
                               node->position());
}


void TranslatorVisitor::run(AstFunction *node) {
    LOGGER << "Running translator" << std::endl;

//    Scope::FunctionIterator fit(node->scope());
    m_function = new BytecodeFunction(node);
    visitBlockNode(node->node()->body());
}

} //namespace
