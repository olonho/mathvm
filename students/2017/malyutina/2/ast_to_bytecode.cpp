#include "include/ast_to_bytecode.h"

ast_to_bytecode::ast_to_bytecode() : _code(new byte_code) {}

ast_to_bytecode::~ast_to_bytecode() { delete _code; }

Code *ast_to_bytecode::code() {
    Code *result = _code;
    _code = nullptr;
    return result;
}

void ast_to_bytecode::visitTop(AstFunction *top) {
    _code->addFunction(new BytecodeFunction(top));
    visitAstFunction(top);
    std::string topFunctionName = "<top>";
    ((BytecodeFunction *) _code->
            functionByName(topFunctionName))->bytecode()->addInsn(BC_RETURN);
}

void ast_to_bytecode::visitAstFunction(AstFunction *astFunction) {
    _stack.push(
            static_cast<BytecodeFunction *>(
                    _code->functionByName(astFunction->name())));
    newContext();
    astFunction->node()->visit(this);
    _stack.pop();
}

void ast_to_bytecode::visitBlockNode(BlockNode *node) {
    addVariables(node);
    addFunctions(node);
    visitFunctions(node);
    node->visitChildren(this);
}

void ast_to_bytecode::visitCallNode(CallNode *node) {
    auto function = _code->functionByName(mangledFunctionName(node->name()));
    auto functionId = function->id();
    for (size_t i = 0; i != node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        bool typecastSuccess = typecastStackValueTo(function->parameterType(i));
        if (!typecastSuccess) {
            typeMismatch(node->parameterAt(i));
        }
    }
    insn(BC_CALL);
    int16(functionId);
    if (function->returnType() != VT_VOID) {
        pushTypeToStack(function->returnType());
    }
}

void ast_to_bytecode::visitFunctionNode(FunctionNode *node) {
    getVariablesFro_stackReverseOrder(node);
    node->body()->visit(this);
    if (!isTopFunction(node) && node->returnType() != VT_VOID) {
        bool typecastSuccess = typecastStackValueTo(node->returnType());
        if (!typecastSuccess) {
            typeMismatch(node);
        }
    }
    popContext();
}

void ast_to_bytecode::typeMismatch(AstNode *node) {
    throw translate_exception(Status::Error("Type mismatch",
                                             node->position()));
}

void ast_to_bytecode::getVariablesFro_stackReverseOrder(FunctionNode *node) {
    for (int32_t i = node->parametersNumber() - 1; i != -1; --i) {
        auto variableId = newVariable(node->parameterName(i));
        switch (node->parameterType(i)) {
            case VT_INT:
                insn(BC_STOREIVAR);
                break;
            case VT_DOUBLE:
                insn(BC_STOREDVAR);
                break;
            case VT_STRING:
                insn(BC_STORESVAR);
                break;
            default:
                invalid();
                break;
        }
        int16(variableId);
    }
}

void ast_to_bytecode::visitReturnNode(ReturnNode *node) {
    if (node->returnExpr()) {
        if (currentFunction()->returnType() == VT_VOID) {
            throw translate_exception(
                    Status::Error(("Function " + currentFunction()->name() +
                                   " returns something instead of declared void").c_str(), node->position()));
        }
        node->returnExpr()->visit(this);
        typecastStackValueTo(currentFunction()->returnType());
    }
    insn(BC_RETURN);
}

void ast_to_bytecode::visitStringLiteralNode(StringLiteralNode *node) {
    uint16_t constantId = _code->makeStringConstant(node->literal());
    insn(BC_SLOAD);
    int16(constantId);
    stringTypeToStack();
}

void ast_to_bytecode::visitIntLiteralNode(IntLiteralNode *node) {
    insn(BC_ILOAD);
    currentBytecode()->addInt64(node->literal());
    intTypeToStack();
}

void ast_to_bytecode::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    insn(BC_DLOAD);
    currentBytecode()->addDouble(node->literal());
    doubleTypeToStack();
}

void ast_to_bytecode::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        printOneOperand(node->operandAt(i));
    }
}

void ast_to_bytecode::visitLoadNode(LoadNode *node) {
    auto variableId = findVariableInContexts(node->var()->name());
    loadVariable(node->var()->type(), variableId);
}

