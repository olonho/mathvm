//
// Created by andy on 11/12/16.
//

#include "bytecode_writer.h"

namespace mathvm {

// util
namespace {

bool isNumberType(VarType vt) {
    return vt == VT_INT || vt == VT_DOUBLE;
}

}

void BytecodeWriter::visitBlockNode(BlockNode *node) {
    Scope::VarIterator iter(node->scope());
    uint16_t firstId = _localVariablesOffset;
    _scopesStack.push_back(node->scope());
    while (iter.hasNext()) {
        AstVar *var = iter.next();
        _varDescriptions.insert({var, {firstId++, currentScopeId(), var->type()}});
    }

    Scope::FunctionIterator funcIter(node->scope());
    while (funcIter.hasNext()) {
        AstFunction *func = funcIter.next();
        TranslatedFunction *newFunction = new BytecodeFunction{func};

        _code->addFunction(newFunction);
        func->node()->visit(this);
        newFunction->setScopeId(currentScopeId() + 1);
    }

    for (uint32_t i = 0; i < node->nodes(); i++) {
        node->nodeAt(i)->visit(this);
    }
    _scopesStack.pop_back();
}

void BytecodeWriter::visitCallNode(CallNode *node) {
    AstFunction* f = _scopesStack.back()->lookupFunction(node->name());
    if (!f) {
        throw PositionalException{node->position(), "Usage of undeclare function"};
    }

    TranslatedFunction *func = _code->functionByName(node->name());
    if (!func) {
        throw PositionalException{node->position(), "Usage of undeclare function"};
    }

    _currentCode->addInsn(BC_CALL);
    _currentCode->addUInt16(func->id());

    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        node->parameterAt(i)->visit(this);
    }
}

void BytecodeWriter::visitNativeCallNode(NativeCallNode *node) {
    uint16_t id = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), 0);
    _currentCode->addInsn(BC_CALLNATIVE);
    _currentCode->addUInt16(id);
}

void BytecodeWriter::visitFunctionNode(FunctionNode *node) {
    _localVariablesOffset = node->parametersNumber();
    Bytecode* prevCode = _currentCode;
    BytecodeFunction *func = dynamic_cast<BytecodeFunction*>(_code->functionByName(node->name()));

    if (!func) {
        assert(false);
    }

    _currentCode = func->bytecode();
    visitBlockNode(node->body());
    _currentCode = prevCode;
}

void BytecodeWriter::visitBinaryOpNode(BinaryOpNode *node) {
    switch (node->kind()) {
        case tOR:
        case tAND:
            shortCircuitAndOr(node);
            break;
        case tMOD:
        case tAOR:
        case tAXOR:
        case tAAND:{
            integerBinaryOp(node);
            break;
        }
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            comparison(node);
            break;
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
            numberBinaryOp(node);
            break;
        case tCOMMA:break;
        default:
            throw PositionalException(node->position(), "Unexpected binary operator");
    }
}

void BytecodeWriter::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    _currentCode->addInsn(BC_DLOAD);
    _currentCode->addDouble(node->literal());
}

void BytecodeWriter::visitForNode(ForNode *node) {
    auto varIdIter = _varDescriptions.find(node->var());
    if (varIdIter == _varDescriptions.end()) {
        throw PositionalException(node->position(), "No such variable");
    }

    if (varIdIter->second.type != VT_INT) {
        throw PositionalException{node->position(), "Variabl in for loop must "
                "have integer type"};
    }

    // initialize variable with lower bound
    // should be done even if range is somthing like 100..-1
    loadVar(varIdIter->second);
    BinaryOpNode *inExpr = dynamic_cast<BinaryOpNode*>(node->inExpr());
    if (!inExpr) {
        throw PositionalException{node->position(), "For loop with invalid range expression"};
    }
    inExpr->left()->visit(this);
    storeVar(varIdIter->second);

    // check initial value

    Label finish{_currentCode};

    Label repeat = _currentCode->currentLabel();
    inExpr->right()->visit(this);
    loadVar(varIdIter->second);
    _currentCode->addBranch(BC_IFICMPG, finish);

    node->body()->visit(this);
    _currentCode->addInsn(BC_ILOAD1);
    _currentCode->addInsn(BC_IADD);
    _currentCode->addBranch(BC_JA, repeat);
    _currentCode->bind(finish);
}

