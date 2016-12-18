#include "bytecode_translator.h"
#include "translation_exception.h"
#include "helpers.h"
#include <mathvm.h>
#include <parser.h>


namespace mathvm {

namespace details {

BytecodePrinter :: BytecodePrinter() {}

BytecodePrinter :: ~BytecodePrinter() {}

void BytecodePrinter :: visitBinaryOpNode(BinaryOpNode* node) {
    switch (node->kind()) {
        case tAND:
        case tOR: {
            handleLogicOp(node);
            break;
        }

        case tAOR:
        case tAAND:
        case tAXOR: {
            handleBitwiseOp(node);
            break;
        }

        case tADD:
        case tSUB:
        case tMUL:
        case tDIV: {
            handleArithmeticOp(node);
            break;
        }

        case tMOD: {
            handleModuloOp(node);
            break;
        }

        case tEQ:
        case tNEQ:
        case tLT:
        case tGT:
        case tLE:
        case tGE: {
            handleCmp(node);
            break;
        }

        default: {
            throw TranslationException("Unknown binary operation");
            break;
        }
    }
}

void BytecodePrinter :: visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);
    switch (node->kind()) {
        case tSUB: {
            getCurrentBytecode()->addInsn(NEGATE(_topType));
            break;
        }

        case tNOT: {
            getCurrentBytecode()->addInsn(BC_ILOAD1);
            getCurrentBytecode()->addInsn(BC_IAXOR);
            break;
        }

        default: {
            throw TranslationException("Unknown unary operation.");
        }
    }
}

void BytecodePrinter :: visitStringLiteralNode(StringLiteralNode* node) {
    _topType = VT_STRING;
    getCurrentBytecode()->addInsn(BC_SLOAD);
    getCurrentBytecode()->addUInt16(_code->makeStringConstant(node->literal()));
}

void BytecodePrinter :: visitIntLiteralNode(IntLiteralNode* node) {
    _topType = VT_INT;
    getCurrentBytecode()->addInsn(BC_ILOAD);
    getCurrentBytecode()->addInt64(node->literal());
}

void BytecodePrinter :: visitDoubleLiteralNode(DoubleLiteralNode* node) {
    _topType = VT_DOUBLE;
    getCurrentBytecode()->addInsn(BC_DLOAD);
    getCurrentBytecode()->addDouble(node->literal());
}

void BytecodePrinter :: visitLoadNode(LoadNode* node) {
    loadVar(node->var());
}

void BytecodePrinter :: visitStoreNode(StoreNode* node) {
    node->value()->visit(this);
    castTopType(node->var()->type());
    switch (node->op()) {
        case tASSIGN: {
            break;
        }

        case tINCRSET: {
            loadVar(node->var());
            getCurrentBytecode()->addInsn(ADD(_topType));
            break;
        }

        case tDECRSET: {
            loadVar(node->var());
            getCurrentBytecode()->addInsn(SUB(_topType));
            break;
        }

        default: {
            break;
        }
    }
    storeVar(node->var());
}

void BytecodePrinter :: visitBlockNode(BlockNode* node) {
    initVariables(node->scope());
    initFunctions(node->scope());
    for (size_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
    }
}

void BytecodePrinter :: visitNativeCallNode(NativeCallNode* node) {
    //TODO
}

void BytecodePrinter :: visitForNode(ForNode* node) {
    Label loopStart(getCurrentBytecode());
    Label loopFinish(getCurrentBytecode());

    BinaryOpNode* inExpr = node->inExpr()->asBinaryOpNode();
    inExpr->left()->visit(this);
    storeVar(node->var());

    getCurrentBytecode()->bind(loopStart);
    loadVar(node->var());
    inExpr->right()->visit(this);

    getCurrentBytecode()->addBranch(BC_IFICMPL, loopFinish);
    node->body()->visit(this);

    loadVar(node->var());
    getCurrentBytecode()->addInsn(BC_ILOAD1);
    getCurrentBytecode()->addInsn(BC_IADD);
    storeVar(node->var());
    getCurrentBytecode()->addBranch(BC_JA, loopStart);
    getCurrentBytecode()->bind(loopFinish);
}