void ast_to_bytecode::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);
    auto variableId = findVariableInContexts(node->var()->name());
    if (node->var()->type() == VT_STRING) {
        storeString(variableId);
    } else {
        if (node->op() != tASSIGN) {
            loadVariable(node->var()->type(), variableId);

            bool typecastSuccess = typecastStackValuesToCommonType();
            if (!typecastSuccess) {
                typeMismatch(node);
            }
            if (node->op() == tINCRSET) {
                add();
            } else if (node->op() == tDECRSET) {
                sub();
            }
        }

        bool typecastSuccess = typecastStackValueTo(node->var()->type());
        if (!typecastSuccess) {
            typeMismatch(node);
        }

        storeVariable(node->var()->type(), variableId);
    }
    popTypeFro_stack();
}

void ast_to_bytecode::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
    if (ttos() == VT_INT) {
        switch (node->kind()) {
            case tADD:
                nop();
                break;
            case tSUB:
                insn(BC_INEG);
                break;
            case tNOT:
                notop();
                break;
            default:
                break;
        }
    } else if (ttos() == VT_DOUBLE) {
        switch (node->kind()) {
            case tADD:
                nop();
                break;
            case tSUB:
                insn(BC_DNEG);
                break;
            case tNOT:
                typeMismatch(node);
                break;
            default:
                break;
        }
    } else {
        typeMismatch(node);
    }
}

void ast_to_bytecode::visitBinaryOpNode(BinaryOpNode *node) {
    node->right()->visit(this);
    node->left()->visit(this);
    bool typecastSuccess = typecastStackValuesToCommonType();
    if (!typecastSuccess) {
        typeMismatch(node);
    }
    switch (node->kind()) {
        case tOR:
            lor();
            break;
        case tAND:
            land();
            break;
        case tAOR:
            iaor();
            break;
        case tAAND:
            iaand();
            break;
        case tAXOR:
            iaxor();
            break;
        case tEQ:
            eq();
            break;
        case tNEQ:
            neq();
            break;
        case tGT:
            gt();
            break;
        case tLT:
            lt();
            break;
        case tLE:
            le();
            break;
        case tGE:
            ge();
            break;
        case tADD:
            add();
            break;
        case tSUB:
            sub();
            break;
        case tMUL:
            mul();
            break;
        case tDIV:
            div();
            break;
        case tMOD:
            mod();
            break;
        case tRANGE:
            if (!isRangeAllowed()) {
                throw translate_exception(Status::Error(
                        "Ranges are allowed only in for loops", node->position()));
            }
            disallowRange();
            break;
        default:
            break;
    }
}

void ast_to_bytecode::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    Label afterTrueBlockLabel(currentBytecode());
    izero();
    currentBytecode()->addBranch(BC_IFICMPE, afterTrueBlockLabel);

    node->thenBlock()->visit(this);

    Label afterFalseBlockLabel(currentBytecode());
    currentBytecode()->addBranch(BC_JA, afterFalseBlockLabel);

    currentBytecode()->bind(afterTrueBlockLabel);

    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }

    currentBytecode()->bind(afterFalseBlockLabel);
}

void ast_to_bytecode::visitForNode(ForNode *node) {
    if (!node->inExpr()->isBinaryOpNode() ||
        node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
        throw translate_exception(
                Status::Error("Malformed 'in' expression",
                              node->inExpr()->position()));
    }
    allowRange();
    node->inExpr()->visit(this);

    auto counterVariableId = findVariableInContexts(node->var()->name());
    auto counterVariableType = node->var()->type();
    if (counterVariableType != VT_INT) {
        throw translate_exception(Status::Error(
                ("For counter variable is " + std::string(typeToName(counterVariableType)) +
                 " instead of int").c_str(), node->position()));
    }
    storeVariable(ttos(), counterVariableId);
    popTypeFro_stack();


    Label beforeConditionLabel(currentBytecode());
    currentBytecode()->bind(beforeConditionLabel);

    Label afterForLabel(currentBytecode());

    loadVariable(counterVariableType, counterVariableId);

    currentBytecode()->addBranch(BC_IFICMPG, afterForLabel);

    node->body()->visit(this);

    iinc(counterVariableId);

    currentBytecode()->addBranch(BC_JA, beforeConditionLabel);

    currentBytecode()->bind(afterForLabel);

    pop();
}