void BytecodeWriter::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);

    Label elseLabel;
    Label finish;
    Label &ifNotIf = node->elseBlock() ? elseLabel : finish;
    VarType condType = _typeDeducer.getNodeType(node->ifExpr());
    switch (condType) {
        case VT_INT:
            _currentCode->addInsn(BC_ILOAD0);
            _currentCode->addBranch(BC_IFICMPE, ifNotIf);
            break;
        case VT_DOUBLE:
            _currentCode->addInsn(BC_DLOAD0);
            _currentCode->addBranch(BC_IFICMPE, ifNotIf);
            break;
        case VT_STRING:
            _currentCode->addInsn(BC_SLOAD0);
            _currentCode->addBranch(BC_IFICMPE, ifNotIf);
            break;
        default:
            throw PositionalException{node->position(), "Invalid expression in if statement"};
    }
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        _currentCode->bind(elseLabel);
        node->elseBlock()->visit(this);
    }
    _currentCode->bind(finish);
}

void BytecodeWriter::visitIntLiteralNode(IntLiteralNode *node) {
    _currentCode->addInsn(BC_ILOAD);
    _currentCode->addInt64(node->literal());
}

void BytecodeWriter::visitLoadNode(LoadNode *node) {
    auto varIdIter = _varDescriptions.find(node->var());
    if (varIdIter == _varDescriptions.end()) {
        throw PositionalException(node->position(), "No such variable");
    }
    VarDescription varDesc = varIdIter->second;

    VarType type = _typeDeducer.getNodeType(node);
    if (type == VT_VOID || type == VT_INVALID) {
        throw PositionalException(node->position(), "Can't load such variable type");
    }

    loadVar(varDesc);
}

void BytecodeWriter::visitStoreNode(StoreNode *node) {
    VarType valType = _typeDeducer.getNodeType(node->value());
    auto varIdIter = _varDescriptions.find(node->var());
    if (varIdIter == _varDescriptions.end()) {
        throw PositionalException(node->position(), "Undeclared variable usage");
    }
    VarDescription varDesc = varIdIter->second;

    if ((isNumberType(valType) && !isNumberType(varDesc.type)) ||
        (isNumberType(varDesc.type) && !isNumberType(valType))) {
        throw PositionalException{node->position(), "Value and variable valType are incompatible"};
    }

    if (valType == VT_VOID || valType == VT_INVALID) {
        throw PositionalException(node->position(), "Can't store such variable valType");
    }

    auto loadValWithCast = [&]() {
        node->value()->visit(this);
        if (valType == VT_DOUBLE && varDesc.type == VT_INT) {
            _currentCode->addInsn(BC_D2I);
        } else if (valType == VT_INT && varDesc.type == VT_DOUBLE) {
            _currentCode->addInsn(BC_I2D);
        }
    };

    if (node->op() == tDECRSET || node->op() == tINCRSET) {
        if (varDesc.type == VT_STRING) {
            throw PositionalException{node->position(), "Can't increment or decrement strings"};
        }

        loadValWithCast();
        loadVar(varDesc);

        if (node->op() == tDECRSET) {
            subtract(varDesc.type);
        } else {
            add(varDesc.type);
        }
    } else {
        loadValWithCast();
    }

    storeVar(varDesc);
}

void BytecodeWriter::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); i++) {
        VarType opType = _typeDeducer.getNodeType(node->operandAt(i));

        if (opType == VT_VOID || opType == VT_INVALID) {
            throw PositionalException{node->position(), "Can't print this expression"};
        }
        node->operandAt(i)->visit(this);
        print(opType);
    }
}

