#include <dlfcn.h>

#include "parser.h"
#include "MyBytecodeTranslator.h"

mathvm::Status* mathvm::MyBytecodeTranslator::translate(const string &program, Code **code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }
    MyBytecodeVisitor bytecode_visitor(*code);
    return bytecode_visitor.translate(parser.top());
}

mathvm::MyBytecodeVisitor::MyBytecodeVisitor(Code *code) {
    _code = code;
}

mathvm::Status* mathvm::MyBytecodeVisitor::translate(AstFunction *root) {
    BytecodeFunction* bytecode_root = new BytecodeFunction(root);
    _code->addFunction(bytecode_root);
    process_function(root, bytecode_root);
    return Status::Ok();
}

void mathvm::MyBytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    TokenKind kind = node->kind();
    VarType type = VT_INVALID;
    switch (kind) {
    case tOR:
    case tAND:
        type = process_logical_op(node);
        break;
    case tAOR:
    case tAAND:
    case tAXOR:
        type = process_bitwise_op(node);
        break;
    case tEQ:
    case tNEQ:
    case tGE:
    case tLE:
    case tGT:
    case tLT:
        type = process_comparasion_op(node);
        break;
    case tADD:
    case tSUB:
    case tMUL:
    case tDIV:
    case tMOD:
        type = process_arithmetic_op(node);
        break;
    default:
        throw std::runtime_error("wrong kind");
    }
    _type = type;
}

mathvm::Bytecode* mathvm::MyBytecodeVisitor::get_bytecode() {
    return _scope->get_bytecode_function()->bytecode();
}

void mathvm::MyBytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    Bytecode* bytecode = get_bytecode();
    bytecode->addInsn(BC_DLOAD);
    bytecode->addDouble(node->literal());
    _type = VT_DOUBLE;
}

void mathvm::MyBytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    Bytecode* bytecode = get_bytecode();
    bytecode->addInsn(BC_ILOAD);
    bytecode->addInt64(node->literal());
    _type = VT_INT;
}

void mathvm::MyBytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    Bytecode* bytecode = get_bytecode();
    uint16_t string_id = _code->makeStringConstant(node->literal());
    bytecode->addInsn(BC_SLOAD);
    bytecode->addUInt16(string_id);
    _type = VT_STRING;
}

void mathvm::MyBytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
    Bytecode* bytecode = get_bytecode();
    switch (node->kind()) {
    case tADD:
        break;
    case tSUB:
        switch (_type) {
        case VT_INT:
            bytecode->addInsn(BC_INEG);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_DNEG);
            break;
        default:
            throw std::runtime_error("wrong type for unary op");
        }
        break;
    case tNOT:
    {
        if (_type != VT_INT) {
            throw std::runtime_error("wrong type for not op");
        }
        bytecode->addInsn(BC_ILOAD0);
        Label l1 = Label(bytecode);
        bytecode->addBranch(BC_IFICMPE, l1);
        bytecode->addInsn(BC_ILOAD);
        Label l2 = Label(bytecode);
        bytecode->addBranch(BC_JA, l2);
        bytecode->bind(l1);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->bind(l2);
        _type = VT_INT;
        break;
    }
    default:
        throw std::runtime_error("wrong unary op");
    }
}

void mathvm::MyBytecodeVisitor::visitBlockNode(BlockNode *node) {
    Scope::VarIterator var_iter(node->scope());
    _scope->add_vars(var_iter);
    Scope::FunctionIterator func_iter(node->scope());
    while(func_iter.hasNext()) {
        AstFunction* ast_func = func_iter.next();
        if (_code->functionByName(ast_func->name()) != nullptr) {
            throw std::runtime_error("func already defined");
        } else {
            _code->addFunction(new BytecodeFunction(ast_func));
        }
    }
    func_iter = Scope::FunctionIterator(node->scope());
    while(func_iter.hasNext()) {
        AstFunction* ast_func = func_iter.next();
        process_function(ast_func, (BytecodeFunction*)_code->functionByName(ast_func->name()));
    }
    for (uint32_t i = 0; i < node->nodes(); i++) {
        node->nodeAt(i)->visit(this);
    }
    _type = VT_VOID;
}

void mathvm::MyBytecodeVisitor::visitCallNode(CallNode *node) {
    BytecodeFunction* bc_func = (BytecodeFunction*)_code->functionByName(node->name());

    if (bc_func == nullptr)
        throw std::runtime_error("function not found");
    if (bc_func->parametersNumber() != node->parametersNumber())
        throw std::runtime_error("wrong number of parameters");

    Bytecode* bytecode = get_bytecode();
    for (uint32_t i = node->parametersNumber(); i>=1; i--) {
        node->parameterAt(i-1)->visit(this);
        cast_type_to(bc_func->parameterType(i-1));
    }
    bytecode->addInsn(BC_CALL);
    bytecode->addUInt16(bc_func->id());
    _type = bc_func->returnType();
}

