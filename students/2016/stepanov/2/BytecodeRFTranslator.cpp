#include <dlfcn.h>
#include <cmath>
#include "../../../../include/mathvm.h"
#include "../../../../vm/parser.h"
#include "BytecodeRFTranslator.h"
#include "ScopeData.h"
#include "VmException.h"

mathvm::BytecodeRFTranslator::~BytecodeRFTranslator() {}

mathvm::Status *mathvm::BytecodeRFTranslator::translate(const string &program, mathvm::Code **code) {
    if (code == nullptr) {
        return Status::Error("Code is nullptr");
    }

    Parser parserInstance = Parser();
    Status *parseStatus = parserInstance.parseProgram(program);
    if (parseStatus->isError()) {
        return parseStatus;
    }
    delete parseStatus;

    BytecodeRfVisitor *visitor = new BytecodeRfVisitor();
    Status *result = visitor->runTranslate(*code, parserInstance.top());
    visitor->dumpByteCode(std::cout);

    delete visitor;
    return result;
}


void mathvm::BytecodeRfVisitor::visitForNode(mathvm::ForNode *node) {
    Bytecode *bc = currentSd->containedFunction->bytecode();
    BinaryOpNode *range = node->inExpr()->asBinaryOpNode();
    Label endLabel = Label(bc);
    Label bodyLabel = Label(bc);

    range->left()->visit(this);
    storeVariable(node->var());
    range->right()->visit(this);
    bc->add(BC_DUMP);
    loadVariable(node->var());
    bc->addBranch(BC_IFICMPG, endLabel);
    bc->addBranch(BC_JA, bodyLabel);

    Label continueLabel = bc->currentLabel();
    bc->add(BC_DUMP);
    loadVariable(node->var());
    bc->add(BC_ILOAD1);
    bc->add(BC_IADD);
    bc->add(BC_DUMP);
    storeVariable(node->var());
    bc->addBranch(BC_IFICMPG, endLabel);
    bodyLabel.bind(bc->current(), bc);
    visitBlockNode(node->body());
    bc->addBranch(BC_JA, continueLabel);
    endLabel.bind(bc->current(), bc);
    bc->add(BC_POP);
}

void mathvm::BytecodeRfVisitor::visitPrintNode(PrintNode *node) {
    Bytecode *bc = currentSd->containedFunction->bytecode();
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        switch (currentSd->topType) {
            case VT_DOUBLE:
                bc->add(BC_DPRINT);
                break;
            case VT_INT:
                bc->add(BC_IPRINT);
                break;
            case VT_STRING:
                bc->add(BC_SPRINT);
                break;
            case VT_INVALID:
            case VT_VOID:
            default:
                throw new std::logic_error("print unexpected type");
        }
        currentSd->topType = VT_VOID;
    }
}

void mathvm::BytecodeRfVisitor::visitLoadNode(LoadNode *node) {
    AstVisitor::visitLoadNode(node);
    loadVariable(node->var());
}

void mathvm::BytecodeRfVisitor::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    Bytecode *bc = currentSd->containedFunction->bytecode();

    Label elseLabel = Label(bc);
    Label endLabel = Label(bc);
    bool elseExists = node->elseBlock() != nullptr;

    bc->add(BC_ILOAD0);
    bc->addBranch(BC_IFICMPE, (elseExists) ? elseLabel : endLabel);
    currentSd->topType = VT_VOID;
    visitBlockNode(node->thenBlock());
    if (elseExists) {
        bc->addBranch(BC_JA, endLabel);
        elseLabel.bind(bc->current(), bc);
        visitBlockNode(node->elseBlock());
    }
    endLabel.bind(bc->current(), bc);
}

