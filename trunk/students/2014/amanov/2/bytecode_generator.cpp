#include "bytecode_generator.h"
#include <dlfcn.h>

namespace mathvm {


void BytecodeGenerator::addInsn(Instruction insn)
{
    bytecode()->addInsn(insn);
}

void BytecodeGenerator::addIntOperandsBinaryInsn(AstNode *left, AstNode *right, Instruction insn)
{
    left->visit(this);
    convOperandIfNeed(VT_INT);
    assertTosType(VT_INT);
    right->visit(this);
    convOperandIfNeed(VT_INT);
    assertTosType(VT_INT);
    addInsn(insn);
    setTosOperandType(VT_INT);
}

void BytecodeGenerator::addBinaryConvertibleInsn(AstNode *right, AstNode *left, Instruction insn)
{
    right->visit(this);
    VarType rightOperandType = tosOperandType();
    left->visit(this);
    assertTypeIsNumeric(rightOperandType);
    assertTypeIsNumeric(tosOperandType());
    if (rightOperandType == VT_DOUBLE)
        convOperandIfNeed(VT_DOUBLE);
    else if (isTosOperandType(VT_DOUBLE) && rightOperandType == VT_INT) {
        addInsn(BC_SWAP);
        addInsn(BC_I2D);
        addInsn(BC_SWAP);
        setTosOperandType(VT_DOUBLE);
    }
    if (isTosOperandType(VT_INT))
        insn = (Instruction) (insn + 1);
    addInsn(insn);
}

void BytecodeGenerator::addArithmeticBinaryOp(AstNode *left, AstNode *right, TokenKind op)
{
    Instruction insn = BC_INVALID;
    switch (op) {
        case tADD : insn = BC_DADD;
            break;
        case tSUB : insn = BC_DSUB;
            break;
        case tMUL : insn = BC_DMUL;
            break;
        case tDIV : insn = BC_DDIV;
            break;
        default : break;
    }
    assert(insn != BC_INVALID);
    addBinaryConvertibleInsn(right, left, insn);
}

void BytecodeGenerator::error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    verror(0, format, args);
}

void BytecodeGenerator::convOperandIfNeed(VarType typeToConverse)
{
    if (isTosOperandType(typeToConverse) || !isNumeric(tosOperandType())
            || !isNumeric(typeToConverse))
        return;
    Instruction convInsn = (typeToConverse == VT_INT) ? BC_D2I : BC_I2D;
    addInsn(convInsn);
    setTosOperandType(typeToConverse);
}

void BytecodeGenerator::addCompareBinaryOp(AstNode *left, AstNode *right, TokenKind op)
{
    Instruction insn = BC_INVALID;
    switch (op) {
        case tEQ : insn = BC_IFICMPE;
            break;
        case tNEQ : insn = BC_IFICMPNE;
                break;
        case tGT : insn = BC_IFICMPG;
            break;
        case tGE : insn = BC_IFICMPGE;
            break;
        case tLT : insn = BC_IFICMPL;
            break;
        case tLE : insn = BC_IFICMPLE;
            break;
        default : break;
    }
    assert(insn != BC_INVALID);
    addBinaryConvertibleInsn(right, left, BC_DCMP);
    setTosOperandType(VT_INT);
    addInsn(BC_ILOAD0);
    addInsn(BC_SWAP);
    Label l1 = label();
    addBranch(insn, l1);
    addInsn(BC_ILOAD0); //false
    Label l2 = label();
    addBranch(BC_JA, l2);
    bind(l1);
    addInsn(BC_ILOAD1);
    bind(l2);
    setTosOperandType(VT_INT);
}


void BytecodeGenerator::assertTosType(VarType tosType)
{
    if (!isTosOperandType(tosType))
        error("invalid operand type : %s , expect %s", typeToName(tosOperandType()), typeToName(tosType));
}

void BytecodeGenerator::addIntegerBinaryOp(AstNode *left, AstNode *right, TokenKind op)
{
    Instruction insn = BC_INVALID;
    switch (op) {
        case tAOR : insn = BC_IAOR;
            break;
        case tAAND : insn = BC_IAAND;
            break;
        case tAXOR : insn = BC_IAXOR;
            break;
        case tMOD : insn = BC_IMOD;
            break;
        default : break;
    }
    assert(insn != BC_INVALID);
    addIntOperandsBinaryInsn(left, right, insn);
}

