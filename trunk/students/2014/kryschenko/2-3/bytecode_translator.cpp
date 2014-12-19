#include <dlfcn.h>
#include "bytecode_translator.hpp"


using namespace mathvm;

Status* BytecodeTranslator::translate(const string &program, Code **code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError())
        return status;

    BytecodeVisitor visitor;

    status = visitor.translateBytecode(*code, parser.top());

    return status;
}

Status* BytecodeVisitor::translateBytecode(Code *code, AstFunction *top) {
    code_ = code;
    BytecodeFunction* bf = new BytecodeFunction(top);
    code_->addFunction(bf);
    try {
        translateFunction(top);
        bf->bytecode()->addInsn(BC_STOP);
    } catch(std::runtime_error* e) {
        return Status::Error(e->what());
    } catch(...) {
        return Status::Error("Translate error");
    }

    return Status::Ok();

}

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    switch (node->kind()) {
        case tADD:
        case tSUB:
        case tDIV:
        case tMUL:
        case tMOD:
            addArithmeticalOp(node);
            break;
        case tAND:
        case tOR:
            this->addLogicalOp(node);
            break;
        case tAAND:
        case tAOR:
        case tAXOR:
            this->addBitwiseOp(node);
            break;
        case tEQ:
        case tNEQ:
        case tGE:
        case tLE:
        case tGT:
        case tLT:
            this->addCompareOp(node);
            break;
        default:
            throw std::runtime_error("Invalide binary operation");
    }
}

void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    switch (node->kind()) {
        case tNOT:
            this->addNotOp(node);
            break;
        case tSUB:
            this->addUnarySubOp(node);
            break;
        default:
            throw new std::runtime_error("Invalide unary operation");
    }
}


void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    bc()->addInsn(BC_ILOAD);
    bc()->addInt64(node->literal());
    setTosType(VT_INT);
}

void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    bc()->addInsn(BC_DLOAD);
    bc()->addDouble(node->literal());
    setTosType(VT_DOUBLE);
}

void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    bc()->addInsn(BC_SLOAD);
    bc()->addUInt16(code_->makeStringConstant(node->literal()));
    setTosType(VT_STRING);
}

void BytecodeVisitor::visitLoadNode(LoadNode *node) {
    loadVar(node->var());
}

void BytecodeVisitor::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);
    VarType vt = node->var()->type();
    castTos(vt);
    if (node->op() == tINCRSET) {
        loadVar(node->var());
        Instruction incr_ins = (vt == VT_INT) ? BC_IADD : BC_DADD;
        bc()->addInsn(incr_ins);
    } else if (node->op() == tDECRSET) {
        loadVar(node->var());
        Instruction decr_ins = (vt == VT_INT) ? BC_ISUB : BC_DSUB;
        bc()->addInsn(decr_ins);
    }

    storeVar(node->var());

}

void BytecodeVisitor::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    castTos(VT_INT);

    Label l_else = Label(bc());
    Label l_end = Label(bc());

    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, l_else);
    node->thenBlock()->visit(this);
    bc()->addBranch(BC_JA, l_end);

    bc()->bind(l_else);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }
    bc()->bind(l_end);

    setTosType(VT_VOID);
}



void BytecodeVisitor::visitForNode(ForNode *node) {
    BinaryOpNode* in_expr = node->inExpr()->asBinaryOpNode();
    if (!in_expr) {
        throw new std::runtime_error("Error in for expression");
    }
    in_expr->left()->visit(this);
    castTos(VT_INT);
    if (node->var()->type() != VT_INT) {
        throw new std::runtime_error("iter var in for expression must be int");
    }
    storeVar(node->var());

    Label l_start = Label(bc());
    Label l_end = Label(bc());

    bc()->bind(l_start);

    in_expr->right()->visit(this);
    castTos(VT_INT);
    loadVar(node->var());
    bc()->addBranch(BC_IFICMPG, l_end);

    node->body()->visit(this);

    loadVar(node->var());
    bc()->addInsn(BC_ILOAD1);
    bc()->addInsn(BC_IADD);
    storeVar(node->var());
    bc()->addBranch(BC_JA, l_start);
    bc()->bind(l_end);

    setTosType(VT_VOID);
}

