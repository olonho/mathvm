#include "MyTranslator.h"

using namespace mathvm;
using namespace std;

void MyTranslator::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
    VarType expectedType = myNodeTypes[node];
    if (node->kind() == tAND || node->kind() == tOR) {
        Label lEnd(myBytecode);
        Label lFirst(myBytecode);
	
        visitWithTypeControl(node->left(), expectedType);
        myBytecode->add(BC_ILOAD0);
        if (node->kind() == tAND)
            myBytecode->addBranch(BC_IFICMPE, lFirst);
        else
            myBytecode->addBranch(BC_IFICMPNE, lFirst);
	
        visitWithTypeControl(node->right(), expectedType);
        myBytecode->addBranch(BC_JA, lEnd);
        myBytecode->bind(lFirst);

        if (node->kind() == tAND)
            myBytecode->add(BC_ILOAD0);
        else
            myBytecode->add(BC_ILOAD1);

        myBytecode->bind(lEnd);
        return;
    }
    visitWithTypeControl(node->left(), expectedType);
    visitWithTypeControl(node->right(), expectedType);
    if (tryDoArithmetics(node, expectedType))
        return;
    if (expectedType == VT_DOUBLE) {
        tryDoFloatingLogic(node);
    }
    else {
        tryDoIntegerLogic(node);
    }
}

void MyTranslator::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    node->operand()->visit(this);
    VarType operandType = myNodeTypes[node->operand()];
    if (operandType == VT_STRING)
        throw TranslationException("String unary operations not supported");
    switch (node->kind()) {
    case tSUB:
        bytecodeNeg(operandType);
        break;
    case tNOT:
        if (operandType == VT_DOUBLE)
            throw TranslationException("Invalid argument type for NOT command");
        myBytecode->addInsn(BC_ILOAD0);
        doIFICMP(BC_IFICMPE);
    default:
        break;
    }
    myLastNodeType = operandType;
}

void MyTranslator::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
    uint16_t id = myCode.makeStringConstant(node->literal());
    myBytecode->addInsn(BC_SLOAD);
    myBytecode->addInt16(id);
    myLastNodeType = VT_STRING;
}

void MyTranslator::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
    myBytecode->addInsn(BC_DLOAD);
    myBytecode->addDouble(node->literal());
    myLastNodeType = VT_DOUBLE;
}

void MyTranslator::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
    myBytecode->addInsn(BC_ILOAD);
    myBytecode->addInt64(node->literal());
    myLastNodeType = VT_INT;
}

void MyTranslator::visitLoadNode(mathvm::LoadNode* node) {
    bool isClosure = myScopeManager.isClosure(node->var()->name());
    VarId id = myScopeManager.getVariableId(node->var());
    loadVarCommand(node->var()->type(), isClosure, id);
    myLastNodeType = node->var()->type();
}

void MyTranslator::visitStoreNode(mathvm::StoreNode* node) {
    bool isClosure = myScopeManager.isClosure(node->var()->name());
    VarId id = myScopeManager.getVariableId(node->var());
    VarType expectedType = node->var()->type();
    if (node->op() != tASSIGN) {
        if (node->var()->type() == VT_STRING)
            throw TranslationException("Strings can not mutate");
        loadVarCommand(expectedType, isClosure, id);
        visitWithTypeControl(node->value(), expectedType);
        if (node->op() == tDECRSET)
            bytecodeSub(expectedType);
        else if (node->op() == tINCRSET)
            bytecodeAdd(expectedType);
        else
            throw TranslationException((std::string)"Unsupported operation: " + tokenOp(node->op()));
    }
    else {
        visitWithTypeControl(node->value(), expectedType);
    }
    storeVarCommand(node->var()->type(), isClosure, id);
}