inline void mathvm::BytecodeRfVisitor::processArithmeticOperation(BinaryOpNode *node) {
    Instruction operationBcInt;
    Instruction operationBcDouble = BC_INVALID;
    switch (node->kind()) {
        case tADD:
            operationBcInt = BC_IADD;
            operationBcDouble = BC_DADD;
            break;
        case tSUB:
            operationBcInt = BC_ISUB;
            operationBcDouble = BC_DSUB;
            break;
        case tMUL:
            operationBcInt = BC_IMUL;
            operationBcDouble = BC_DMUL;
            break;
        case tDIV:
            operationBcInt = BC_IDIV;
            operationBcDouble = BC_DDIV;
            break;
        case tMOD:
            operationBcInt = BC_IMOD;
            break;
        default:
            throw new std::logic_error("unexpected type in arithmetic operation");
    }

    node->left()->visit(this);
    VarType leftType = currentSd->topType;
    node->right()->visit(this);
    VarType rightType = currentSd->topType;
    bool requireSwap = true;

    Instruction targetInstruction;
    VarType resultType;
    switch (leftType) {
        case VT_DOUBLE:
            targetInstruction = operationBcDouble;
            resultType = VT_DOUBLE;
            switch (rightType) {
                case VT_DOUBLE:
                    break;
                case VT_INT:
                    prepareTopType(VT_DOUBLE);
                    break;
                default:
                    throw new std::logic_error("unexpected type in arithmetic operation");
            }
            break;
        case VT_INT:
            switch (rightType) {
                case VT_DOUBLE:
                    requireSwap = false;
                    currentSd->containedFunction->bytecode()->add(BC_SWAP);
                    currentSd->topType = VT_INT;
                    prepareTopType(VT_DOUBLE);
                    resultType = VT_DOUBLE;
                    targetInstruction = operationBcDouble;
                    break;
                case VT_INT:
                    resultType = VT_INT;
                    targetInstruction = operationBcInt;
                    break;
                default:
                    throw new std::logic_error("unexpected type in arithmetic operation");
            }
            break;
        default:
            throw new std::logic_error("unexpected type in arithmetic operation");
    }
    if (requireSwap) {
        currentSd->containedFunction->bytecode()->add(BC_SWAP);
    }
    if (targetInstruction == BC_INVALID) {
        throw new std::logic_error("unexpected type in arithmetic operation");
    }
    currentSd->containedFunction->bytecode()->add(targetInstruction);
    currentSd->topType = resultType;
}

inline void mathvm::BytecodeRfVisitor::processLogicOperation(BinaryOpNode *node) {
    Bytecode *bc = currentSd->containedFunction->bytecode();
    node->left()->visit(this);
    VarType leftType = currentSd->topType;
    if (leftType != VT_INT) {
        throw new std::logic_error("int expected for logic operator");
    }
    Label succLabel = Label(bc);
    Label endLabel = Label(bc);
    switch (node->kind()) {
        case tOR:
            bc->add(BC_ILOAD0);
            bc->addBranch(BC_IFICMPNE, succLabel);
            node->right()->visit(this);
            if (currentSd->topType != VT_INT) {
                throw new std::logic_error("int expected for logic operator");
            }
            bc->add(BC_ILOAD0);
            bc->addBranch(BC_IFICMPNE, succLabel);
            bc->add(BC_ILOAD0);
            bc->addBranch(BC_JA, endLabel);
            succLabel.bind(bc->current(), bc);
            bc->add(BC_ILOAD1);
            endLabel.bind(bc->current(), bc);
            break;
        case tAND:
            bc->add(BC_ILOAD0);
            bc->addBranch(BC_IFICMPE, succLabel);
            node->right()->visit(this);
            if (currentSd->topType != VT_INT) {
                throw new std::logic_error("int expected for logic operator");
            }
            bc->add(BC_ILOAD0);
            bc->addBranch(BC_IFICMPE, succLabel);
            bc->add(BC_ILOAD1);
            bc->addBranch(BC_JA, endLabel);
            succLabel.bind(bc->current(), bc);
            bc->add(BC_ILOAD0);
            endLabel.bind(bc->current(), bc);
            break;
        default:
            throw new std::logic_error("unexpected operator");
    }

}

inline void mathvm::BytecodeRfVisitor::processBitwiseOperation(BinaryOpNode *node) {
    node->left()->visit(this);
    VarType leftType = currentSd->topType;
    if (leftType != VT_INT) {
        throw new std::logic_error("int expected for bitwise operator");
    }
    node->right()->visit(this);
    VarType rightType = currentSd->topType;
    if (rightType != VT_INT) {
        throw new std::logic_error("int expected for bitwise operator");
    }
    switch (node->kind()) {
        case tAAND:
            currentSd->containedFunction->bytecode()->add(BC_IAAND);
            break;
        case tAXOR:
            currentSd->containedFunction->bytecode()->add(BC_IAXOR);
            break;
        case tAOR:
            currentSd->containedFunction->bytecode()->add(BC_IAOR);
            break;
        default:
            throw new std::logic_error("unexpected type in arithmetic operation");
    }
    currentSd->topType = VT_INT;
}