void BytecodeVisitor::visitWhileNode(WhileNode *node) {
    Label l_start = Label(bc());
    Label l_end = Label(bc());

    bc()->bind(l_start);
    node->whileExpr()->visit(this);
    castTos(VT_INT);
    bc()->addInsn(BC_ILOAD0);
    bc()->addBranch(BC_IFICMPE, l_end);
    node->loopBlock()->visit(this);
    bc()->addBranch(BC_JA, l_start);
    bc()->bind(l_end);

    setTosType(VT_VOID);
}

void BytecodeVisitor::visitBlockNode(BlockNode *node) {

    Scope::VarIterator it_var(node->scope());
    while (it_var.hasNext()) {
        AstVar* var = it_var.next();
        context_->addVar(var);
    }

    Scope::FunctionIterator it_f(node->scope());
    while (it_f.hasNext()) {
        AstFunction* af = it_f.next();

        BytecodeFunction* bf = (BytecodeFunction*) code_->functionByName(af->name());
        if (!bf) {
            bf = new BytecodeFunction(af);
            code_->addFunction(bf);
        } else {
            throw new std::runtime_error("Uncorrect function");
        }
    }

    it_f = Scope::FunctionIterator(node->scope());
    while (it_f.hasNext()) {
        translateFunction(it_f.next());
    }

    for (uint i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
    }

    setTosType(VT_VOID);
}

void BytecodeVisitor::visitFunctionNode(FunctionNode *node) {

    node->body()->visit(this);
    setTosType(VT_VOID);
}


void BytecodeVisitor::visitReturnNode(ReturnNode *node) {
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
        castTos(context_->getBf()->returnType());
    }
    bc()->addInsn(BC_RETURN);

    setTosType(context_->getBf()->returnType());
}

void BytecodeVisitor::visitNativeCallNode(NativeCallNode *node) {

    void *code = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (!code) {
        throw new std::runtime_error("Native function does not exist");
    }
    uint16_t fn_id = code_->makeNativeFunction(node->nativeName(), node->nativeSignature(), code);
    bc()->addInsn(BC_CALLNATIVE);
    bc()->addUInt16(fn_id);

    setTosType(node->nativeSignature()[0].first);
}

void BytecodeVisitor::visitCallNode(CallNode *node) {
    BytecodeFunction* bf = (BytecodeFunction*) code_->functionByName(node->name());
    if (!bf) {
        throw new runtime_error("Function does not exist");
    }
    if (node->parametersNumber() != bf->parametersNumber()) {
        throw new runtime_error("Uncorrect parameters number");
    }

    for (size_t i = node->parametersNumber(); node->parametersNumber() > 0 && i > 0; --i) {
        node->parameterAt(i-1)->visit(this);
        castTos(bf->parameterType(i-1));
    }


    bc()->addInsn(BC_CALL);
    bc()->addUInt16(bf->id());

    setTosType(bf->returnType());
}

void BytecodeVisitor::visitPrintNode(PrintNode *node) {
    for (uint i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        Instruction ins = BC_INVALID;
        switch (tosType()) {
            case VT_INT:
                ins = BC_IPRINT;
                break;
            case VT_DOUBLE:
                ins = BC_DPRINT;
                break;
            case VT_STRING:
                ins = BC_SPRINT;
                break;
            default:
                throw new std::runtime_error("Uncorrect type of printing parameter");
        }
        bc()->addInsn(ins);
        setTosType(VT_VOID);
    }

}



void BytecodeVisitor::addArithmeticalOp(BinaryOpNode *pNode) {
    pNode->right()->visit(this);
    VarType rightType = tosType();
    pNode->left()->visit(this);
    VarType leftType = tosType();
    castForArithmeticalOp(leftType, rightType);

    if (tosType() == VT_INT) {
        switch(pNode->kind()) {
            case tADD:
                bc()->addInsn(BC_IADD);
                break;
            case tMUL:
                bc()->addInsn(BC_IMUL);
                break;
            case tSUB:
                bc()->addInsn(BC_ISUB);
                break;
            case tDIV:
                bc()->addInsn(BC_IDIV);
                break;
            case tMOD:
                bc()->addInsn(BC_IMOD);
                break;
            default:
                throw new std::runtime_error("Error in arithmetical operation");
        }
        setTosType(VT_INT);
        return;
    }
    if (tosType() == VT_DOUBLE) {
        switch(pNode->kind()) {
            case tADD:
                bc()->addInsn(BC_DADD);
                break;
            case tMUL:
                bc()->addInsn(BC_DMUL);
                break;
            case tSUB:
                bc()->addInsn(BC_DSUB);
                break;
            case tDIV:
                bc()->addInsn(BC_DDIV);
                break;
            default:
                throw new std::runtime_error("Error in arithmetical operation");
        }
        setTosType(VT_DOUBLE);
        return;
    }

}