void MyTranslator::visitForNode(mathvm::ForNode* node) {
    Label lCheck(myBytecode);
    Label lEnd(myBytecode);
    BinaryOpNode * range = node->inExpr()->asBinaryOpNode();
    if (range == NULL || range->kind() != tRANGE)
        throw TranslationException("Range not specified in for statement");
    if (!myScopeManager.isVarOnStack(node->var()))
        throw TranslationException("Undefined variable " + node->var()->name());
    uint16_t varId = myScopeManager.getVariableId(node->var()).id;

    range->left()->visit(this);
    myBytecode->addInsn(BC_STOREIVAR);
    myBytecode->addInt16(varId);
    myBytecode->bind(lCheck);
    
    myBytecode->addInsn(BC_LOADIVAR);
    myBytecode->addInt16(varId);
    range->right()->visit(this);
    myBytecode->addBranch(BC_IFICMPG, lEnd);

    node->body()->visit(this);

    myBytecode->addInsn(BC_LOADIVAR);
    myBytecode->addInt16(varId);
    myBytecode->addInsn(BC_ILOAD1);
    myBytecode->addInsn(BC_IADD);
    myBytecode->addInsn(BC_STOREIVAR);
    myBytecode->addInt16(varId);

    myBytecode->addBranch(BC_JA, lCheck);
    myBytecode->bind(lEnd);
}

void MyTranslator::visitWhileNode(mathvm::WhileNode* node) {
    Label lEnd(myBytecode);
    Label lCheck(myBytecode);
    myBytecode->bind(lCheck);
    node->whileExpr()->visit(this);
    myBytecode->addInsn(BC_ILOAD0);
    myBytecode->addBranch(BC_IFICMPE, lEnd);
    node->loopBlock()->visit(this);
    myBytecode->addBranch(BC_JA, lCheck);
    myBytecode->bind(lEnd);
}

void MyTranslator::visitIfNode(mathvm::IfNode* node) {
    Label lEnd(myBytecode);
    node->ifExpr()->visit(this);
    myBytecode->addInsn(BC_ILOAD0);
    if (node->elseBlock()) {
        Label lFalse(myBytecode);
        myBytecode->addBranch(BC_IFICMPE, lFalse);
        node->thenBlock()->visit(this);
        myBytecode->addBranch(BC_JA, lEnd);
        myBytecode->bind(lFalse);
        node->elseBlock()->visit(this);
    }
    else {
        myBytecode->addBranch(BC_IFICMPE, lEnd);
        node->thenBlock()->visit(this);
    }
    myBytecode->bind(lEnd);
}

void MyTranslator::visitBlockNode(mathvm::BlockNode* node) {
    myScopeManager.pushScope(node->scope());
    node->visitChildren(this);
    Scope::FunctionIterator it(node->scope());
    while (it.hasNext()) {
        AstFunction* f = it.next();
        f->node()->visit(this);
    }
    myScopeManager.popScope();
}

void MyTranslator::visitFunctionNode(mathvm::FunctionNode* node) {
    Bytecode * prev = myBytecode;
    myBytecode = new Bytecode;
    FunctionScope const * scope = myScopeManager.getFunctionScope(node->name());
    ExtendedBytecodeFunction *bfun = new ExtendedBytecodeFunction(scope);
    myCode.addFunction(bfun);
    node->body()->visit(this);

    if (prev == 0)
        myBytecode->addInsn(BC_STOP);
    *bfun->bytecode() = *myBytecode;

    delete myBytecode;
    myBytecode = prev;
}

void MyTranslator::visitPrintNode(mathvm::PrintNode* node) {
    for (unsigned int i = 0; i < node->operands(); ++i) {
        AstNode* op = node->operandAt(i);
        op->visit(this);
        bytecodePrint(myNodeTypes[op]);
    }
}

void MyTranslator::bytecodeAdd(VarType expectedType) {
    if (expectedType == VT_DOUBLE)
        myBytecode->addInsn(BC_DADD);
    else if (expectedType == VT_INT)
        myBytecode->addInsn(BC_IADD);
    else throw TranslationException("Invalid operation");
}

void MyTranslator::bytecodeSub(mathvm::VarType expectedType) {
    if (expectedType == VT_DOUBLE)
        myBytecode->addInsn(BC_DSUB);
    else if (expectedType == VT_INT)
        myBytecode->addInsn(BC_ISUB);
    else throw TranslationException("Invalid operation");
}

void MyTranslator::bytecodeMul(mathvm::VarType expectedType) {
    if (expectedType == VT_DOUBLE)
        myBytecode->addInsn(BC_DMUL);
    else if (expectedType == VT_INT)
        myBytecode->addInsn(BC_IMUL);
    else throw TranslationException("Invalid operation");
}

