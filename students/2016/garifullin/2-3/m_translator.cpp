#include <dlfcn.h>
#include "m_translator.h"
#include "translator_exception.h"

using namespace mathvm;

Status* TranslatorToBytecode::translate(const string &program, Code **code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError())
        return status;

    BytecodeVisitor visitor;

    status = visitor.translateToBytecode(*code, parser.top());

    return status;
}


Status* BytecodeVisitor::translateToBytecode(Code *code, AstFunction *here) {
    _code = code;
    BytecodeFunction* byte_func = new BytecodeFunction(here);
    _code->addFunction(byte_func);
    try {
        translateFunction(here);
        byte_func->bytecode()->addInsn(BC_STOP);
    } catch(TranslatorException const &e) {
        return Status::Error(e.what());
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
        handleArithOp(node);
        break;
    case tAND:
    case tOR:
        handleLogicOp(node);
        break;
    case tAAND:
    case tAOR:
    case tAXOR:
        handleBitOp(node);
        break;
    case tEQ:
    case tNEQ:
    case tGE:
    case tLE:
    case tGT:
    case tLT:
        handleCmpOp(node);
        break;
    default:
        throw TranslatorException("Invalid binop");
    }
}


void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    switch (node->kind()) {
    case tNOT:
        this->handleNotOp(node);
        break;
    case tSUB:
        this->handleNegateOp(node);
        break;
    default:
        throw TranslatorException("Invalid unop");
    }
}


void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    bytecode()->addInsn(BC_ILOAD);
    bytecode()->addInt64(node->literal());
    setTosType(VT_INT);
}


void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    bytecode()->addInsn(BC_DLOAD);
    bytecode()->addDouble(node->literal());
    setTosType(VT_DOUBLE);
}


void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    bytecode()->addInsn(BC_SLOAD);
    bytecode()->addUInt16(_code->makeStringConstant(node->literal()));
    setTosType(VT_STRING);
}


void BytecodeVisitor::visitLoadNode(LoadNode *node) {
    loadVar(node->var());
}


void BytecodeVisitor::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);
    VarType var_type = node->var()->type();
    castTos(var_type);
    if (node->op() == tINCRSET) {
        loadVar(node->var());
        Instruction inc_insn = (var_type == VT_INT) ? BC_IADD : BC_DADD;
        bytecode()->addInsn(inc_insn);
    } else if (node->op() == tDECRSET) {
        loadVar(node->var());
        Instruction dec_insn = (var_type == VT_INT) ? BC_ISUB : BC_DSUB;
        bytecode()->addInsn(dec_insn);
    }

    storeVar(node->var());
}


void BytecodeVisitor::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    castTos(VT_INT);

    Label else_lab = Label(bytecode());
    Label end_lab = Label(bytecode());

    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, else_lab);
    node->thenBlock()->visit(this);
    bytecode()->addBranch(BC_JA, end_lab);

    bytecode()->bind(else_lab);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }
    bytecode()->bind(end_lab);

    setTosType(VT_VOID);
}


void BytecodeVisitor::visitForNode(ForNode *node) {
    BinaryOpNode* in_expr = node->inExpr()->asBinaryOpNode();
    if (!in_expr) {
        throw TranslatorException("Invalid for expression: in must be binary");
    }
    in_expr->left()->visit(this);
    castTos(VT_INT);
    if (node->var()->type() != VT_INT) {
        throw TranslatorException("Invalid for expression: iteration variable must be an int");
    }
    storeVar(node->var());

    Label start_lab = Label(bytecode());
    Label end_lab = Label(bytecode());

    bytecode()->bind(start_lab);

    in_expr->right()->visit(this);
    castTos(VT_INT);
    loadVar(node->var());
    bytecode()->addBranch(BC_IFICMPG, end_lab);

    node->body()->visit(this);

    loadVar(node->var());
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addInsn(BC_IADD);
    storeVar(node->var());
    bytecode()->addBranch(BC_JA, start_lab);
    bytecode()->bind(end_lab);

    setTosType(VT_VOID);
}