void mathvm::MyBytecodeVisitor::cast_type_to(VarType target) {
    if (_type == target) {
        return;
    }
    Bytecode* bytecode = get_bytecode();
    if (_type == VT_INT && target == VT_DOUBLE) {
        bytecode->addInsn(BC_I2D);
        return;
    }
    if (_type == VT_DOUBLE && target == VT_INT) {
        bytecode->addInsn(BC_D2I);
        return;
    }
    throw std::runtime_error("wrong cast");
}

void mathvm::MyBytecodeVisitor::visitForNode(ForNode *node) {
    Bytecode* bytecode = get_bytecode();
    BinaryOpNode* range = node->inExpr()->asBinaryOpNode();

    range->left()->visit(this);
    cast_type_to(VT_INT);

    store_var(node->var());

    Label l1 = Label(bytecode);
    bytecode->bind(l1);

    range->right()->visit(this);
    cast_type_to(VT_INT);

    load_var(node->var());

    Label l2 = Label(bytecode);
    bytecode->addBranch(BC_IFICMPG, l2);

    node->body()->visit(this);
    load_var(node->var());
    bytecode->addInsn(BC_ILOAD1);
    bytecode->addInsn(BC_IADD);
    store_var(node->var());
    bytecode->addBranch(BC_JA, l1);
    bytecode->bind(l2);
    _type = VT_VOID;
}

void mathvm::MyBytecodeVisitor::visitFunctionNode(FunctionNode *node) {
    node->body()->visit(this);
    _type = node->returnType();
}

void mathvm::MyBytecodeVisitor::visitIfNode(IfNode *node) {
    Bytecode* bytecode = get_bytecode();
    Label l1(bytecode);
    node->ifExpr()->visit(this);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, l1);
    node->thenBlock()->visit(this);
    if (node->elseBlock() != nullptr) {
        Label l2(bytecode);
        bytecode->addBranch(BC_JA, l2);
        bytecode->bind(l1);
        node->elseBlock()->visit(this);
        bytecode->bind(l2);
    } else {
        bytecode->bind(l1);
    }
}

void mathvm::MyBytecodeVisitor::visitLoadNode(LoadNode *node) {
    load_var(node->var());
}

void mathvm::MyBytecodeVisitor::visitNativeCallNode(NativeCallNode *node) {
    void* ext_func = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (ext_func == nullptr) {
        throw std::runtime_error("cant find ext func");
    }
    Bytecode* bytecode = get_bytecode();
    uint16_t ext_func_id = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), ext_func);
    bytecode->addInsn(BC_CALLNATIVE);
    bytecode->addUInt16(ext_func_id);
    _type = node->nativeSignature().front().first;
}

void mathvm::MyBytecodeVisitor::visitPrintNode(PrintNode *node) {
    Bytecode* bytecode = get_bytecode();
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        switch (_type) {
        case VT_DOUBLE:
            bytecode->addInsn(BC_DPRINT);
            break;
        case VT_INT:
            bytecode->addInsn(BC_IPRINT);
            break;
        case VT_STRING:
            bytecode->addInsn(BC_SPRINT);
            break;
        default:
            throw std::runtime_error("wrong type for print");
        }
    }
    _type = VT_VOID;
}


void mathvm::MyBytecodeVisitor::visitReturnNode(ReturnNode *node) {
    Bytecode* bytecode = get_bytecode();

    if (node->returnExpr() != nullptr) {
        node->returnExpr()->visit(this);
    }

    VarType type = _scope->get_bytecode_function()->returnType();
    cast_type_to(type);
    bytecode->addInsn(BC_RETURN);
    _type = type;
}

void mathvm::MyBytecodeVisitor::visitStoreNode(StoreNode *node) {
    Bytecode* bytecode = get_bytecode();
    node->value()->visit(this);
    cast_type_to(node->var()->type());

    switch (node->op()) {
    case tINCRSET:
        load_var(node->var());
        switch (node->var()->type()) {
        case VT_DOUBLE:
            bytecode->addInsn(BC_DADD);
            break;
        case VT_INT:
            bytecode->addInsn(BC_IADD);
            break;
        default:
            throw std::runtime_error("wrong type for tINCRSET");
        }
        break;
    case tDECRSET:
        load_var(node->var());
        switch (node->var()->type()) {
        case VT_DOUBLE:
            bytecode->addInsn(BC_DSUB);
            break;
        case VT_INT:
            bytecode->addInsn(BC_ISUB);
            break;
        default:
            throw std::runtime_error("wrong type for tDECRSET");
        }
        break;
    case tASSIGN:
        break;
    default:
        throw std::runtime_error("wrong type for store");
    }

    store_var(node->var());
}