void BytecodeWriter::visitReturnNode(ReturnNode *node) {
    node->returnExpr()->visit(this);
    _currentCode->addInsn(BC_RETURN);
}

void BytecodeWriter::visitStringLiteralNode(StringLiteralNode *node) {
    uint16_t id = _code->makeStringConstant(node->literal());

    _currentCode->addInsn(BC_SLOAD);
    _currentCode->addUInt16(id);
}

void BytecodeWriter::visitUnaryOpNode(UnaryOpNode *node) {
    VarType type = _typeDeducer.getNodeType(node->operand());
    if (node->kind() == tSUB) {
        if (type != VT_DOUBLE && type != VT_INT) {
            throw PositionalException{node->position(), "Can't negate this type of expression"};
        }
        node->operand()->visit(this);
        Instruction instr = type == VT_DOUBLE ? BC_DNEG : BC_INEG;
        _currentCode->addInsn(instr);
    } else { // tNOT
        switch (type) {
            case VT_DOUBLE:
                _currentCode->addInsn(BC_D2I);
            case VT_INT: {
                _currentCode->addInsn(BC_ILOAD0);
                Label put1;
                Label finish;
                _currentCode->addBranch(BC_IFICMPE, put1); // goto 1
                _currentCode->addInsn(BC_ILOAD0);
                _currentCode->addBranch(BC_JA, finish);
                _currentCode->bind(put1);
                _currentCode->addInsn(BC_ILOAD1);
                _currentCode->bind(finish);
                break;
            }
            default:
                throw PositionalException{node->position(), "Can't apply not to this type of expression"};
        }
    }
}

void BytecodeWriter::visitWhileNode(WhileNode *node) {
    static const map<VarType, Instruction> loadZero = {
            {VT_INT, BC_ILOAD0},
            {VT_DOUBLE, BC_DLOAD0},
            {VT_STRING, BC_SLOAD0}
    };

    VarType condType = _typeDeducer.getNodeType(node->whileExpr());
    Label checkCond = _currentCode->currentLabel();
    node->whileExpr()->visit(this);
    _currentCode->addInsn(loadZero.at(condType));
    if (condType == VT_DOUBLE) {
        _currentCode->addInsn(BC_D2I);
    }

    Label finish(_currentCode);
    _currentCode->addBranch(BC_IFICMPE, finish);
    node->loopBlock()->visit(this);
    _currentCode->addBranch(BC_JA, checkCond);
    _currentCode->bind(finish);
}

void BytecodeWriter::loadVar(VarDescription varDesc) {
    static std::map<VarType, Instruction> sameContextInstructions =
            {{VT_INT, BC_LOADIVAR}, {VT_DOUBLE, BC_LOADDVAR}, {VT_STRING, BC_LOADSVAR}};
    static std::map<VarType, Instruction> differentContextInstructions =
            {{VT_INT, BC_LOADCTXIVAR}, {VT_DOUBLE, BC_LOADCTXDVAR}, {VT_STRING, BC_LOADCTXSVAR}};

    if (varDesc.scopeId == currentScopeId()) {
        _currentCode->addInsn(sameContextInstructions.at(varDesc.type));
        _currentCode->addUInt16(varDesc.id);
    } else {
        _currentCode->addInsn(differentContextInstructions.at(varDesc.type));
        _currentCode->addUInt16(varDesc.scopeId);
        _currentCode->addUInt16(varDesc.id);
    }
}

void BytecodeWriter::storeVar(VarDescription varDesc) {
    static const std::map<VarType, Instruction> sameContextInstructions =
            {{VT_INT, BC_STOREIVAR}, {VT_DOUBLE, BC_STOREDVAR}, {VT_STRING, BC_STORESVAR}};
    static const std::map<VarType, Instruction> differentContextInstructions =
            {{VT_INT, BC_STORECTXIVAR}, {VT_DOUBLE, BC_STORECTXDVAR}, {VT_STRING, BC_STORECTXSVAR}};


    if (varDesc.scopeId == currentScopeId()) {
        _currentCode->addInsn(sameContextInstructions.at(varDesc.type));
        _currentCode->addUInt16(varDesc.id);
    } else {
        _currentCode->addInsn(differentContextInstructions.at(varDesc.type));
        _currentCode->addUInt16(varDesc.scopeId);
        _currentCode->addUInt16(varDesc.id);
    }
}