void BytecodeVisitor::addLogicalOp(BinaryOpNode *pNode) {

    Instruction ins = BC_INVALID;
    if (pNode->kind() == tAND) {
        ins = BC_ILOAD0;
    }
    if (pNode->kind() == tOR) {
        ins = BC_ILOAD1;
    }
    if (ins == BC_INVALID) {
        throw new runtime_error("Invalid logical operation");
    }

    pNode->left()->visit(this);
    castTos(VT_INT);

    bc()->addInsn(ins);
    Label right_end = Label(bc());
    bc()->addBranch(BC_IFICMPE, right_end);

    pNode->right()->visit(this);
    Label operation_end = Label(bc());
    bc()->addBranch(BC_JA, operation_end);

    bc()->bind(right_end);
    bc()->addInsn(ins);

    bc()->bind(operation_end);

    castTos(VT_INT);
    setTosType(VT_INT);

}

void BytecodeVisitor::addCompareOp(BinaryOpNode *pNode) {
    Instruction ins = BC_INVALID;
    switch (pNode->kind()) {
        case tEQ:
            ins = BC_IFICMPE;
            break;
        case tNEQ:
            ins = BC_IFICMPNE;
            break;
        case tGE:
            ins = BC_IFICMPGE;
            break;
        case tLE:
            ins = BC_IFICMPLE;
            break;
        case tGT:
            ins = BC_IFICMPG;
            break;
        case tLT:
            ins = BC_IFICMPL;
            break;
        default:
            throw new runtime_error("Invalid compare operation");
    }

    pNode->right()->visit(this);
    castTos(VT_INT);
    pNode->left()->visit(this);
    castTos(VT_INT);

    Label l1 = Label(bc());
    bc()->addBranch(ins, l1);
    bc()->addInsn(BC_ILOAD0);
    Label l2 = Label(bc());
    bc()->addBranch(BC_JA, l2);
    bc()->bind(l1);
    bc()->addInsn(BC_ILOAD1);
    bc()->bind(l2);

    setTosType(VT_INT);
}

void BytecodeVisitor::addBitwiseOp(BinaryOpNode *pNode) {
    pNode->right()->visit(this);
    castTos(VT_INT);
    pNode->left()->visit(this);
    castTos(VT_INT);
    switch (pNode->kind()) {
        case tAOR:
            bc()->addInsn(BC_IAOR);
            break;
        case tAAND:
            bc()->addInsn(BC_IAAND);
            break;
        case tAXOR:
            bc()->addInsn(BC_IAXOR);
            break;
        default:
            throw new runtime_error("Invalid bitwise operation");

    }

    setTosType(VT_INT);
}


void BytecodeVisitor::addNotOp(UnaryOpNode *pNode) {
    pNode->operand()->visit(this);
    if (tosType() != VT_INT) {
        throw new std::runtime_error("Uncorrect cast");
    }
    bc()->addInsn(BC_ILOAD0);
    Label l1 = Label(bc());
    bc()->addBranch(BC_IFICMPE, l1);
    bc()->addInsn(BC_ILOAD0);
    Label l2 = Label(bc());
    bc()->addBranch(BC_JA, l2);
    bc()->bind(l1);
    bc()->addInsn(BC_ILOAD1);
    bc()->bind(l2);

    setTosType(VT_INT);
}

void BytecodeVisitor::addUnarySubOp(UnaryOpNode *pNode) {
    pNode->operand()->visit(this);
    switch (tosType()) {
        case VT_STRING:
            castTos(VT_INT);
            bc()->addInsn(BC_INEG);
            break;
        case VT_INT:
            bc()->addInsn(BC_INEG);
            break;
        case VT_DOUBLE:
            bc()->addInsn(BC_DNEG);
            break;
        default:
            throw new std::runtime_error("Type error");
    }
}