void mathvm::MyBytecodeVisitor::visitWhileNode(WhileNode *node) {
    Bytecode* bytecode = get_bytecode();
    Label l1 = bytecode->currentLabel();
    node->whileExpr()->visit(this);
    Label l2 = Label(bytecode);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, l2);
    node->loopBlock()->visit(this);
    bytecode->addBranch(BC_JA, l1);
    bytecode->bind(l2);
}

mathvm::VarType mathvm::MyBytecodeVisitor::process_arithmetic_op(BinaryOpNode *node) {
    Bytecode* bytecode = get_bytecode();
    node->right()->visit(this);
    VarType right_type = _type;
    node->left()->visit(this);
    VarType left_type = _type;
    if ((node->kind() == tMOD) && (left_type != VT_INT || right_type != VT_INT)) {
        throw std::runtime_error("wrong types for mod");
    }
    VarType res_type = get_common_type(left_type, right_type);
    switch (node->kind()) {
    case tADD:
        switch (res_type) {
        case VT_INT:
            bytecode->addInsn(BC_IADD);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_DADD);
            break;
        default:
            break;
        }
        break;
    case tSUB:
        switch (res_type) {
        case VT_INT:
            bytecode->addInsn(BC_ISUB);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_DSUB);
            break;
        default:
            break;
        }
        break;
    case tMUL:
        switch (res_type) {
        case VT_INT:
            bytecode->addInsn(BC_IMUL);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_DMUL);
            break;
        default:
            break;
        }
        break;
    case tDIV:
        switch (res_type) {
        case VT_INT:
            bytecode->addInsn(BC_IDIV);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_DDIV);
            break;
        default:
            break;
        }
        break;
    case tMOD:
        switch (res_type) {
        case VT_INT:
            bytecode->addInsn(BC_IMOD);
            break;
        default:
            break;
        }
        break;
    default:
        throw std::runtime_error("wrong type for arithmetic_op");
    }
    return res_type;
}

mathvm::VarType mathvm::MyBytecodeVisitor::process_bitwise_op(BinaryOpNode *node) {
    Bytecode* bytecode = get_bytecode();
    node->right()->visit(this);
    VarType right_type = _type;
    node->left()->visit(this);
    VarType left_type = _type;
    if (left_type!= VT_INT || right_type != VT_INT) {
        throw std::runtime_error("wrong types for bitwise_op");
    }

    switch (node->kind()) {
    case tAOR:
        bytecode->addInsn(BC_IAOR);
        break;
    case tAAND:
        bytecode->addInsn(BC_IAAND);
        break;
    case tAXOR:
        bytecode->addInsn(BC_IAXOR);
        break;
    default:
        throw std::runtime_error("wrong bitwise_op");
    }

    return VT_INT;
}

mathvm::VarType mathvm::MyBytecodeVisitor::process_comparasion_op(BinaryOpNode *node) {
    Bytecode* bytecode = get_bytecode();
    node->right()->visit(this);
    VarType right_type = _type;
    node->left()->visit(this);
    VarType left_type = _type;
    VarType res_type = get_common_type(left_type, right_type);
    switch (res_type) {
    case VT_INT:
        bytecode->addInsn(BC_ICMP);
        break;
    case VT_DOUBLE:
        bytecode->addInsn(BC_DCMP);
        break;
    default:
        throw std::runtime_error("wrong type for comparasion_op");
    }

    bytecode->addInsn(BC_ILOAD0);
    bytecode->addInsn(BC_SWAP);

    Label l1(bytecode);
    Label l2(bytecode);


    switch (node->kind()) {
    case tEQ:
        bytecode->addBranch(BC_IFICMPE, l1);
        break;
    case tNEQ:
        bytecode->addBranch(BC_IFICMPNE, l1);
        break;
    case tGT:
        bytecode->addBranch(BC_IFICMPG, l1);
        break;
    case tGE:
        bytecode->addBranch(BC_IFICMPGE, l1);
        break;
    case tLT:
        bytecode->addBranch(BC_IFICMPL, l1);
        break;
    case tLE:
        bytecode->addBranch(BC_IFICMPLE, l1);
        break;
    default:
        throw std::runtime_error("wrong comparasion_op");
    }
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_JA, l2);
    bytecode->bind(l1);
    bytecode->addInsn(BC_ILOAD1);
    bytecode->bind(l2);
    return VT_INT;
}