void BytecodeVisitor::visitWhileNode(WhileNode *node) {
    Label start_lab = Label(bytecode());
    Label end_lab = Label(bytecode());

    bytecode()->bind(start_lab);
    node->whileExpr()->visit(this);
    castTos(VT_INT);
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, end_lab);
    node->loopBlock()->visit(this);
    bytecode()->addBranch(BC_JA, start_lab);
    bytecode()->bind(end_lab);

    setTosType(VT_VOID);
}


void BytecodeVisitor::visitBlockNode(BlockNode *node) {

    Scope::VarIterator iter_var(node->scope());
    while (iter_var.hasNext()) {
        AstVar* var = iter_var.next();
        _context->addVar(var);
    }

    Scope::FunctionIterator iter_func(node->scope());
    while (iter_func.hasNext()) {
        AstFunction* ast_func = iter_func.next();

        BytecodeFunction* byte_func = (BytecodeFunction*) _code->functionByName(ast_func->name());
        if (!byte_func) {
            byte_func = new BytecodeFunction(ast_func);
            _code->addFunction(byte_func);
        } else {
            throw TranslatorException("Duplicate func");
        }
    }

    iter_func = Scope::FunctionIterator(node->scope());
    while (iter_func.hasNext()) {
        translateFunction(iter_func.next());
    }

    for (uint32_t i = 0; i < node->nodes(); ++i) {
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
        castTos(_context->getByteFunc()->returnType());
    }
    bytecode()->addInsn(BC_RETURN);

    setTosType(_context->getByteFunc()->returnType());
}


void BytecodeVisitor::visitNativeCallNode(NativeCallNode *node) {

    void *code = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (!code) {
        throw TranslatorException("Native func doesn't exist");
    }
    uint16_t func_id = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), code);
    bytecode()->addInsn(BC_CALLNATIVE);
    bytecode()->addUInt16(func_id);

    setTosType(node->nativeSignature()[0].first);
}


void BytecodeVisitor::visitCallNode(CallNode *node) {
    BytecodeFunction* byte_func = (BytecodeFunction*) _code->functionByName(node->name());
    if (!byte_func) {
        throw TranslatorException("Func doesn't exist");
    }
    if (node->parametersNumber() != byte_func->parametersNumber()) {
        throw TranslatorException("Incorrect number of parameters");
    }

    for (uint32_t i = node->parametersNumber(); node->parametersNumber() > 0 && i > 0; --i) {
        node->parameterAt(i-1)->visit(this);
        castTos(byte_func->parameterType(i-1));
    }

    bytecode()->addInsn(BC_CALL);
    bytecode()->addUInt16(byte_func->id());

    setTosType(byte_func->returnType());
}


void BytecodeVisitor::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        Instruction insn = BC_INVALID;
        switch (tosType()) {
        case VT_INT:
            insn = BC_IPRINT;
            break;
        case VT_DOUBLE:
            insn = BC_DPRINT;
            break;
        case VT_STRING:
            insn = BC_SPRINT;
            break;
        default:
            throw TranslatorException("Invalid type of print parameter");
        }
        bytecode()->addInsn(insn);
        setTosType(VT_VOID);
    }
}


void BytecodeVisitor::loadVar(AstVar const * var) {
    Instruction local_insn = BC_INVALID;
    Instruction context_insn = BC_INVALID;
    switch (var->type()) {
    case VT_INT:
        local_insn = BC_LOADIVAR;
        context_insn = BC_LOADCTXIVAR;
        break;
    case VT_DOUBLE:
        local_insn = BC_LOADDVAR;
        context_insn = BC_LOADCTXDVAR;
        break;
    case VT_STRING:
        local_insn = BC_LOADSVAR;
        context_insn = BC_LOADCTXSVAR;
        break;
    default:
        throw TranslatorException("Invalid type to load");
    }

    VarContext var_info = _context->getVarContext(var->name());
    if (var_info.context_id == _context->getId()) {
        bytecode()->addInsn(local_insn);
    } else {
        bytecode()->addInsn(context_insn);
        bytecode()->addUInt16(var_info.context_id);
    }
    bytecode()->addUInt16(var_info.var_id);

    setTosType(var->type());
}