void ast_to_bytecode::iinc(id_variable &id) {
    loadInt(id);
    insn(BC_ILOAD1);
    add();
    storeInt(id);
}

void ast_to_bytecode::visitWhileNode(WhileNode *node) {
    Label beforeConditionLabel(currentBytecode());
    currentBytecode()->bind(beforeConditionLabel);

    Label afterWhileLabel(currentBytecode());

    node->whileExpr()->visit(this);
    izero();
    currentBytecode()->addBranch(BC_IFICMPE, afterWhileLabel);

    node->loopBlock()->visit(this);

    currentBytecode()->addBranch(BC_JA, beforeConditionLabel);

    currentBytecode()->bind(afterWhileLabel);
}

Bytecode *ast_to_bytecode::currentBytecode() { return currentFunction()->bytecode(); }

BytecodeFunction *ast_to_bytecode::currentFunction() { return _stack.top(); }

void ast_to_bytecode::insn(Instruction instruction) { currentBytecode()->addInsn(instruction); }

void ast_to_bytecode::int16(int16_t value) { currentBytecode()->addInt16(value); }

void ast_to_bytecode::nop() {}

void ast_to_bytecode::lor() {
    if (ttos() == VT_INT) {
        iaor();
        izero();
        icmp();
    } else {
        dzero();
        dcmp();
        swap();
        dzero();
        dcmp();
        lor();
    }
}

void ast_to_bytecode::land() {
    if (ttos() == VT_INT) {
        izero();
        eq();
        swap();
        izero();
        eq();
    } else {
        dzero();
        eq();
        swap();
        dzero();
        eq();
    }
    lor();
    notop();
}

void ast_to_bytecode::eq() {
    neq();
    notop();
}

void ast_to_bytecode::neq() {
    if (ttos() == VT_INT) {
        icmp();
    } else {
        dcmp();
    }
}

void ast_to_bytecode::notop() {
    izero();
    icmp();
    iload1();
    iaand();
    iload1();
    iaxor();
}

void ast_to_bytecode::gt() {
    if (ttos() == VT_INT) {
        icmp();
    } else {
        dcmp();
    }
    iload1();
    eq();
}

void ast_to_bytecode::iloadm1() {
    insn(BC_ILOADM1);
    intTypeToStack();
}

void ast_to_bytecode::iload1() {
    insn(BC_ILOAD1);
    intTypeToStack();
}

void ast_to_bytecode::lt() {
    if (ttos() == VT_INT) {
        icmp();
    } else {
        dcmp();
    }
    iloadm1();
    eq();
}

void ast_to_bytecode::ge() {
    lt();
    notop();
}

void ast_to_bytecode::le() {
    gt();
    notop();
}

void ast_to_bytecode::add() {
    if (ttos() == VT_INT) {
        insn(BC_IADD);
    } else {
        insn(BC_DADD);
    }
    popTypeFro_stack();
}

void ast_to_bytecode::sub() {
    if (ttos() == VT_INT) {
        insn(BC_ISUB);
    } else {
        insn(BC_DSUB);
    }
    popTypeFro_stack();
}

void ast_to_bytecode::mul() {
    if (ttos() == VT_INT) {
        imul();
    } else {
        dmul();
    }
}

void ast_to_bytecode::div() {
    if (ttos() == VT_INT) {
        insn(BC_IDIV);
    } else {
        insn(BC_DDIV);
    }
    popTypeFro_stack();
}

void ast_to_bytecode::mod() {
    insn(BC_IMOD);
    two2int();
}

void ast_to_bytecode::pop() {
    insn(BC_POP);
    popTypeFro_stack();
}

void ast_to_bytecode::invalid() {
    insn(BC_INVALID);
    pushTypeToStack(VT_INVALID);
}

void ast_to_bytecode::icmp() {
    insn(BC_ICMP);
    two2int();
}

void ast_to_bytecode::dcmp() {
    insn(BC_DCMP);
    two2int();
}


void ast_to_bytecode::izero() {
    insn(BC_ILOAD0);
    intTypeToStack();
}

void ast_to_bytecode::dzero() {
    insn(BC_DLOAD0);
    doubleTypeToStack();
}