void BytecodeWriter::subtract(VarType type) {
    switch (type) {
        case VT_DOUBLE:
            _currentCode->addInsn(BC_DSUB);
            return;
        case VT_INT:
            _currentCode->addInsn(BC_ISUB);
            return;
        default:
            assert(false);
    }
}

void BytecodeWriter::add(VarType type) {
    switch (type) {
        case VT_DOUBLE:
            _currentCode->addInsn(BC_DADD);
            return;
        case VT_INT:
            _currentCode->addInsn(BC_IADD);
            return;
        default:
            assert(false);
    }
}

void BytecodeWriter::print(VarType type) {
    switch (type) {
        case VT_DOUBLE:
            _currentCode->addInsn(BC_DPRINT);
            return;
        case VT_INT:
            _currentCode->addInsn(BC_IPRINT);
            return;
        case VT_STRING:
            _currentCode->addInsn(BC_SPRINT);
            return;
        default:
            assert(false);
    }
}

void BytecodeWriter::shortCircuitAndOr(BinaryOpNode *node) {
    static std::map<VarType, Instruction> loadZeroInstr =
            {{VT_INT, BC_ILOAD0}, {VT_DOUBLE, BC_DLOAD0}, {VT_STRING, BC_SLOAD0}};

    const VarType leftType = _typeDeducer.getNodeType(node->left());
    const VarType rightType = _typeDeducer.getNodeType(node->right());
    const TokenKind kind = node->kind();

    assert(kind == tAND || kind == tOR);

    Instruction shortPathResult = kind == tOR ? BC_ILOAD1 : BC_ILOAD0;
    Instruction longPathResut = kind == tOR ? BC_ILOAD0 : BC_ILOAD1;
    Instruction comparison = kind == tOR ? BC_IFICMPE : BC_IFICMPNE;

    node->left()->visit(this);
    _currentCode->addInsn(loadZeroInstr.at(leftType));

    Label checkSecondArg(_currentCode);
    _currentCode->addBranch(comparison, checkSecondArg);// if first operand is 0, jump

    _currentCode->addInsn(shortPathResult);

    Label finish(_currentCode);
    _currentCode->addBranch(BC_JA, finish); // skip fail branch
    checkSecondArg.bind(_currentCode->current());


    node->right()->visit(this);
    _currentCode->addInsn(loadZeroInstr.at(rightType));

    Label bothArgsLeadToLongPath(_currentCode);
    _currentCode->addBranch(comparison, bothArgsLeadToLongPath); // skip success branch
    _currentCode->addInsn(shortPathResult);
    _currentCode->addBranch(BC_JA, finish); // skip fail branch
    _currentCode->bind(bothArgsLeadToLongPath);
    _currentCode->addInsn(longPathResut);
    _currentCode->bind(finish);
}