inline void mathvm::BytecodeRfVisitor::processComparisonOperation(BinaryOpNode *node) {
    node->left()->visit(this);
    VarType leftType = currentSd->topType;
    node->right()->visit(this);
    VarType rightType = currentSd->topType;
    Bytecode *bc = currentSd->containedFunction->bytecode();

    bool isIntComparison = true;
    switch (leftType) {
        case VT_DOUBLE:
            isIntComparison = false;
            switch (rightType) {
                case VT_DOUBLE:
                    break;
                case VT_INT:
                    prepareTopType(VT_DOUBLE);
                    break;
                default:
                    throw new std::logic_error("unexpected type in comparison operation");
            }
            break;
        case VT_INT:
            switch (rightType) {
                case VT_DOUBLE:
                    isIntComparison = false;
                    currentSd->containedFunction->bytecode()->add(BC_SWAP);
                    currentSd->topType = VT_INT;
                    prepareTopType(VT_DOUBLE);
                    currentSd->containedFunction->bytecode()->add(BC_SWAP);
                    break;
                case VT_INT:
                    break;
                default:
                    throw new std::logic_error("unexpected type in comparison operation");
            }
            break;
        default:
            throw new std::logic_error("unexpected type in comparison operation");
    }

    if (isIntComparison) {
        bc->add(BC_ICMP);
    } else {
        bc->add(BC_DCMP);
    }

    Label failLabel = Label(bc);
    Label continueLabel = Label(bc);
    bc->add(BC_ILOAD0);
    switch (node->kind()) {
        case tEQ:
            bc->addBranch(BC_IFICMPNE, failLabel);
            break;
        case tNEQ:
            bc->addBranch(BC_IFICMPE, failLabel);
            break;
        case tLT:
            bc->addBranch(BC_IFICMPGE, failLabel);
            break;
        case tLE:
            bc->addBranch(BC_IFICMPG, failLabel);
            break;
        case tGT:
            bc->addBranch(BC_IFICMPLE, failLabel);
            break;
        case tGE:
            bc->addBranch(BC_IFICMPL, failLabel);
            break;
        default:
            throw new std::logic_error("unexpected operator");
    }

    bc->add(BC_ILOAD1);
    bc->addBranch(BC_JA, continueLabel);
    failLabel.bind(bc->current(), bc);
    bc->add(BC_ILOAD0);
    continueLabel.bind(bc->current(), bc);
    currentSd->topType = VT_INT;
}

void mathvm::BytecodeRfVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    TokenKind operationKind = node->kind();
    switch (operationKind) {
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            processArithmeticOperation(node);
            break;

        case tOR:
        case tAND:
            processLogicOperation(node);
            break;

        case tAOR:
        case tAAND:
        case tAXOR:
            processBitwiseOperation(node);
            break;

        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            processComparisonOperation(node);
            break;

        default:
            throw new std::logic_error("unexpected operator");
    }
}

void mathvm::BytecodeRfVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    Bytecode *bc = currentSd->containedFunction->bytecode();
    if (fabs(node->literal()) < 1E-10) {
        bc->add(BC_DLOAD0);
    } else if (fabs(node->literal() - 1) < 1E-10) {
        bc->add(BC_DLOAD1);
    } else if (fabs(node->literal() + 1) < 1E-10) {
        bc->add(BC_DLOADM1);
    } else {
        bc->add(BC_DLOAD);
        bc->addDouble(node->literal());
    }
    currentSd->topType = VT_DOUBLE;
}

void mathvm::BytecodeRfVisitor::visitStoreNode(StoreNode *node) {
    Bytecode *bc = currentSd->containedFunction->bytecode();
    node->value()->visit(this);
    prepareTopType(node->var()->type());


    switch (node->op()) {
        case tINCRSET:
            loadVariable(node->var());
            switch (node->var()->type()) {
                case VT_DOUBLE:
                    bc->add(BC_DADD);
                    break;
                case VT_INT:
                    bc->add(BC_IADD);
                    break;
                default:
                    assert(false);
            }
            break;
        case tDECRSET:
            loadVariable(node->var());
            switch (node->var()->type()) {
                case VT_DOUBLE:
                    bc->add(BC_DSUB);
                    break;
                case VT_INT:
                    bc->add(BC_ISUB);
                    break;
                default:
                    assert(false);
            }
            break;
        case tASSIGN:
            break;
        default:
            throw new std::logic_error("unexpected save action");
    }

    storeVariable(node->var());
}