void ast_to_bytecode::imul() {
    insn(BC_IMUL);
    two2int();
}

void ast_to_bytecode::dmul() {
    insn(BC_DMUL);
    two2double();
}

void ast_to_bytecode::iaor() {
    insn(BC_IAOR);
    two2int();
}

void ast_to_bytecode::iaand() {
    insn(BC_IAAND);
}

void ast_to_bytecode::iaxor() {
    insn(BC_IAXOR);
}

void ast_to_bytecode::two2int() {
    pop2TypesFro_stack();
    intTypeToStack();
}

void ast_to_bytecode::two2double() {
    pop2TypesFro_stack();
    doubleTypeToStack();
}

void ast_to_bytecode::swap() {
    auto topType = _varTypeStack.back();
    auto secondFromTop = _varTypeStack[_varTypeStack.size() - 2];
    _varTypeStack[_varTypeStack.size() - 1] = secondFromTop;
    _varTypeStack[_varTypeStack.size() - 2] = topType;
    insn(BC_SWAP);
}

id_variable ast_to_bytecode::internalVariableId(int16_t internalId) {
    return std::make_pair(0, internalId);
}


VarType ast_to_bytecode::ttos() { return _varTypeStack.back(); }

void ast_to_bytecode::pushTypeToStack(VarType type) { _varTypeStack.push_back(type); }

void ast_to_bytecode::intTypeToStack() { pushTypeToStack(VT_INT); }

void ast_to_bytecode::doubleTypeToStack() { pushTypeToStack(VT_DOUBLE); }

void ast_to_bytecode::stringTypeToStack() { pushTypeToStack(VT_STRING); }

void ast_to_bytecode::popTypeFro_stack() { _varTypeStack.pop_back(); }

void ast_to_bytecode::pop2TypesFro_stack() {
    popTypeFro_stack();
    popTypeFro_stack();
}

void ast_to_bytecode::printOneOperand(AstNode *node) {
    node->visit(this);
    Instruction printInstruction = BC_INVALID;
    switch (ttos()) {
        case VT_STRING:
            printInstruction = BC_SPRINT;
            break;
        case VT_INT:
            printInstruction = BC_IPRINT;
            break;
        case VT_DOUBLE:
            printInstruction = BC_DPRINT;
            break;
        default:
            break;
    }
    insn(printInstruction);
    popTypeFro_stack();
}

void ast_to_bytecode::varId(id_variable &id) {
    if (!isInLocalContext(id)) {
        int16(id.first);
    }
    int16(id.second);
}

void ast_to_bytecode::storeVariable(VarType type, id_variable &id) {
    switch (type) {
        case VT_INT:
            storeInt(id);
            break;
        case VT_DOUBLE:
            storeDouble(id);
            break;
        case VT_STRING:
            storeString(id);
            break;
        default:
            invalid();
            break;
    }
}

void ast_to_bytecode::storeInt(id_variable &id) {
    if (isInLocalContext(id)) {
        insn(BC_STOREIVAR);
    } else {
        insn(BC_STORECTXIVAR);
    }
    varId(id);
}

void ast_to_bytecode::storeDouble(id_variable &id) {
    if (isInLocalContext(id)) {
        insn(BC_STOREDVAR);
    } else {
        insn(BC_STORECTXDVAR);
    }
    varId(id);
}

void ast_to_bytecode::storeString(id_variable &id) {
    if (isInLocalContext(id)) {
        insn(BC_STORESVAR);
    } else {
        insn(BC_STORECTXSVAR);
    }
    varId(id);
}

void ast_to_bytecode::loadVariable(VarType type, id_variable &id) {
    switch (type) {
        case VT_INT:
            loadInt(id);
            break;
        case VT_DOUBLE:
            loadDouble(id);
            break;
        case VT_STRING:
            loadString(id);
            break;
        default:
            invalid();
            break;
    }
}

void ast_to_bytecode::loadInt(id_variable &id) {
    if (isInLocalContext(id)) {
        insn(BC_LOADIVAR);
    } else {
        insn(BC_LOADCTXIVAR);
    }
    varId(id);
    intTypeToStack();
}

void ast_to_bytecode::loadDouble(id_variable &id) {
    if (isInLocalContext(id)) {
        insn(BC_LOADDVAR);
    } else {
        insn(BC_LOADCTXDVAR);
    }
    varId(id);
    doubleTypeToStack();
}