void BytecodePrinter :: visitWhileNode(WhileNode* node) {
    Label loopStart(getCurrentBytecode());
    Label loopFinish(getCurrentBytecode());
    getCurrentBytecode()->bind(loopStart);

    node->whileExpr()->visit(this);
    getCurrentBytecode()->addInsn(BC_ILOAD0);
    getCurrentBytecode()->addBranch(BC_IFICMPE, loopFinish);

    node->loopBlock()->visit(this);
    getCurrentBytecode()->addBranch(BC_JA, loopStart);
    getCurrentBytecode()->bind(loopFinish);
}

void BytecodePrinter :: visitIfNode(IfNode* node) {
    Label afterThen(getCurrentBytecode());
    node->ifExpr()->visit(this);

    if (node->elseBlock()) {
        Label elseLabel(getCurrentBytecode());

        getCurrentBytecode()->addInsn(BC_ILOAD0);
        getCurrentBytecode()->addBranch(BC_IFICMPE, elseLabel);

        node->thenBlock()->visit(this);

        getCurrentBytecode()->addBranch(BC_JA, afterThen);
        getCurrentBytecode()->bind(elseLabel);
        node->elseBlock()->visit(this);
    } else {
        getCurrentBytecode()->addInsn(BC_ILOAD0);
        getCurrentBytecode()->addBranch(BC_IFICMPE, afterThen);
        node->thenBlock()->visit(this);
    }

    getCurrentBytecode()->bind(afterThen);
}

void BytecodePrinter :: visitReturnNode(ReturnNode* node) {
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
        castTopType(_currentScopeData->function()->returnType());
    }
    getCurrentBytecode()->addInsn(BC_RETURN);
}

void BytecodePrinter :: visitFunctionNode(FunctionNode* node) {
    BytecodeFunction* bytecodeFunction = (BytecodeFunction*)_code->functionByName(node->name());
    ScopeData* functionScopeData = new ScopeData(bytecodeFunction, _currentScopeData);
    _currentScopeData = functionScopeData;

    for(size_t i = 0; i < node->parametersNumber(); ++i) {
        AstVar* var = node->body()->scope()->lookupVariable(node->parameterName(i));
        _currentScopeData->addAstVar(var);
        storeVar(var);
    }

    if (node->body()->nodes() != 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
    } else {
        node->body()->visit(this);
    }

    bytecodeFunction->setLocalsNumber(_currentScopeData->localsCount());
    _currentScopeData = _currentScopeData->parent();
    delete functionScopeData;
}

void BytecodePrinter :: visitCallNode(CallNode* node) {
    BytecodeFunction* bytecodeFunction = (BytecodeFunction*)_code->functionByName(node->name());
    if (!bytecodeFunction) {
        throw TranslationException("Missing function definition.");
    }

    if (bytecodeFunction->parametersNumber() != node->parametersNumber()) {
        throw TranslationException("Wrong number of arguments.");
    }

    for (int i = node->parametersNumber() - 1; i > -1; --i) {
        node->parameterAt(i)->visit(this);
        castTopType(bytecodeFunction->parameterType(i));
    }

    getCurrentBytecode()->addInsn(BC_CALL);
    getCurrentBytecode()->addUInt16(bytecodeFunction->id());

    _topType = (bytecodeFunction->returnType() != VT_VOID) ? bytecodeFunction->returnType() : _topType;
}

void BytecodePrinter :: visitPrintNode(PrintNode* node) {
    for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        getCurrentBytecode()->addInsn(PRINT(_topType));
    }
}

Status* BytecodePrinter :: translateToBytecode(Code* code, AstFunction* top) {
    _code = code;
    BytecodeFunction* topmostBytecodeFunction = new BytecodeFunction(top);
    _code->addFunction(topmostBytecodeFunction);
    _currentScopeData = new ScopeData(topmostBytecodeFunction);

    try {
        Scope* topmostScope = top->scope()->childScopeAt(0);
        initVariables(topmostScope);
        initFunctions(topmostScope);

        for (size_t i = 0; i < top->node()->body()->nodes(); ++i) {
            top->node()->body()->nodeAt(i)->visit(this);
        }

        topmostBytecodeFunction->setLocalsNumber(_currentScopeData->localsCount());
        getCurrentBytecode()->addInsn(BC_STOP);
        return Status::Ok();
    } catch (TranslationException& e) {
        return Status::Error(e.what());
    }
}

Bytecode* BytecodePrinter :: getCurrentBytecode() {
    return _currentScopeData->function()->bytecode();
}