void mathvm::BytecodeRfVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    uint16_t stringId = _code->makeStringConstant(node->literal());
    Bytecode *bc = currentSd->containedFunction->bytecode();
    bc->add(BC_SLOAD);
    bc->addUInt16(stringId);
    currentSd->topType = VT_STRING;
}

void mathvm::BytecodeRfVisitor::visitWhileNode(WhileNode *node) {
    Bytecode *bc = currentSd->containedFunction->bytecode();
    Label endLabel = Label(bc);
    Label startLabel = bc->currentLabel();
    node->whileExpr()->visit(this);
    bc->add(BC_ILOAD0);
    currentSd->topType = VT_INT;
    bc->addBranch(BC_IFICMPE, endLabel);
    visitBlockNode(node->loopBlock());
    bc->addBranch(BC_JA, startLabel);
    endLabel.bind(bc->current(), bc);
}

void mathvm::BytecodeRfVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    Bytecode *bc = currentSd->containedFunction->bytecode();
    switch (node->literal()) {
        case 1LL:
            bc->add(BC_ILOAD1);
            break;
        case 0LL:
            bc->add(BC_ILOAD0);
            break;
        case -1LL:
            bc->add(BC_ILOADM1);
            break;
        default:
            bc->add(BC_ILOAD);
            bc->addInt64(node->literal());
    }
    currentSd->topType = VT_INT;
}

void mathvm::BytecodeRfVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
    VarType type = currentSd->topType;

    Bytecode *bc = currentSd->containedFunction->bytecode();
    Label failLabel = Label(bc);
    Label continueLabel = Label(bc);

    switch (node->kind()) {
        case tNOT:
            if (type != VT_INT) {
                throw new VmException("unexpected type for not operator", node->position());
            }
            bc->add(BC_ILOAD0);
            bc->addBranch(BC_IFICMPNE, failLabel);
            bc->add(BC_ILOAD1);
            bc->addBranch(BC_JA, continueLabel);
            failLabel.bind(bc->current(), bc);
            bc->add(BC_ILOAD0);
            continueLabel.bind(bc->current(), bc);
            break;
        case tADD:
            break;
        case tSUB:
            switch (type) {
                case VT_DOUBLE:
                    bc->add(BC_DNEG);
                    break;
                case VT_INT:
                    bc->add(BC_INEG);
                    break;
                default:
                    throw new VmException("unexpected type for negative operator", node->position());
            }
            break;
        default:
            throw new VmException("unexpected unary operator", node->position());
    }
}

void mathvm::BytecodeRfVisitor::visitNativeCallNode(NativeCallNode *node) {
    void *exportedFunction = dlsym(RTLD_DEFAULT, node->nativeName().c_str());

    if (exportedFunction == nullptr) {
        throw new VmException(("cannot find native call for " + node->nativeName()).c_str(), node->position());
    }

    uint16_t nativeId = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), exportedFunction);
    Bytecode *bc = currentSd->containedFunction->bytecode();

    for (uint32_t i = node->nativeSignature().size(); i >= 2; i--) {
        const VariableRF *variableInfo = currentSd->lookupVariableInfo(
                currentSd->containedFunction->parameterName(i - 2));
        loadVariableByInfo(variableInfo, currentSd->containedFunction->parameterType(i - 2));
        prepareTopType(node->nativeSignature().operator[](i - 1).first);
    }

    bc->add(BC_CALLNATIVE);
    bc->addUInt16(nativeId);
    currentSd->topType = node->nativeSignature().front().first;
}

void mathvm::BytecodeRfVisitor::visitBlockNode(BlockNode *node) {
    ScopeData *old_sd = currentSd;
    currentSd = new ScopeData(currentSd);
    scopeEvaluator(node->scope());
    for (uint32_t i = 0; i < node->nodes(); i++) {
        node->nodeAt(i)->visit(this);
    }
    old_sd->updateNestedStack(currentSd->getCountVariablesInScope());
    old_sd->topType = currentSd->topType;
    delete currentSd;
    currentSd = old_sd;
}

void mathvm::BytecodeRfVisitor::visitFunctionNode(FunctionNode *node) {
    node->body()->visit(this);
}