void BytecodeGenerator::addLogicalBinaryOp(AstNode *left, AstNode *right, TokenKind op)
{
    Instruction compIns = (op == tAND) ? BC_IFICMPNE : BC_IFICMPE;
    Instruction ins1 = BC_ILOAD0;
    Instruction ins2 = BC_ILOAD1;
    if (op != tAND)
        std::swap(ins1, ins2);

    left->visit(this);
    assertTosType(VT_INT);
    addInsn(BC_ILOAD0);
    Label l1 = label();
    addBranch(compIns, l1);
    addInsn(ins1);
    Label l2 = label();
    addBranch(BC_JA, l2);
    bind(l1);
    right->visit(this);
    assertTosType(VT_INT);
    addInsn(BC_ILOAD0);
    Label l3 = label();
    addBranch(compIns, l3);
    addInsn(ins1);
    addBranch(BC_JA, l2);
    bind(l3);
    addInsn(ins2);
    bind(l2);
    setTosOperandType(VT_INT);
}

void BytecodeGenerator::addLogicalNot(AstNode *operand)
{
    operand->visit(this);
    assertTosType(VT_INT);
    addInsn(BC_ILOAD0);
    Label l1 = label();
    addBranch(BC_IFICMPE, l1);
    addInsn(BC_ILOAD0);
    Label l2 = label();
    addBranch(BC_JA, l2);
    bind(l1);
    addInsn(BC_ILOAD1);
    bind(l2);
    setTosOperandType(VT_INT);
}

void BytecodeGenerator::pushContext(AstFunction *function, BytecodeFunction *translatedFun)
{
    ContextHandler *contextHandler = new ContextHandler(function, translatedFun->bytecode(), translatedFun->id(), m_currentContextHandler);
    m_currentContextHandler = contextHandler;
}

void BytecodeGenerator::popContext()
{
    assert(m_currentContextHandler);
    ContextHandler *contextHandler = m_currentContextHandler;
    m_currentContextHandler = contextHandler->parent();
    delete contextHandler;
}

void BytecodeGenerator::translateFunction(AstFunction *function)
{
    BytecodeFunction *translatedFun = (BytecodeFunction *) m_code->functionByName(function->name());
    pushContext(function, translatedFun);
    if (!m_currentContextHandler->pushScope(function->scope()))
        error("Locals number limit");
    function->node()->body()->visit(this);
    translatedFun->setLocalsNumber(m_currentContextHandler->getContextSize());
    if (function->name() == AstFunction::top_name)
        addInsn(BC_STOP);
    m_currentContextHandler->popScope();
    popContext();
}

void BytecodeGenerator::declareFunctions(Scope *scope)
{
    Scope::FunctionIterator it = Scope::FunctionIterator(scope);
    while (it.hasNext()) {
        AstFunction *function = it.next();
        BytecodeFunction *translatedFunction = new BytecodeFunction(function);
        m_code->addFunction(translatedFunction);
    }
    for (uint32_t i = 0; i < scope->childScopeNumber(); i++) {
        Scope *s = scope->childScopeAt(i);
        declareFunctions(s);
    }
}

void BytecodeGenerator::assertTypeIsNumeric(VarType type)
{
    if (!isNumeric(type))
        error("invalid type %s, expect numeric type", typeToName(type));
}

Status *BytecodeGenerator::generate(AstFunction *top)
{
    try {
        declareFunctions(top->scope());
        translateFunction(top);
    } catch (ErrorInfoHolder* error) {
        return Status::Error(error->getMessage(), error->getPosition());
    }
    return Status::Ok();
}