// writes code which performs comparison described in node
// comparing int and double causes int to be cast to double
void BytecodeWriter::comparison(BinaryOpNode *node) {
    static const std::map<TokenKind, std::pair<Instruction, Instruction>> finishInstructions = {
            {tEQ,  {BC_ILOAD0,  BC_IFICMPNE}}, // second instr is branch to fail
            {tNEQ, {BC_ILOAD0,  BC_IFICMPE}},
            {tGE,  {BC_ILOADM1, BC_IFICMPE}},
            {tLE,  {BC_ILOAD1,  BC_IFICMPE}},
            {tLT,  {BC_ILOADM1, BC_IFICMPNE}},
            {tGT,  {BC_ILOAD1,  BC_IFICMPNE}}
    };

    const VarType leftType = _typeDeducer.getNodeType(node->left());
    const VarType rightType = _typeDeducer.getNodeType(node->right());
    const TokenKind kind = node->kind();

    assert(finishInstructions.count(kind));

    if ((leftType == VT_STRING && rightType != VT_STRING) ||
        (leftType != VT_STRING && rightType == VT_STRING)) {
        throw PositionalException(node->position(), "Can't compare strings with numbers");
    }

    bool hasDoubleArg = leftType == VT_DOUBLE || rightType == VT_DOUBLE;
    bool convertFirstArg = leftType != VT_DOUBLE && rightType == VT_DOUBLE;
    bool convertSecondArg = leftType == VT_DOUBLE && rightType != VT_DOUBLE;

    // since we compare left ? right, we need to load right first, then left
    node->right()->visit(this);
    if (convertSecondArg) {
        _currentCode->addInsn(BC_I2D);
    }

    node->left()->visit(this);
    if (convertFirstArg) {
        _currentCode->addInsn(BC_I2D);
    }

    Instruction comparator = hasDoubleArg ? BC_DCMP : BC_ICMP;
    _currentCode->addInsn(comparator);

    // now stack top is either -1, 0 or 1
    // finish comparison
    _currentCode->addInsn(finishInstructions.at(kind).first);
    Label fail(_currentCode);
    _currentCode->addBranch(finishInstructions.at(kind).second, fail);
    _currentCode->addInsn(BC_ILOAD1);
    Label success(_currentCode);
    _currentCode->addBranch(BC_JA, success);
    fail.bind(_currentCode->current());
    _currentCode->addInsn(BC_ILOAD0);
    success.bind(_currentCode->current());
}

void BytecodeWriter::integerBinaryOp(BinaryOpNode *node) {
    static const std::map<TokenKind , Instruction> mapping =
            {{tAOR, BC_IAOR}, {tAAND, BC_IAAND}, {tAXOR, BC_IAXOR},
             {tMOD, BC_IMOD}};

    const VarType leftType = _typeDeducer.getNodeType(node->left());
    const VarType rightType = _typeDeducer.getNodeType(node->right());
    const TokenKind kind = node->kind();

    assert(mapping.count(kind));

    node->left()->visit(this);
    node->right()->visit(this);
    if (leftType != VT_INT || rightType != VT_INT) {
        throw PositionalException(node->position(),
                                  string(tokenOp(kind)) + " expects two integer arguments");
    }
    _currentCode->addInsn(mapping.at(kind));
}

void BytecodeWriter::numberBinaryOp(BinaryOpNode *node) {
    static const std::map<TokenKind, std::pair<Instruction, Instruction>> mapping = {
            {tADD, {BC_IADD, BC_DADD}},
            {tSUB, {BC_ISUB, BC_DSUB}},
            {tDIV, {BC_IDIV, BC_DDIV}},
            {tMUL, {BC_IMUL, BC_DMUL}}};

    const VarType leftType = _typeDeducer.getNodeType(node->left());
    const VarType rightType = _typeDeducer.getNodeType(node->right());
    const TokenKind kind = node->kind();

    assert(mapping.count(kind));

    bool hasDoubleArg = leftType == VT_DOUBLE || rightType == VT_DOUBLE;
    bool convertFirstArg = leftType != VT_DOUBLE && rightType == VT_DOUBLE;
    bool convertSecondArg = leftType == VT_DOUBLE && rightType != VT_DOUBLE;

    node->left()->visit(this);
    if (convertFirstArg) {
        _currentCode->addInsn(BC_I2D);
    }
    node->right()->visit(this);
    if (convertSecondArg) {
        _currentCode->addInsn(BC_I2D);
    }

    if (!isNumberType(leftType) || !isNumberType(rightType)) {
        throw PositionalException(node->position(),
                                  string(tokenOp(kind)) + " expects two number arguments");
    }

    Instruction instr = hasDoubleArg ? mapping.at(kind).second : mapping.at(kind).first;

    _currentCode->addInsn(instr);
}

}