void mathvm::BytecodeRfVisitor::visitReturnNode(ReturnNode *node) {
    Bytecode *bc = currentSd->containedFunction->bytecode();
    if (node->returnExpr() != nullptr) {
        node->returnExpr()->visit(this);
    }
    const VarType expectedType = currentSd->containedFunction->returnType();
    if (expectedType != currentSd->topType) {
        prepareTopType(expectedType);
        currentSd->topType = expectedType;
    }
    bc->add(BC_RETURN);
}

void mathvm::BytecodeRfVisitor::visitCallNode(CallNode *node) {
    BytecodeFunction *bf = currentSd->lookupFunctionByName(node->name());
    Bytecode *bc = currentSd->containedFunction->bytecode();
    if (bf == nullptr) {
        throw new std::logic_error("cannot found target call function");
    }
    if (bf->parametersNumber() != node->parametersNumber()) {
        throw new std::logic_error("incorrect count of arguments for function call");
    }

    for (uint32_t i = node->parametersNumber(); i >= 1; i--) {
        node->parameterAt(i - 1)->visit(this);
        prepareTopType(bf->parameterType(i - 1));
    }

    bc->add(BC_CALL);
    bc->addUInt16(bf->id());
    currentSd->topType = bf->returnType();
}

mathvm::BytecodeRfVisitor::~BytecodeRfVisitor() {

}

mathvm::BytecodeRfVisitor::BytecodeRfVisitor() {
}

mathvm::Status *mathvm::BytecodeRfVisitor::runTranslate(Code *code, AstFunction *function) {
    try {
        BytecodeFunction *currentFunction = new BytecodeFunction(function);
        currentSd = new ScopeData(currentFunction);
        _code = code;
        _code->addFunction(currentFunction);
        currentSd->addFunction(currentFunction);

        translateFunction(function);
    }
    catch (mathvm::VmException *ex) {
        return Status::Error(ex->what(), ex->getPosition());
    }
    catch (std::exception *ex) {
        return Status::Error(ex->what());
    }
    return Status::Ok();
}

void mathvm::BytecodeRfVisitor::translateFunction(AstFunction *function) {
    BytecodeFunction *currentFunctionBc = currentSd->lookupFunctionByName(function->name());
    if (currentFunctionBc == nullptr) {
        throw new std::logic_error("Cannot find finction " + function->name());
    }
    ScopeData *old_sd = currentSd;
    currentSd = new ScopeData(currentFunctionBc, old_sd);

    for (uint32_t currentParameter = 0; currentParameter < function->parametersNumber(); ++currentParameter) {
        AstVar *variable = function->scope()->lookupVariable(function->parameterName(currentParameter), false);
        currentSd->addVariable(variable);
        storeVariable(variable);
    }

    function->node()->visit(this);

    if (function->returnType() == VT_VOID) {
        currentFunctionBc->bytecode()->add(BC_RETURN);
    }

    currentFunctionBc->setLocalsNumber(currentSd->getCountVariablesInScope());
    currentFunctionBc->setScopeId(currentSd->scope_id);
    old_sd->topType = currentSd->topType;
    delete currentSd;
    currentSd = old_sd;
}

void mathvm::BytecodeRfVisitor::storeVariable(const AstVar *var) {
    Instruction localScopeInstruction;
    Instruction fastScopeInstruction;
    Instruction totalScopeInstruction;


    switch (var->type()) {
        case VT_DOUBLE:
            localScopeInstruction = BC_STOREDVAR;
            totalScopeInstruction = BC_STORECTXDVAR;
            fastScopeInstruction = BC_STOREDVAR0;
            break;
        case VT_INT:
            localScopeInstruction = BC_STOREIVAR;
            totalScopeInstruction = BC_STORECTXIVAR;
            fastScopeInstruction = BC_STOREIVAR0;
            break;
        case VT_STRING:
            localScopeInstruction = BC_STORESVAR;
            totalScopeInstruction = BC_STORECTXSVAR;
            fastScopeInstruction = BC_STORESVAR0;
            break;
        case VT_VOID:
        case VT_INVALID:
        default:
            throw new std::logic_error("unexpected type");
    }

    const VariableRF *variableInfo = currentSd->lookupVariableInfo(var->name());
    if (variableInfo == nullptr) {
        throw new std::logic_error("Cannot find variable " + var->name());
    }

    Bytecode *bc = currentSd->containedFunction->bytecode();
    if (currentSd->scope_id == variableInfo->variable_scope) {
        if (variableInfo->variable_id <= 3) {
            bc->add(fastScopeInstruction + variableInfo->variable_id);
        } else {
            bc->add(localScopeInstruction);
            bc->addUInt16(variableInfo->variable_id);
        }
    } else {
        bc->add(totalScopeInstruction);
        bc->addUInt16(variableInfo->variable_scope);
        bc->addUInt16(variableInfo->variable_id);
    }
    currentSd->topType = VT_VOID;
}