mathvm::VarType mathvm::MyBytecodeVisitor::process_logical_op(BinaryOpNode *node) {
    Bytecode* bytecode = get_bytecode();

    node->left()->visit(this);
    cast_type_to(VT_INT);

    bytecode->addInsn(BC_ILOAD0);
    Label l1(bytecode);

    switch (node->kind()) {
    case tOR:
        bytecode->addBranch(BC_IFICMPNE, l1);
        break;
    case tAND:
        bytecode->addBranch(BC_IFICMPE, l1);
        break;
    default:
        throw std::runtime_error("wrong logical_op");
    }

    node->right()->visit(this);
    cast_type_to(VT_INT);

    Label l2(bytecode);
    bytecode->addBranch(BC_JA, l2);
    bytecode->bind(l1);

    switch (node->kind()) {
    case tOR:
        bytecode->addInsn(BC_ILOAD1);
        break;
    case tAND:
        bytecode->addInsn(BC_ILOAD0);
        break;
    default:
        throw std::runtime_error("wrong logical_op");
    }
    bytecode->bind(l2);

    return VT_INT;
}

void mathvm::MyBytecodeVisitor::process_function(AstFunction *ast_function, BytecodeFunction *bytecode_function) {
    MyScope* new_scope = new MyScope(bytecode_function, ast_function->scope(), _scope);
    _scope = new_scope;
    for (uint32_t i = 0; i < ast_function->parametersNumber(); i++) {
        store_var(ast_function->scope()->lookupVariable(ast_function->parameterName(i), false));
    }
    ast_function->node()->visit(this);
    bytecode_function->setLocalsNumber(_scope->get_num_of_vars());
    bytecode_function->setScopeId(_scope->get_scope_id());
    _scope = _scope->get_parent();
    delete new_scope;
}

void mathvm::MyBytecodeVisitor::store_var(const AstVar *var) {
    Bytecode* bytecode = get_bytecode();
    MyVar my_var = _scope->get_var(var->name());
    if (my_var.var_scope == _scope->get_scope_id()) {
        switch (var->type()) {
        case VT_INT:
            bytecode->addInsn(BC_STOREIVAR);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_STOREDVAR);
            break;
        case VT_STRING:
            bytecode->addInsn(BC_STORESVAR);
            break;
        default:
            throw std::runtime_error("wrong var_type for store");
        }
    } else {
        switch (var->type()) {
        case VT_INT:
            bytecode->addInsn(BC_STORECTXIVAR);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_STORECTXDVAR);
            break;
        case VT_STRING:
            bytecode->addInsn(BC_STORECTXSVAR);
            break;
        default:
            throw std::runtime_error("wrong var_type for store");
        }
        bytecode->addUInt16(my_var.var_scope);
    }
    bytecode->addUInt16(my_var.var_id);
    _type = var->type();
}

void mathvm::MyBytecodeVisitor::load_var(const AstVar *var) {
    Bytecode* bytecode = get_bytecode();
    MyVar my_var = _scope->get_var(var->name());
    if (my_var.var_scope == _scope->get_scope_id()) {
        switch (var->type()) {
        case VT_INT:
            bytecode->addInsn(BC_LOADIVAR);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_LOADDVAR);
            break;
        case VT_STRING:
            bytecode->addInsn(BC_LOADSVAR);
            break;
        default:
            throw std::runtime_error("wrong var_type for load");
        }
    } else {
        switch (var->type()) {
        case VT_INT:
            bytecode->addInsn(BC_LOADCTXIVAR);
            break;
        case VT_DOUBLE:
            bytecode->addInsn(BC_LOADCTXDVAR);
            break;
        case VT_STRING:
            bytecode->addInsn(BC_LOADCTXSVAR);
            break;
        default:
            throw std::runtime_error("wrong var_type for load");
        }
        bytecode->addUInt16(my_var.var_scope);
    }
    bytecode->addUInt16(my_var.var_id);
    _type = var->type();
}

mathvm::VarType mathvm::MyBytecodeVisitor::get_common_type(VarType left_type, VarType right_type) {
    Bytecode* bytecode = get_bytecode();
    if (left_type == right_type) {
        return left_type;
    }

    if (!((left_type == VT_INT && right_type == VT_DOUBLE) || (left_type == VT_DOUBLE && right_type == VT_INT))) {
        throw std::runtime_error("wrong types for cast");
    }

    if (left_type == VT_INT) {
        bytecode->addInsn(BC_I2D);
    } else {
        bytecode->addInsn(BC_SWAP);
        bytecode->addInsn(BC_I2D);
        bytecode->addInsn(BC_SWAP);
    }
    return VT_DOUBLE;
}

mathvm::Translator* mathvm::Translator::create(const string &impl) {
    return new MyBytecodeTranslator();
}
