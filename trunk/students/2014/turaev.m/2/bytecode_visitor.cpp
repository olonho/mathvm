#include "ast.h"
#include "visitors.h"
#include "rich_function.h"
#include "function_crawler.h"
#include "bytecode_visitor.h"
#include "if_builder.h"

using namespace mathvm;

BytecodeMainVisitor::BytecodeMainVisitor(Code *code, AstFunction *topFunction) : _code(code) {
    _code->addFunction(_currentFunction = new RichFunction(topFunction));
    FunctionCrawler crawler(code);
    topFunction->node()->body()->visit(&crawler);

    _typeTokenInstruction[VT_DOUBLE][tADD] = BC_DADD;
    _typeTokenInstruction[VT_INT][tADD] = BC_IADD;
    _typeTokenInstruction[VT_DOUBLE][tSUB] = BC_DSUB;
    _typeTokenInstruction[VT_INT][tSUB] = BC_ISUB;
    _typeTokenInstruction[VT_DOUBLE][tMUL] = BC_DMUL;
    _typeTokenInstruction[VT_INT][tMUL] = BC_IMUL;
    _typeTokenInstruction[VT_DOUBLE][tDIV] = BC_DDIV;
    _typeTokenInstruction[VT_INT][tDIV] = BC_IDIV;

    _typeTokenInstruction[VT_INT][tAOR] = BC_IAOR;
    _typeTokenInstruction[VT_INT][tAAND] = BC_IAAND;
    _typeTokenInstruction[VT_INT][tAXOR] = BC_IAXOR;

    _typeTokenInstruction[VT_INT][tEQ] = BC_IFICMPE;
    _typeTokenInstruction[VT_INT][tNEQ] = BC_IFICMPNE;
    _typeTokenInstruction[VT_INT][tGT] = BC_IFICMPG;
    _typeTokenInstruction[VT_INT][tGE] = BC_IFICMPGE;
    _typeTokenInstruction[VT_INT][tLT] = BC_IFICMPL;
    _typeTokenInstruction[VT_INT][tLE] = BC_IFICMPLE;
}

void BytecodeMainVisitor::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        switch (_typesStack.top()) {
            case VT_DOUBLE: {
                emit(BC_DPRINT);
                break;
            }
            case VT_INT: {
                emit(BC_IPRINT);
                break;
            }
            case VT_STRING: {
                emit(BC_SPRINT);
                break;
            }
            default: {
                assert(0);
            }
        }
        _typesStack.pop();
    }
}

void BytecodeMainVisitor::visitNativeCallNode(NativeCallNode *node) {
    assert(0);
}

void BytecodeMainVisitor::visitCallNode(CallNode *node) {
    node->visitChildren(this);
    emit(BC_CALL);
    uint16_t functionId = _code->functionByName(node->name())->id();
    emitUInt16(functionId);
}

void BytecodeMainVisitor::visitReturnNode(ReturnNode *node) {
    node->visitChildren(this);
    emit(BC_RETURN);
}

void BytecodeMainVisitor::visitFunctionNode(FunctionNode *node) {
    uint16_t adjustment = node->parametersNumber() - 1;
    for (uint16_t i = 0; i < node->parametersNumber(); ++i) {
        switch (node->parameterType(adjustment - i)) {
            case (VT_INVALID): { //for () -> Type functions
                continue;
            }
            case (VT_INT): {
                emit(BC_STOREIVAR);
                emitUInt16(adjustment - i);
                break;
            }
            case (VT_DOUBLE): {
                emit(BC_STOREDVAR);
                emitUInt16(adjustment - i);
                break;
            }
            case (VT_STRING): {
                emit(BC_STORESVAR);
                emitUInt16(adjustment - i);
                break;
            }
            default: {
                assert(0);
            }
        }
    }

    node->visitChildren(this);
}


//TODO: refactor this method
void BytecodeMainVisitor::visitBlockNode(BlockNode *node) {
    _scopeToFunctionMap[node->scope()] = _currentFunction->id();

    //variables:
    Scope::VarIterator it(node->scope());
    while (it.hasNext()) {
        AstVar *var = it.next();
        //TODO: this will override value with same name in current fucntion's scope
        _currentFunction->addLocalVariable(var->name());
    }

    //functions:
    {
        RichFunction *parentFunction = _currentFunction;
        Scope::FunctionIterator it(node->scope());
        while (it.hasNext()) {
            AstFunction *currentAstFunction = it.next();
            _currentFunction = dynamic_cast<RichFunction *>(_code->functionByName(currentAstFunction->name()));
            assert(_currentFunction != 0);
            _scopeToFunctionMap[currentAstFunction->scope()] = _currentFunction->id();
            currentAstFunction->node()->visit(this);
        }
        _currentFunction = parentFunction;
    }

    node->visitChildren(this);
}