void BytecodePrinter :: handleCmp(BinaryOpNode* node) {
    Instruction ins = BC_INVALID;

    switch (node->kind()) {
        case tGT: {
            ins = BC_IFICMPG;
            break;
        }

        case tLT: {
            ins = BC_IFICMPL;
            break;
        }

        case tLE: {
            ins = BC_IFICMPLE;
            break;
        }

        case tGE: {
            ins = BC_IFICMPGE;
            break;
        }

        case tEQ: {
            ins = BC_IFICMPE;
            break;
        }

        case tNEQ: {
            ins = BC_IFICMPNE;
            break;
        }

        default: {
            throw TranslationException("Unknown comparison operation.");
        }
    }

    node->right()->visit(this);
    castTopType(VT_INT);
    node->left()->visit(this);
    castTopType(VT_INT);

    Label l1(getCurrentBytecode());
    getCurrentBytecode()->addBranch(ins, l1);
    getCurrentBytecode()->addInsn(BC_ILOAD0);
    Label l2(getCurrentBytecode());
    getCurrentBytecode()->addBranch(BC_JA, l2);
    getCurrentBytecode()->bind(l1);
    getCurrentBytecode()->addInsn(BC_ILOAD1);
    getCurrentBytecode()->bind(l2);
    _topType = VT_INT;
}

void BytecodePrinter :: handleArithmeticOp(BinaryOpNode* node) {
    node->right()->visit(this);
    VarType rType = _topType;
    node->left()->visit(this);
    VarType lType = _topType;

    castOperandTypes(lType, rType);

    switch (node->kind()) {
        case tADD: {
            getCurrentBytecode()->addInsn(ADD(_topType));
            break;
        }

        case tSUB: {
            getCurrentBytecode()->addInsn(SUB(_topType));
            break;
        }

        case tMUL: {
            getCurrentBytecode()->addInsn(MUL(_topType));
            break;
        }

        case tDIV: {
            getCurrentBytecode()->addInsn(DIV(_topType));
            break;
        }

        default: {
            throw TranslationException("Unknown arithmetic operation.");
        }
    }
}

void BytecodePrinter :: handleLogicOp(BinaryOpNode* node) {
    Label start(getCurrentBytecode());
    Label finish(getCurrentBytecode());

    node->left()->visit(this);

    switch (node->kind()) {
        case tAND: {
            getCurrentBytecode()->addInsn(BC_ILOAD0);
            break;
        }

        case tOR: {
            getCurrentBytecode()->addInsn(BC_ILOAD1);
            break;
        }

        default: {
            throw TranslationException("Unknown logic operation");
        }
    }

    getCurrentBytecode()->addBranch(BC_IFICMPE, start);
    node->right()->visit(this);
    getCurrentBytecode()->addBranch(BC_JA, finish);
    getCurrentBytecode()->bind(start);

    switch (node->kind()) {
        case tAND: {
            getCurrentBytecode()->addInsn(BC_ILOAD0);
            break;
        }

        case tOR: {
            getCurrentBytecode()->addInsn(BC_ILOAD1);
            break;
        }

        default: {
            throw TranslationException("Unknown logic operation");
        }
    }

    getCurrentBytecode()->bind(finish);
}

void BytecodePrinter :: handleBitwiseOp(BinaryOpNode* node) {
    node->right()->visit(this);
    node->left()->visit(this);

    switch (node->kind()) {
        case tAOR: {
            getCurrentBytecode()->addInsn(BC_IAOR);
            break;
        }

        case tAAND: {
            getCurrentBytecode()->addInsn(BC_IAAND);
            break;
        }

        case tAXOR: {
            getCurrentBytecode()->addInsn(BC_IAXOR);
            break;
        }

        default: {
            throw TranslationException("Unknown bit operation.");
        }
    }
}

void BytecodePrinter :: handleModuloOp(BinaryOpNode* node) {
    node->right()->visit(this);
    if (_topType != VT_INT) {
        throw TranslationException("Right operand of modulo operation must be integer.");
    }

    node->left()->visit(this);
    if (_topType != VT_INT) {
        throw TranslationException("Left operand of modulo operation must be integer.");
    }

    getCurrentBytecode()->addInsn(BC_IMOD);
}