void ast_to_bytecode::loadString(id_variable &id) {
    if (isInLocalContext(id)) {
        insn(BC_LOADSVAR);
    } else {
        insn(BC_LOADCTXSVAR);
    }
    varId(id);
    stringTypeToStack();
}

variable_context &ast_to_bytecode::currentContext() { return _varStack.back(); }

void ast_to_bytecode::newContext() {
    if (_varStack.empty()) {
        _varStack.push_back(variable_context());
    } else {
        _varStack.push_back(
                variable_context(currentContext().contextId() + 1));
    }
}

void ast_to_bytecode::popContext() {
    currentFunction()->setLocalsNumber(currentContext().localsNumber());
    _varStack.pop_back();
}

id_variable ast_to_bytecode::findVariableInContexts(const std::string &name) {
    for (auto it = _varStack.rbegin(); it != _varStack.rend(); ++it) {
        if (it->variableExists(name)) {
            return it->find_id_variable(name);
        }
    }
    return id_variable();
}

uint16_t ast_to_bytecode::newVariable(const std::string name) {
    auto id = currentContext().newVar(name);
    if (currentContext().hasOverflowed()) {
        throw translate_exception(
                Status::Error("Too many variables in a context"));
    }
    return id;
}

bool ast_to_bytecode::typecastStackValueTo(VarType desiredType) {
    if (desiredType == ttos()) {
        return true;
    }
    if (ttos() == VT_STRING || desiredType == VT_STRING) {
        return false;
    }
    if (ttos() == VT_INT) {
        int2double();
    } else {
        double2int();
    }
    return true;
}

void ast_to_bytecode::int2double() {
    insn(BC_I2D);
    popTypeFro_stack();
    doubleTypeToStack();
}

void ast_to_bytecode::double2int() {
    insn(BC_D2I);
    popTypeFro_stack();
    intTypeToStack();
}

bool ast_to_bytecode::typecastStackValuesToCommonType() {
    auto last = _varTypeStack.back();
    auto prevLast = _varTypeStack[_varTypeStack.size() - 2];
    if (last == VT_INVALID || prevLast == VT_INVALID ||
        last == VT_STRING || prevLast == VT_STRING) {
        return false;
    }

    if (last != prevLast) {
        if (last == VT_INT) {
            int2double();
        } else {
            swap();
            int2double();
            swap();
        }
    }
    return true;
}

void ast_to_bytecode::addVariables(BlockNode *node) {
    Scope::VarIterator iter(node->scope());
    while (iter.hasNext()) {
        newVariable(iter.next()->name());
    }
}

void ast_to_bytecode::addFunctions(BlockNode *node) {
    Scope::FunctionIterator iter(node->scope());
    while (iter.hasNext()) {
        auto function = iter.next();
        if (!isFunctionKnown(function->name())) {
            _code->addFunction(new BytecodeFunction(function));
        }
    }
}

void ast_to_bytecode::visitFunctions(BlockNode *node) {
    Scope::FunctionIterator iter(node->scope());
    while (iter.hasNext()) {
        visitAstFunction(iter.next());
    }
}

void ast_to_bytecode::pop2ndTop() {
    swap();
    insn(BC_POP);
}

void ast_to_bytecode::pop23thTop() {
    pop2ndTop();
    pop2ndTop();
}

void ast_to_bytecode::pop234thTop() {
    pop2ndTop();
    pop23thTop();
}

string ast_to_bytecode::mangledFunctionName(const string &functionName) {
    return functionName;
}

bool ast_to_bytecode::isInLocalContext(id_variable &id) {
    return currentContext().contextId() == id.first;
}

bool ast_to_bytecode::isTopFunction(FunctionNode *node) {
    return node->name() == "<top>";
}

void ast_to_bytecode::allowRange() {
    mRangeAllowed = true;
}

void ast_to_bytecode::disallowRange() {
    mRangeAllowed = false;
}

bool ast_to_bytecode::isRangeAllowed() {
    return mRangeAllowed;
}

bool ast_to_bytecode::isFunctionKnown(const std::string &name) {
    return _code->functionByName(name) != nullptr;
}