void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode *node)
{
    TokenKind opType = node->kind();
    AstNode *left = node->left();
    AstNode *right = node->right();

    if (opType >= tADD && opType <= tDIV) {
        addArithmeticBinaryOp(left, right, opType);
        return;
    }

    if (opType == tAAND || opType == tAOR || opType == tAXOR || opType == tMOD) {
        addIntegerBinaryOp(left, right, opType);
        return;
    }

    if (opType >= tEQ && opType <= tLE) {
        addCompareBinaryOp(left, right, opType);
        return;
    }


    if (opType == tAND || opType == tOR) {
        addLogicalBinaryOp(left, right, opType);
        return;
    }

    if (opType == tRANGE) {
        error("Incorrect use of range operator ( .. )");
        return;
    }

    assert(false);
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode *node)
{
    if (node->kind() == tNOT) {
        addLogicalNot(node->operand());
    } else if (node->kind() == tSUB){
        node->operand()->visit(this);
        assertTypeIsNumeric(tosOperandType());
        Instruction insn = (isTosOperandType(VT_DOUBLE)) ? BC_DNEG : BC_INEG;
        addInsn(insn);
    }
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode *node)
{
    if (node->literal().empty()) {
        addInsn(BC_SLOAD0);
    } else {
        uint16_t id = m_code->makeStringConstant(node->literal());
        addInsn(BC_SLOAD);
        bytecode()->addUInt16(id);
    }
    setTosOperandType(VT_STRING);
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
    if (node->literal() == 0.)
        addInsn(BC_DLOAD0);
    else if (node->literal() == 1.)
        addInsn(BC_DLOAD1);
    else if (node->literal() == -1.)
        addInsn(BC_DLOADM1);
    else {
        addInsn(BC_DLOAD);
        bytecode()->addDouble(node->literal());
    }
    setTosOperandType(VT_DOUBLE);
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode *node)
{
    switch (node->literal()) {
        case 0: addInsn(BC_ILOAD0);
            break;
        case 1: addInsn(BC_ILOAD1);
            break;
        case -1: addInsn(BC_ILOADM1);
            break;
        default: addInsn(BC_ILOAD);
                 bytecode()->addInt64(node->literal());
            break;
    }

    setTosOperandType(VT_INT);
}

void BytecodeGenerator::visitLoadNode(LoadNode *node)
{
    addLoadVar(node->var());
}

void BytecodeGenerator::addVarOperation(const AstVar *var, bool isLoad)
{
    VarInfo varInfo = m_currentContextHandler->getVarInfo(var->name());
    Instruction insn = BC_INVALID;
    Instruction intInsn[] = {BC_LOADIVAR0, BC_STOREIVAR0, BC_LOADIVAR, BC_STOREIVAR, BC_LOADCTXIVAR, BC_STORECTXIVAR};
    Instruction doubleInsn[] = {BC_LOADDVAR0, BC_STOREDVAR0, BC_LOADDVAR, BC_STOREDVAR, BC_LOADCTXDVAR, BC_STORECTXDVAR};
    Instruction stringInsn[] = {BC_LOADSVAR0, BC_STORESVAR0, BC_LOADSVAR, BC_STORESVAR, BC_LOADCTXSVAR, BC_STORECTXSVAR};
    size_t startIndx = 0;
    if (varInfo.isLocal) {
        startIndx = varInfo.localId <= 3 ? 0 : 2;
    } else {
        startIndx = 4;
    }
    size_t insnIndx = isLoad ? startIndx : startIndx + 1;
    switch (var->type()) {
        case VT_INT: insn = intInsn[insnIndx];
            break;
        case VT_DOUBLE: insn = doubleInsn[insnIndx];
            break;
        case VT_STRING: insn = stringInsn[insnIndx];
            break;
        default:
            break;
    }
    assert(insn != BC_INVALID);
    if (varInfo.isLocal && varInfo.localId <= 3)
        insn = (Instruction) (insn + varInfo.localId);
    if (!isLoad) {
        convOperandIfNeed(var->type());
        assertTosType(var->type());
    }
    addInsn(insn);
    if (!varInfo.isLocal)
        bytecode()->addUInt16(varInfo.contextId);
    if (!(varInfo.isLocal && varInfo.localId <= 3))
        bytecode()->addUInt16(varInfo.localId);
    VarType resultType = isLoad ? var->type() : VT_VOID;
    setTosOperandType(resultType);
}


void BytecodeGenerator::visitStoreNode(StoreNode *node)
{
    node->value()->visit(this);
    VarType varType = node->var()->type();
    convOperandIfNeed(varType);
    assertTosType(varType);
    if (node->op() == tINCRSET) {
        Instruction insn = (varType == VT_DOUBLE) ? BC_DADD : BC_IADD;
        addLoadVar(node->var());
        addInsn(insn);
    } else if (node->op() == tDECRSET) {
        Instruction insn = (varType == VT_DOUBLE) ? BC_DSUB : BC_ISUB;
        addLoadVar(node->var());
        addInsn(insn);
    }
    addStoreVar(node->var());
}

void BytecodeGenerator::visitForNode(ForNode *node)
{
    BinaryOpNode *inExpr =  node->inExpr()->asBinaryOpNode();
    if (!(inExpr && inExpr->kind() == tRANGE)) {
        error("For loop error : invalid range expression");
    }
    if (node->var()->type() != VT_INT) {
        error("Invalid for loop var type");
    }
    AstNode *leftBound = inExpr->left();
    AstNode *rightBound = inExpr->right();
    leftBound->visit(this);
    addStoreVar(node->var());
    Label loop = label();
    Label exit = label();
    bind(loop);
    rightBound->visit(this);
    addLoadVar(node->var());
    addBranch(BC_IFICMPG, exit);
    node->body()->visit(this);
    addLoadVar(node->var());
    addInsn(BC_ILOAD1);
    addInsn(BC_IADD);
    addStoreVar(node->var());
    addBranch(BC_JA, loop);
    bind(exit);
    setTosOperandType(VT_VOID);
}