void mathvm::BytecodeRfVisitor::loadVariable(AstVar const *var) {
    const VariableRF *variableInfo = currentSd->lookupVariableInfo(var->name());
    loadVariableByInfo(variableInfo, var->type());
}

void mathvm::BytecodeRfVisitor::loadVariableByInfo(const VariableRF *variableInfo, VarType type) {
    Instruction localScopeInstruction;
    Instruction fastScopeInstruction;
    Instruction totalScopeInstruction;
    switch (type) {
        case VT_DOUBLE:
            localScopeInstruction = BC_LOADDVAR;
            totalScopeInstruction = BC_LOADCTXDVAR;
            fastScopeInstruction = BC_LOADDVAR0;
            break;
        case VT_INT:
            localScopeInstruction = BC_LOADIVAR;
            totalScopeInstruction = BC_LOADCTXIVAR;
            fastScopeInstruction = BC_LOADIVAR0;
            break;
        case VT_STRING:
            localScopeInstruction = BC_LOADSVAR;
            totalScopeInstruction = BC_LOADCTXSVAR;
            fastScopeInstruction = BC_LOADSVAR0;
            break;
        case VT_VOID:
        case VT_INVALID:
        default:
            throw new std::logic_error("unexpected type");
    }


    Bytecode *bc = currentSd->containedFunction->bytecode();
    if (currentSd->scope_id == variableInfo->variable_scope) {
        if (variableInfo->variable_id <= 3) {
            bc->add(fastScopeInstruction + variableInfo->variable_id);
        } else {
            bc->add(localScopeInstruction);
            bc->addUInt16(variableInfo->variable_id);
        }
    } else {
        bc->add(totalScopeInstruction);
        bc->addUInt16(variableInfo->variable_scope);
        bc->addUInt16(variableInfo->variable_id);
    }
    currentSd->topType = type;
}

void mathvm::BytecodeRfVisitor::scopeEvaluator(Scope *scope) {
    Scope::VarIterator it = Scope::VarIterator(scope);
    while (it.hasNext()) {
        AstVar *next = it.next();
        currentSd->addVariable(next);
    }
    Scope::FunctionIterator itFun = Scope::FunctionIterator(scope);
    while (itFun.hasNext()) {
        AstFunction *next = itFun.next();
        BytecodeFunction *currentByteFunction = currentSd->lookupFunctionByName(next->name(), false);
        if (currentByteFunction == nullptr) {
            currentByteFunction = new BytecodeFunction(next);
            _code->addFunction(currentByteFunction);
            currentSd->addFunction(currentByteFunction);
        }
    }
    //only after index all names
    itFun = Scope::FunctionIterator(scope);
    while (itFun.hasNext()) {
        AstFunction *next = itFun.next();
        translateFunction(next);
    }
}

void mathvm::BytecodeRfVisitor::prepareTopType(VarType param) {
    switch (currentSd->topType) {
        case VT_DOUBLE:
            switch (param) {
                case VT_DOUBLE:
                    break;
                case VT_INT:
                    currentSd->containedFunction->bytecode()->add(BC_D2I);
                    break;
                default:
                    throw new std::logic_error("incorrect cast");
            }
            break;
        case VT_INT:
            switch (param) {
                case VT_DOUBLE:
                    currentSd->containedFunction->bytecode()->add(BC_I2D);
                    break;
                case VT_INT:
                    break;
                default:
                    throw new std::logic_error("incorrect cast");
            }
            break;
        case VT_STRING:
            switch (param) {
                case VT_STRING:
                    break;
                case VT_INT:
                    currentSd->containedFunction->bytecode()->add(BC_S2I);
                    break;
                default:
                    throw new std::logic_error("incorrect cast to string");
            }
            break;
        default:
            throw new std::logic_error("incorrect cast");
    }
    currentSd->topType = param;
}

void mathvm::BytecodeRfVisitor::dumpByteCode(ostream &out) {
    _code->disassemble(out);
}


mathvm::Status *mathvm::BytecodeTranslatorImpl::translate(const string &program, mathvm::Code **code) {
    return nullptr;
}