void BytecodeVisitor::storeVar(AstVar const * var) {
    Instruction local_insn = BC_INVALID;
    Instruction context_insn = BC_INVALID;
    switch (var->type()) {
    case VT_INT:
        local_insn = BC_STOREIVAR;
        context_insn = BC_STORECTXIVAR;
        break;
    case VT_DOUBLE:
        local_insn = BC_STOREDVAR;
        context_insn = BC_STORECTXDVAR;
        break;
    case VT_STRING:
        local_insn = BC_STORESVAR;
        context_insn = BC_STORECTXSVAR;
        break;
    default:
        throw TranslatorException("Invalid type to store");
    }

    VarContext var_info = _context->getVarContext(var->name());
    if (var_info.context_id == _context->getId()) {
        bytecode()->addInsn(local_insn);
    } else {
        bytecode()->addInsn(context_insn);
        bytecode()->addUInt16(var_info.context_id);
    }
    bytecode()->addUInt16(var_info.var_id);

    setTosType(var->type());
}


void BytecodeVisitor::handleArithOp(BinaryOpNode *op_node) {
    op_node->right()->visit(this);
    VarType right_ty = tosType();
    op_node->left()->visit(this);
    VarType left_ty = tosType();
    castArithOp(left_ty, right_ty);

    if (tosType() == VT_INT) {
        switch(op_node->kind()) {
        case tADD:
            bytecode()->addInsn(BC_IADD);
            break;
        case tMUL:
            bytecode()->addInsn(BC_IMUL);
            break;
        case tSUB:
            bytecode()->addInsn(BC_ISUB);
            break;
        case tDIV:
            bytecode()->addInsn(BC_IDIV);
            break;
        case tMOD:
            bytecode()->addInsn(BC_IMOD);
            break;
        default:
            throw TranslatorException("Error in arith ops");
        }
        setTosType(VT_INT);
        return;
    }
    if (tosType() == VT_DOUBLE) {
        switch(op_node->kind()) {
        case tADD:
            bytecode()->addInsn(BC_DADD);
            break;
        case tMUL:
            bytecode()->addInsn(BC_DMUL);
            break;
        case tSUB:
            bytecode()->addInsn(BC_DSUB);
            break;
        case tDIV:
            bytecode()->addInsn(BC_DDIV);
            break;
        default:
            throw TranslatorException("Error in arith ops");
        }
        setTosType(VT_DOUBLE);
        return;
    }

}


void BytecodeVisitor::handleLogicOp(BinaryOpNode *op_node) {

    Instruction insn = BC_INVALID;
    if (op_node->kind() == tAND) {
        insn = BC_ILOAD0;
    }
    if (op_node->kind() == tOR) {
        insn = BC_ILOAD1;
    }
    if (insn == BC_INVALID) {
        throw TranslatorException("Invalid logical operation");
    }

    op_node->left()->visit(this);
    castTos(VT_INT);

    bytecode()->addInsn(insn);
    Label right_lab = Label(bytecode());
    bytecode()->addBranch(BC_IFICMPE, right_lab);

    op_node->right()->visit(this);
    Label end_lab = Label(bytecode());
    bytecode()->addBranch(BC_JA, end_lab);

    bytecode()->bind(right_lab);
    bytecode()->addInsn(insn);

    bytecode()->bind(end_lab);

    castTos(VT_INT);
    setTosType(VT_INT);

}


void BytecodeVisitor::handleCmpOp(BinaryOpNode *op_node) {
    Instruction insn = BC_INVALID;
    switch (op_node->kind()) {
    case tEQ:
        insn = BC_IFICMPE;
        break;
    case tNEQ:
        insn = BC_IFICMPNE;
        break;
    case tGE:
        insn = BC_IFICMPGE;
        break;
    case tLE:
        insn = BC_IFICMPLE;
        break;
    case tGT:
        insn = BC_IFICMPG;
        break;
    case tLT:
        insn = BC_IFICMPL;
        break;
    default:
        throw TranslatorException("Invalid compare operation");
    }

    op_node->right()->visit(this);
    castTos(VT_INT);
    op_node->left()->visit(this);
    castTos(VT_INT);

    Label left_lab = Label(bytecode());
    bytecode()->addBranch(insn, left_lab);
    bytecode()->addInsn(BC_ILOAD0);
    Label right_lab = Label(bytecode());
    bytecode()->addBranch(BC_JA, right_lab);
    bytecode()->bind(left_lab);
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->bind(right_lab);

    setTosType(VT_INT);
}