void BytecodeMainVisitor::visitIfNode(IfNode *node) {
    ThenStep ifBuilder = IfBuilder(this, bytecode())
                         .If(node->ifExpr())
                         .Then(node->thenBlock());
    if (node->elseBlock()) {
        ifBuilder
        .Else(node->elseBlock())
        .Done();
    } else {
        ifBuilder
        .Done();
    }
}

void BytecodeMainVisitor::visitWhileNode(WhileNode *node) {
    Label beginLoop = bytecode()->currentLabel();
    IfBuilder(this, bytecode())
    .If(node->whileExpr())
    .Then(node->loopBlock())
    .ThenJumpTo(beginLoop)
    .Done();
}

void BytecodeMainVisitor::visitForNode(ForNode *node) {
    BinaryOpNode *range = dynamic_cast<BinaryOpNode *>(node->inExpr());
    assert(range);
    assert(range->kind() == tRANGE);
    AstNode *leftNode = range->left();
    AstNode *rightNode = range->right();
    const AstVar *astVar = node->var();
    // TODO: Unary minus might be part of range
    // assert(dynamic_cast<LoadNode *>(leftNode) != 0 || dynamic_cast<IntLiteralNode *>(leftNode) != 0);
    // assert(dynamic_cast<LoadNode *>(rightNode) != 0 || dynamic_cast<IntLiteralNode *>(rightNode) != 0);

    StoreNode store(0, astVar, leftNode, tASSIGN);
    IntLiteralNode oneLiteral(0, (uint64_t) 1);
    StoreNode increment(0, astVar, &oneLiteral, tINCRSET);
    LoadNode loadVar(0, astVar);
    BinaryOpNode greaterEqCondition(0, tGE, &loadVar, leftNode);
    BinaryOpNode lessEqCondition(0, tLE, &loadVar, rightNode);
    BinaryOpNode condition(0, tAND, &greaterEqCondition, &lessEqCondition);

    store.visit(this);
    Label beginLoop = bytecode()->currentLabel();
    IfBuilder(this, bytecode())
    .If(&condition)
    .Then(node->body())
    .ThenVisit(increment)
    .ThenJumpTo(beginLoop)
    .Done();
}

void BytecodeMainVisitor::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);

    switch (node->op()) {
        case (tASSIGN): {
            storeToVariable(node->var());
            break;
        }
        case (tINCRSET): {
            loadVariable(node->var());
            binary_math(tADD);
            storeToVariable(node->var());
            break;
        }
        case (tDECRSET): {
            loadVariable(node->var());
            binary_math(tSUB);
            storeToVariable(node->var());
            break;
        }
        default: {
            assert(0);
        }
    }
}

void BytecodeMainVisitor::visitLoadNode(LoadNode *node) {
    loadVariable(node->var());
}

void BytecodeMainVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    emit(BC_ILOAD);
    emitInt64(node->literal());
    _typesStack.push(VT_INT);
}

void BytecodeMainVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    emit(BC_DLOAD);
    emitDouble(node->literal());
    _typesStack.push(VT_DOUBLE);
}

void BytecodeMainVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    uint16_t constantId = _code->makeStringConstant(node->literal());
    emit(BC_SLOAD);
    emitUInt16(constantId);
    _typesStack.push(VT_STRING);
}

void BytecodeMainVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
    unary_math(node->kind());
}

void BytecodeMainVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    TokenKind operation = node->kind();
    switch (operation) {
        case (tOR):
        case (tAND): {
            binary_logic(node);
            break;
        }
        case (tADD):
        case (tSUB):
        case (tMUL):
        case (tDIV):
        case (tMOD):
        case (tAOR):
        case (tAXOR):
        case (tAAND): {
            node->right()->visit(this);
            node->left()->visit(this);
            binary_math(node->kind());
            break;
        }
        case (tEQ):
        case (tNEQ):
        case (tGT):
        case (tGE):
        case (tLT):
        case (tLE): {
            binary_comparison(node);
            break;
        }
        default: {
            assert(0);
        }
    }
}

Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    Parser parser;
    Status *status = parser.parseProgram(program);
    if (status && status->isError()) {
        return status;
    }

    BytecodeMainVisitor visitor(*code = new InterpreterCodeImpl(), parser.top());
    parser.top()->node()->visit(&visitor);

    return Status::Ok();
}

Translator *Translator::create(const string &impl) {
    return new BytecodeTranslatorImpl();
}