void BytecodePrinter :: castOperandTypes(VarType lType, VarType rType) {
    switch (lType) {
        case VT_INT: {
            switch (rType) {
                case VT_INT: {
                    break;
                }

                case VT_DOUBLE: {
                    _topType = VT_DOUBLE;
                    getCurrentBytecode()->addInsn(BC_I2D);
                    break;
                }

                default: {
                    throw TranslationException("Can't cast type of left opearand to type of right operand");

                }
            }
            break;
        }

        case VT_DOUBLE: {
            switch (rType) {
                case VT_INT: {
                    _topType = VT_INT;
                    getCurrentBytecode()->addInsn(BC_SWAP);
                    getCurrentBytecode()->addInsn(BC_I2D);
                    getCurrentBytecode()->addInsn(BC_SWAP);
                    break;
                }

                case VT_DOUBLE: {
                    break;
                }

                default: {
                    throw TranslationException("Can't cast type of left opearand to type of right operand");

                }
            }
            break;
        }

        default: {
            throw TranslationException("Can't cast both operands type to their common type");
        }
    }
}

void BytecodePrinter :: castTopType(VarType targetType) {
    switch (_topType) {
        case VT_INT: {
            switch (targetType) {
                case VT_INT: {
                    break;
                }

                case VT_DOUBLE: {
                    _topType = VT_DOUBLE;
                    getCurrentBytecode()->addInsn(BC_I2D);
                }

                default: {
                    throw TranslationException("Can't cast int to specifed type.");
                }
            }
            break;
        }

        case VT_DOUBLE: {
            switch (targetType) {
                case VT_INT: {
                    _topType = VT_INT;
                    getCurrentBytecode()->addInsn(BC_SWAP);
                    getCurrentBytecode()->addInsn(BC_I2D);
                    getCurrentBytecode()->addInsn(BC_SWAP);
                    break;
                }

                case VT_DOUBLE: {
                    break;
                }

                default: {
                    throw TranslationException("Can't cast double to specifed type.");
                }
            }
            break;
        }

        case VT_STRING: {
            break;
        }

        default: {
            throw TranslationException("Value of unknown type on TOS.");
        }
    }
}

void BytecodePrinter :: initVariables(Scope* scope) {
    Scope::VarIterator iterator(scope);
    while (iterator.hasNext()) {
        _currentScopeData->addAstVar(iterator.next());
    }
}

void BytecodePrinter :: initFunctions(Scope* scope) {
    Scope::FunctionIterator iterator(scope);
    while (iterator.hasNext()) {
        AstFunction* astFun = iterator.next();
        BytecodeFunction* bytecodeFunction = (BytecodeFunction*) _code->functionByName(astFun->name());

        if (!bytecodeFunction) {
            bytecodeFunction = new BytecodeFunction(astFun);
            _code->addFunction(bytecodeFunction);
        }
    }

    iterator = Scope::FunctionIterator(scope);
    while (iterator.hasNext()) {
        iterator.next()->node()->visit(this);
    }
}

void BytecodePrinter :: loadVar(const AstVar* var) {
    TranslatorVar v = _currentScopeData->getVar(var);
    _topType = var->type();

    if (v.contextId() == _currentScopeData->getId()) {
        getCurrentBytecode()->addInsn(LOAD(_topType));
        getCurrentBytecode()->addUInt16(v.id());
    } else {
        getCurrentBytecode()->addInsn(LOAD_CTX(_topType));
        getCurrentBytecode()->addUInt16(v.contextId());
        getCurrentBytecode()->addUInt16(v.id());
    }
}

void BytecodePrinter :: storeVar(const AstVar* var) {
    TranslatorVar v = _currentScopeData->getVar(var);

    if (v.contextId() == _currentScopeData->getId()) {
        getCurrentBytecode()->addInsn(STORE(var->type()));
        getCurrentBytecode()->addUInt16(v.id());
    } else {
        getCurrentBytecode()->addInsn(STORE_CTX(var->type()));
        getCurrentBytecode()->addUInt16(v.contextId());
        getCurrentBytecode()->addUInt16(v.id());
    }
}

} // end namespace details

BytecodeTranslator :: BytecodeTranslator() {}

Status* BytecodeTranslator :: translate(const string& program, Code** code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }
    details :: BytecodePrinter printer;
    Status* translationStatus = printer.translateToBytecode(*code, parser.top());
    return translationStatus;
}

BytecodeTranslator :: ~BytecodeTranslator() {}

} // end namespace mathvm
