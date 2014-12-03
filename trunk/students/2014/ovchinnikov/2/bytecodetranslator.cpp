#include "bytecodetranslator.hpp"
#include "parser.h"

using namespace mathvm;

Translator *Translator::create(const string & impl) {
    if (impl == "translator" || impl == "") {
        return new BytecodeTranslator();
    } else {
        return NULL;
    }
}

Status *BytecodeTranslator::translate(const string & program, Code **out) {
    Parser p;
    {
        Status *s = p.parseProgram(program);
        if (s->isError()) {
            return s;
        }
    }

    auto main = new BytecodeFunction(p.top());
    interpreter->addFunction(main);
    bc = main->bytecode();

    try {
        p.top()->node()->visit(this);
    } catch (const char *msg) {
        *out = 0;
        return Status::Error(msg);
    }

    *out = interpreter;
    return Status::Ok();
}

void BytecodeTranslator::visitFunctionNode(FunctionNode *node) {
    uint16_t id = interpreter->functionByName(node->name())->id();
    BlockScope functionScope(id, currentBlockScope, &node->signature());
    currentBlockScope = &functionScope;
    node->body()->visit(this);
    currentBlockScope = currentBlockScope->parent();
}

void BytecodeTranslator::visitBlockNode(BlockNode *node) {
    enterScope();

    Scope::VarIterator varIterator(node->scope());
    while (varIterator.hasNext()) {
        auto nextVar = varIterator.next();
        currentBlockScope->defineVar(nextVar->type(), nextVar->name());
    }

    Scope::FunctionIterator functionIterator(node->scope());
    while (functionIterator.hasNext()) {
        Bytecode *bytecode = bc;
        auto nextFunction = functionIterator.next();
        auto translatedFunction = new BytecodeFunction(nextFunction);
        interpreter->addFunction(translatedFunction);
        bc = translatedFunction->bytecode();
        nextFunction->node()->visit(this);
        bc = bytecode;
    }

    node->visitChildren(this);
    leaveScope();
}

void BytecodeTranslator::visitIfNode(IfNode *node) {
    Label elseLabel(bc);
    Label endIfLabel(bc);
    node->ifExpr()->visit(this);
    bc->add(BC_ILOAD0);
    bc->addBranch(BC_IFICMPE, elseLabel);
    node->thenBlock()->visit(this);
    bc->addBranch(BC_JA, endIfLabel);
    elseLabel.bind(bc->current());
    if (node->elseBlock()) { node->elseBlock()->visit(this); }
    endIfLabel.bind(bc->current());
}

void BytecodeTranslator::visitWhileNode(WhileNode *node) {
    Label beginWhile = bc->currentLabel();
    Label endWhile(bc);
    node->whileExpr()->visit(this);
    bc->add(BC_ILOAD0);
    bc->addBranch(BC_IFICMPE, endWhile);        // compare to 0 and jump to end if true
    node->loopBlock()->visit(this);             // do while stuff
    bc->addBranch(BC_JA, beginWhile);
    endWhile.bind(bc->current());
}


void BytecodeTranslator::visitForNode(ForNode *node) {
    BinaryOpNode *range = node->inExpr()->asBinaryOpNode();
    if (!range || range->kind() != tRANGE) { throw "Unsupported range"; }

    enterScope();
    currentBlockScope->defineVar(node->var());
    range->left()->visit(this);
    processStoreVar(node->var());

    Label beginFor = bc->currentLabel();
    Label endFor(bc);

    // push counter
    processLoadVar(node->var());
    range->right()->visit(this);
    // compare range end with counter
    // if range < counter jump to end
    bc->addBranch(BC_IFICMPL, endFor);

    // do for stuff
    node->body()->visit(this);

    //increment counter
    processLoadVar(node->var());
    bc->add(BC_ILOAD1);
    bc->add(BC_IADD);
    processStoreVar(node->var());

    bc->addBranch(BC_JA, beginFor);
    endFor.bind(bc->current());
    leaveScope();
}

void BytecodeTranslator::visitLoadNode(LoadNode *node) {
    processLoadVar(node->var());
}

void BytecodeTranslator::visitStoreNode(StoreNode *node) {
    auto astVar = node->var();
    if (node->op() == tINCRSET || node->op() == tDECRSET) {
        processLoadVar(astVar);
        node->value()->visit(this);
        Instruction code = node->op() == tINCRSET
                           ? TYPE_AND_ACTION_TO_BC_NUMERIC(astVar->type(), , ADD)
                           : TYPE_AND_ACTION_TO_BC_NUMERIC(astVar->type(), , SUB);
        if (code == BC_INVALID) { throw "Unsupported increase/decrease operation"; }
        bc->add(code);
    } else {
        node->value()->visit(this);
    }
    processStoreVar(astVar);
}

void BytecodeTranslator::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
    switch (node->kind()) {
        case tSUB: {
                Instruction code = TYPE_AND_ACTION_TO_BC_NUMERIC(tos, , NEG);
                if (code == BC_INVALID) { throw "Unsupported negation type"; }
                bc->add(code);
            }; break;
        case tNOT: {
                coerceToBoolean();
                processComparison(BC_IFICMPE);
            }; break;
        default: throw "Unsupported unary operation";
    }
}

void BytecodeTranslator::visitBinaryOpNode(BinaryOpNode *node) {
    auto token = node->kind();

    node->left()->visit(this);
    auto leftType = tos;

    node->right()->visit(this);
    auto rightType = tos;

    switch (token) {
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:  processArithmeticOperator(token, leftType, rightType); break;
        case tEQ:
        case tNEQ:
        case tGE:
        case tGT:
        case tLT:
        case tLE:   processComparison(token, leftType, rightType); break;
        case tOR:
        case tAND:  processLogicOperator(token, leftType, rightType); break;
        case tAAND:
        case tAOR:
        case tAXOR: processArithmeticLogicOperator(token, leftType, rightType); break;
        default: throw "Unsupported binary operation";
    }
}

void BytecodeTranslator::visitCallNode(CallNode *node) {
    node->visitChildren(this);
    auto f = interpreter->functionByName(node->name());
    assert(f && f->id() > 0);
    bc->add(BC_CALL);
    bc->addUInt16(f->id());
    tos = f->returnType();
}

void BytecodeTranslator::visitNativeCallNode(NativeCallNode *node) {
    interpreter->makeNativeFunction(node->nativeName(), node->nativeSignature(), 0);
}

void BytecodeTranslator::visitReturnNode(ReturnNode *node) {
    node->visitChildren(this);
    bc->add(BC_RETURN);
}

void BytecodeTranslator::visitIntLiteralNode(IntLiteralNode *node) {
    bc->add(BC_ILOAD);
    bc->addInt64(node->literal());
    tos = VT_INT;
}

void BytecodeTranslator::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    bc->add(BC_DLOAD);
    bc->addDouble(node->literal());
    tos = VT_DOUBLE;
}

void BytecodeTranslator::visitStringLiteralNode(StringLiteralNode *node) {
    bc->add(BC_SLOAD);
    bc->addUInt16(interpreter->makeStringConstant(node->literal()));
    tos = VT_STRING;
}

void BytecodeTranslator::visitPrintNode(PrintNode *node) {
    for (auto i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        Instruction code = TYPE_AND_ACTION_TO_BC(tos, , PRINT);
        if (code == BC_INVALID) { throw "Invalid print operand"; }
        bc->add(code);
    }
}