void BytecodeGenerator::visitWhileNode(WhileNode *node)
{
    Label loop = label();
    Label exit = label();
    bind(loop);
    node->whileExpr()->visit(this);
    assertTosType(VT_INT);
    addInsn(BC_ILOAD0);
    addBranch(BC_IFICMPE, exit);
    node->loopBlock()->visit(this);
    addBranch(BC_JA, loop);
    bind(exit);
    setTosOperandType(VT_VOID);
}

void BytecodeGenerator::visitIfNode(IfNode *node)
{
    node->ifExpr()->visit(this);
    assertTosType(VT_INT);
    addInsn(BC_ILOAD0);
    Label then = label();
    Label exit = label();
    addBranch(BC_IFICMPNE, then);
    if (node->elseBlock())
        node->elseBlock()->visit(this);
    addBranch(BC_JA, exit);
    bind(then);
    node->thenBlock()->visit(this);
    bind(exit);
    setTosOperandType(VT_VOID);
}

void BytecodeGenerator::visitBlockNode(BlockNode *node)
{
    if (!m_currentContextHandler->pushScope(node->scope()))
        error("Locals number limit");
    Scope::FunctionIterator it = Scope::FunctionIterator(node->scope());
    while (it.hasNext()) {
        AstFunction *function = it.next();
        translateFunction(function);
    }

    for (uint32_t i = 0; i < node->nodes(); i++) {
        AstNode *blockNode = node->nodeAt(i);
        if (blockNode->isCallNode())
            m_callFromBlock = true;
        blockNode->visit(this);
    }
    m_currentContextHandler->popScope();
    setTosOperandType(VT_VOID);
}

void BytecodeGenerator::visitFunctionNode(FunctionNode *node)
{
    assert(false);
}

void BytecodeGenerator::visitReturnNode(ReturnNode *node)
{
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
        convOperandIfNeed(currentFunction()->returnType());
    }
    if (!isTosOperandType(currentFunction()->returnType())) {
        error("invalid return type : %s , expect %s", typeToName(tosOperandType()), typeToName(currentFunction()->returnType()));
    }
    addInsn(BC_RETURN);
    setTosOperandType(currentFunction()->returnType());
}

void BytecodeGenerator::visitCallNode(CallNode *node)
{
    AstFunction *function = currentFunction()->node()->body()->scope()->lookupFunction(node->name());
    if (!function) {
        error("Function %s does not exist", node->name().c_str());
    }
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        AstNode *param = node->parameterAt(i);
        param->visit(this);
        convOperandIfNeed(function->parameterType(i));
        if (!isTosOperandType(function->parameterType(i))) {
            error("invalid function paramater type : %s , expect %s", typeToName(tosOperandType()), typeToName(function->parameterType(i)));
        }
    }
    addInsn(BC_CALL);
    TranslatedFunction *fun = m_code->functionByName(node->name());
    uint16_t functionId = fun->id();
    bytecode()->addUInt16(functionId);
    if (m_callFromBlock && fun->returnType() != VT_VOID) {
        addInsn(BC_POP);
    } else
        setTosOperandType(function->returnType());
    m_callFromBlock = false;

}

void BytecodeGenerator::visitNativeCallNode(NativeCallNode *node)
{
    void *code = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    uint16_t id = m_code->makeNativeFunction(node->nativeName(), node->nativeSignature(), code);
    addInsn(BC_CALLNATIVE);
    bytecode()->addUInt16(id);
    setTosOperandType(node->nativeSignature()[0].first);
}

void BytecodeGenerator::visitPrintNode(PrintNode *node)
{
    for (uint32_t i = 0; i < node->operands(); i++) {
        AstNode *operand = node->operandAt(i);
        operand->visit(this);
        Instruction insn = BC_INVALID;
        switch (tosOperandType()) {
            case VT_INT : insn = BC_IPRINT;
                break;
            case VT_DOUBLE : insn = BC_DPRINT;
                break;
            case VT_STRING : insn = BC_SPRINT;
                break;
            default : error("invalid print parameter");
                break;
        }
        addInsn(insn);
        setTosOperandType(VT_VOID);
    }
}



}