void BytecodeVisitor::handleBitOp(BinaryOpNode *op_node) {
    op_node->right()->visit(this);
    castTos(VT_INT);
    op_node->left()->visit(this);
    castTos(VT_INT);
    switch (op_node->kind()) {
    case tAOR:
        bytecode()->addInsn(BC_IAOR);
        break;
    case tAAND:
        bytecode()->addInsn(BC_IAAND);
        break;
    case tAXOR:
        bytecode()->addInsn(BC_IAXOR);
        break;
    default:
        throw TranslatorException("Invalid bit op");

    }

    setTosType(VT_INT);
}


void BytecodeVisitor::handleNotOp(UnaryOpNode *op_node) {
    op_node->operand()->visit(this);
    if (tosType() != VT_INT) {
        throw TranslatorException("Invalid cast");
    }
    bytecode()->addInsn(BC_ILOAD0);
    Label left_lab = Label(bytecode());
    bytecode()->addBranch(BC_IFICMPE, left_lab);
    bytecode()->addInsn(BC_ILOAD0);
    Label right_lab = Label(bytecode());
    bytecode()->addBranch(BC_JA, right_lab);
    bytecode()->bind(left_lab);
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->bind(right_lab);

    setTosType(VT_INT);
}


void BytecodeVisitor::handleNegateOp(UnaryOpNode *op_node) {
    op_node->operand()->visit(this);
    switch (tosType()) {
    case VT_STRING:
        castTos(VT_INT);
        bytecode()->addInsn(BC_INEG);
        break;
    case VT_INT:
        bytecode()->addInsn(BC_INEG);
        break;
    case VT_DOUBLE:
        bytecode()->addInsn(BC_DNEG);
        break;
    default:
        throw TranslatorException("Type error");
    }
}


void BytecodeVisitor::castTos(VarType type) {
    if (type == VT_INT) {
        switch (tosType()) {
        case VT_INT:
            break;
        case VT_DOUBLE:
            bytecode()->addInsn(BC_D2I);
            break;
        default:
            throw TranslatorException("Invalid cast");
        }
        setTosType(VT_INT);
        return;
    }
    if (type == VT_DOUBLE) {
        switch (tosType()) {
        case VT_DOUBLE:
            break;
        case VT_INT:
            bytecode()->addInsn(BC_I2D);
            break;
        default:
            throw TranslatorException("Invalid cast");
        }
        setTosType(VT_DOUBLE);
        return;
    }
    if (type == VT_STRING && tosType() == VT_STRING) {
        return;
    }
    throw TranslatorException("Invalid cast");
}


void BytecodeVisitor::castArithOp(VarType left_ty, VarType right_ty) {
    if (left_ty == VT_INT) {
        switch (right_ty) {
        case VT_INT:
            return;
        case VT_DOUBLE:
            bytecode()->addInsn(BC_I2D);
            setTosType(VT_DOUBLE);
            return;
        default:
            break;
        }
    }
    if (left_ty == VT_DOUBLE) {
        switch (right_ty) {
        case VT_DOUBLE:
            return;
        case VT_INT:
            bytecode()->addInsn(BC_SWAP);
            bytecode()->addInsn(BC_I2D);
            bytecode()->addInsn(BC_SWAP);
            return;
        default:
            break;
        }
    }
    throw TranslatorException("Invalid cast of arith op");
}


void BytecodeVisitor::translateFunction(AstFunction *ast_func) {
    BytecodeFunction* byte_func = (BytecodeFunction *) _code->functionByName(ast_func->name());
    ScopeContext* new_context = new ScopeContext(byte_func, ast_func->scope(), _context);
    _context = new_context;

    for (uint i = 0; i < ast_func->parametersNumber(); ++i) {
        AstVar* var = ast_func->scope()->lookupVariable(ast_func->parameterName(i), false);
        storeVar(var);
    }

    ast_func->node()->visit(this);

    byte_func->setLocalsNumber(_context->getLocalsNum());
    byte_func->setScopeId(_context->getId());

    _context = new_context->getParent();
    delete new_context;
}