void BytecodeVisitor::castTos(VarType type) {
    if (type == VT_INT) {
        switch (tosType()) {
            case VT_INT:
                break;
            case VT_DOUBLE:
                bc()->addInsn(BC_D2I);
                break;
//            case VT_STRING:
//                bc()->addInsn(BC_S2I);
//                break;
            default:
                throw new std::runtime_error("Uncorrect cast");
        }
        setTosType(VT_INT);
        return;
    }
    if (type == VT_DOUBLE) {
        switch (tosType()) {
            case VT_DOUBLE:
                break;
            case VT_INT:
                bc()->addInsn(BC_I2D);
                break;
//            case VT_STRING:
//                bc()->addInsn(BC_S2I);
//                bc()->addInsn(BC_I2D);
//                break;
            default:
                throw new std::runtime_error("Uncorrect cast");
        }
        setTosType(VT_DOUBLE);
        return;
    }
    if (type == VT_STRING && tosType() == VT_STRING) {
        return;
    }
    throw new runtime_error("Uncorrect cast");
}


void BytecodeVisitor::castForArithmeticalOp(VarType t1, VarType t2) {
    if (t1 == VT_INT) {
        switch (t2) {
            case VT_INT:
                return;
            case VT_DOUBLE:
                bc()->addInsn(BC_I2D);
                setTosType(VT_DOUBLE);
                return;
            default:
                break;
        }
    }
    if (t1 == VT_DOUBLE) {
        switch (t2) {
            case VT_DOUBLE:
                return;
            case VT_INT:
                bc()->addInsn(BC_SWAP);
                bc()->addInsn(BC_I2D);
                bc()->addInsn(BC_SWAP);
                return;
            default:
                break;
        }
    }
    throw new runtime_error("Uncorrect cast for arithmetical operation");
}




void BytecodeVisitor::translateFunction(AstFunction *f) {
    BytecodeFunction* bf = (BytecodeFunction *) code_->functionByName(f->name());
    ScopeContext* new_context = new ScopeContext(bf, f->scope(), context_);
    context_ = new_context;

    for (uint i = 0; i < f->parametersNumber(); ++i) {
        AstVar* var = f->scope()->lookupVariable(f->parameterName(i), false);
        storeVar(var);
    }

    f->node()->visit(this);

    bf->setLocalsNumber(context_->getLocalsNum());
    bf->setScopeId(context_->getId());

    context_ = new_context->getParent();
    delete new_context;

}

void BytecodeVisitor::loadVar(AstVar const * var) {
    Instruction local_ins = BC_INVALID;
    Instruction context_ins = BC_INVALID;
    switch (var->type()) {
        case VT_INT:
            local_ins = BC_LOADIVAR;
            context_ins = BC_LOADCTXIVAR;
            break;
        case VT_DOUBLE:
            local_ins = BC_LOADDVAR;
            context_ins = BC_LOADCTXDVAR;
            break;
        case VT_STRING:
            local_ins = BC_LOADSVAR;
            context_ins = BC_LOADCTXSVAR;
            break;
        default:
            throw new std::runtime_error("Uncorrect var type try to load");
    }

    VarInfo var_info = context_->getVarInfo(var->name());
    if (var_info.context_id == context_->getId()) {
        bc()->addInsn(local_ins);
    } else {
        bc()->addInsn(context_ins);
        bc()->addUInt16(var_info.context_id);
    }
    bc()->addUInt16(var_info.id);

    setTosType(var->type());
}

void BytecodeVisitor::storeVar(AstVar const * var) {
    Instruction local_ins = BC_INVALID;
    Instruction context_ins = BC_INVALID;
    switch (var->type()) {
        case VT_INT:
            local_ins = BC_STOREIVAR;
            context_ins = BC_STORECTXIVAR;
            break;
        case VT_DOUBLE:
            local_ins = BC_STOREDVAR;
            context_ins = BC_STORECTXDVAR;
            break;
        case VT_STRING:
            local_ins = BC_STORESVAR;
            context_ins = BC_STORECTXSVAR;
            break;
        default:
            throw new std::runtime_error("Uncorrect var type try to store");
    }

    VarInfo var_info = context_->getVarInfo(var->name());
    if (var_info.context_id == context_->getId()) {
        bc()->addInsn(local_ins);
    } else {
        bc()->addInsn(context_ins);
        bc()->addUInt16(var_info.context_id);
    }
    bc()->addUInt16(var_info.id);

    setTosType(var->type());
}