void MyTranslator::bytecodeDiv(mathvm::VarType expectedType) {
    if (expectedType == VT_DOUBLE)
        myBytecode->addInsn(BC_DDIV);
    else if (expectedType == VT_INT)
        myBytecode->addInsn(BC_IDIV);
    else throw TranslationException("Invalid operation");
}

void MyTranslator::bytecodeNeg(mathvm::VarType expectedType) {
    if (expectedType == VT_DOUBLE)
        myBytecode->addInsn(BC_DNEG);
    else if (expectedType == VT_INT)
        myBytecode->addInsn(BC_INEG);
    else throw TranslationException("Invalid operation");
}

void MyTranslator::bytecodePrint(mathvm::VarType expectedType) {
    switch (expectedType) {
    case VT_INT:
        myBytecode->addInsn(BC_IPRINT);
        break;
    case VT_DOUBLE:
        myBytecode->addInsn(BC_DPRINT);
        break;
    case VT_STRING:
        myBytecode->addInsn(BC_SPRINT);
    default:
        break;
    }
}

bool MyTranslator::tryDoArithmetics(mathvm::BinaryOpNode * node, mathvm::VarType expectedType) {
    switch (node->kind()) {
    case tADD:
        bytecodeAdd(expectedType);
        return true;
    case tSUB:
        bytecodeSub(expectedType);
        return true;
    case tMUL:
        bytecodeMul(expectedType);
        return true;
    case tDIV:
        bytecodeDiv(expectedType);
        return true;
    default:
        return false;
    }
    return false;
}

bool MyTranslator::tryDoIntegerLogic(mathvm::BinaryOpNode* node) {
    Instruction ifInstruction = BC_INVALID;
    switch (node->kind()) {
    case tEQ:
        ifInstruction = BC_IFICMPE;
        break;
    case tNEQ:
        ifInstruction = BC_IFICMPNE;
        break;
    case tGT:
        ifInstruction = BC_IFICMPG;
        break;
    case tGE:
        ifInstruction = BC_IFICMPGE;
        break;
    case tLT:
        ifInstruction = BC_IFICMPL;
        break;
    case tLE:
        ifInstruction = BC_IFICMPLE;
        break;
    default:
        return false;
    }
    doIFICMP(ifInstruction);
    return true;
}

bool MyTranslator::tryDoFloatingLogic(mathvm::BinaryOpNode* node) {
    myBytecode->addInsn(BC_DCMP);
    switch (node->kind()) {
    case tEQ:
        myBytecode->addInsn(BC_ILOAD0);
        doIFICMP(BC_IFICMPE);
        return true;
    case tNEQ:
        myBytecode->addInsn(BC_ILOAD0);
        doIFICMP(BC_IFICMPNE);
        return true;
    case tGT:
        myBytecode->addInsn(BC_ILOAD1);
        doIFICMP(BC_IFICMPG);
        return true;
    case tGE:
        myBytecode->addInsn(BC_ILOAD1);
        doIFICMP(BC_IFICMPGE);
        return true;
    case tLT:
        myBytecode->addInsn(BC_ILOADM1);
        doIFICMP(BC_IFICMPL);
        return true;
    case tLE:
        myBytecode->addInsn(BC_ILOADM1);
        doIFICMP(BC_IFICMPLE);
        return true;
    default:
        return false;
    }
}

void MyTranslator::doIFICMP(mathvm::Instruction operation) {
    Label lTrue(myBytecode);
    Label lEnd(myBytecode);
    myBytecode->addBranch(operation, lTrue);
    myBytecode->addInsn(BC_ILOAD0);
    myBytecode->addBranch(BC_JA, lEnd);
    myBytecode->bind(lTrue);
    myBytecode->addInsn(BC_ILOAD1);
    myBytecode->bind(lEnd);
}

void MyTranslator::loadVar(mathvm::AstVar const * var) {
    if (!myScopeManager.isVarOnStack(var))
        throw TranslationException("Undefined variable " + var->name());
    uint16_t varId = myScopeManager.getVariableId(var).id;
    if (var->type() == VT_STRING) {
        myBytecode->addInsn(BC_LOADSVAR);
    }
    else if (var->type() == VT_INT) {
        myBytecode->addInsn(BC_LOADIVAR);
    }
    else if (var->type() == VT_DOUBLE) {
        myBytecode->addInsn(BC_LOADDVAR);
    }
    else {
        throw TranslationException("Unable to load variable: unsupported type");
    }
    myBytecode->addInt16(varId);
}

void MyTranslator::storeVar(mathvm::AstVar const * var) {
    if (!myScopeManager.isVarOnStack(var))
        throw TranslationException("Undefined variable " + var->name());
    uint16_t varId = myScopeManager.getVariableId(var).id;
    if (var->type() == VT_STRING) {
        myBytecode->addInsn(BC_STORESVAR);
    }
    else if (var->type() == VT_INT) {
        myBytecode->addInsn(BC_STOREIVAR);
    }
    else if (var->type() == VT_DOUBLE) {
        myBytecode->addInsn(BC_STOREDVAR);
    }
    else {
        throw TranslationException("Unable to store variable: unsupported type");
    }
    myBytecode->addInt16(varId);
}

void MyTranslator::visitWithTypeControl(AstNode* node, mathvm::VarType expectedType) {
    node->visit(this);
    if (myLastNodeType == VT_DOUBLE && expectedType == VT_INT)
        myBytecode->addInsn(BC_D2I);
    if (myLastNodeType == VT_INT && expectedType == VT_DOUBLE)
        myBytecode->addInsn(BC_I2D);
}

mathvm::Code* MyTranslator::getCode() {
    return &myCode;
}

void MyTranslator::translate(mathvm::AstFunction * main) {
    TranslatorVisitor translatorVisitor;
    translatorVisitor.visit(main);
    myNodeTypes = translatorVisitor.myNodeTypes;
    myScopeManager = translatorVisitor.myScopeManager;
    main->node()->visit(this);
}

void MyTranslator::visitReturnNode(mathvm::ReturnNode* node) {
    if (node->returnExpr())
        visitWithTypeControl(node->returnExpr(), myNodeTypes[node]);
    myBytecode->addInsn(BC_RETURN);
}

void MyTranslator::visitCallNode(mathvm::CallNode* node) {
    for (unsigned int i = 0; i < node->parametersNumber(); ++i) {
        AstNode* n = node->parameterAt(i);
        visitWithTypeControl(n, myNodeTypes[n]);
    }
    myBytecode->addInsn(BC_CALL);
    uint16_t id = myScopeManager.getFunctionId(node->name());
    myBytecode->addInt16(id);
}

void MyTranslator::loadVarCommand(mathvm::VarType variableType, bool isClosure, VarId const& id) {
    Instruction ins;
    switch(variableType) {
    case VT_DOUBLE:
        ins = isClosure ? BC_LOADCTXDVAR : BC_LOADDVAR;
        break;
    case VT_STRING:
        ins = isClosure ? BC_LOADCTXSVAR : BC_LOADSVAR;
        break;
    case VT_INT:
        ins = isClosure ? BC_LOADCTXIVAR : BC_LOADIVAR;
        break;
    default:
        throw TranslationException("Invalid variable type");
    }
    myBytecode->addInsn(ins);
    if (isClosure)
        myBytecode->addUInt16(id.ownerFunction);
    myBytecode->addUInt16(id.id);
}

void MyTranslator::storeVarCommand(mathvm::VarType variableType, bool isClosure, VarId id) {
    Instruction ins;
    switch(variableType) {
    case VT_DOUBLE:
        ins = isClosure ? BC_STORECTXDVAR : BC_STOREDVAR;
        break;
    case VT_STRING:
        ins = isClosure ? BC_STORECTXSVAR : BC_STORESVAR;
        break;
    case VT_INT:
        ins = isClosure ? BC_STORECTXIVAR : BC_STOREIVAR;
        break;
    default:
        throw TranslationException("Invalid variable type");
    }
    myBytecode->addInsn(ins);
    if (isClosure)
        myBytecode->addUInt16(id.ownerFunction);
    myBytecode->addUInt16(id.id);
}

MyTranslator::MyTranslator() : myBytecode(NULL) {}

MyTranslator::~MyTranslator() {
    myScopeManager.cleanUp();
}

