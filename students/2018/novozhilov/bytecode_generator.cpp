//
// Created by jetbrains on 18.10.18.
//

#include <iostream>
#include <mathvm.h>
#include "parser.h"
#include "include/bytecode_generator.h"
#include "include/interpreter_code.h"

using namespace mathvm;
auto &out = std::cout;

Status* BytecodeTranslatorImpl::translate(const std::string &program, Code **code) {
    Parser parser;

    Status *status = parser.parseProgram(program);
    if (status->isOk()) {
        *code = new InterpreterCodeImpl;
        BytecodeGenerator bytecodeGenerator(*code);
        bytecodeGenerator.generateCodeForTopFunction(parser.top());
    }
    return status;
}

BytecodeGenerator::BytecodeGenerator(Code *code) : _code(code), _context(nullptr) {}

void BytecodeGenerator::generateCodeForTopFunction(AstFunction *function) {
    auto* bytecodeFunction = new BytecodeFunction(function);
    _code->addFunction(bytecodeFunction);
    generateCodeForFunction(function);
    bytecodeFunction->bytecode()->addInsn(BC_STOP);
}

void BytecodeGenerator::generateCodeForFunction(AstFunction *function) {
    auto* bcFunc = (BytecodeFunction*) _code->functionByName(function->name());

    _context = new Context(bcFunc, _context, function->scope());

    for (uint i = 0; i < function->parametersNumber(); ++i) {
        AstVar* var = function->scope()->lookupVariable(function->parameterName(i), false);
        storeValueToVar(var);
    }

    function->node()->visit(this);

    bcFunc->setScopeId(_context->getContextId());
    bcFunc->setLocalsNumber(_context->getVarsCount());

    Context* parentContext = _context->getParentContext();
    delete _context;
    _context = parentContext;
}

void BytecodeGenerator::visitForNode(ForNode *node) {
    // TODO: add check for type

    BinaryOpNode *inExpr = node->inExpr()->asBinaryOpNode();
    inExpr->left()->visit(this);

    const AstVar *var = node->var();
    storeValueToVar(var);

    Bytecode *bytecode = getBytecode();

    Label startLabel(bytecode);
    Label endLabel(bytecode);

    bytecode->bind(startLabel);
    loadValueFromVar(var);
    inExpr->right()->visit(this);

    bytecode->addBranch(BC_IFICMPL, endLabel);
    node->body()->visit(this);

    loadValueFromVar(var);
    bytecode->addInsn(BC_ILOAD1);
    bytecode->addInsn(BC_IADD);
    storeValueToVar(var);
    bytecode->addBranch(BC_JA, startLabel);
    bytecode->bind(endLabel);
}

void BytecodeGenerator::visitWhileNode(WhileNode *node) {
    Bytecode *bytecode = getBytecode();

    Label startLabel = Label(bytecode);
    Label endLabel = Label(bytecode);

    bytecode->bind(startLabel);
    node->whileExpr()->visit(this);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, endLabel);

    node->loopBlock()->visit(this);
    bytecode->addBranch(BC_JA, startLabel);
    bytecode->bind(endLabel);
}

void BytecodeGenerator::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    // TODO: add check for type

    Bytecode *bytecode = getBytecode();
    Label elseLabel(bytecode);
    Label endLabel(bytecode);

    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, elseLabel);

    node->thenBlock()->visit(this);
    bytecode->addBranch(BC_JA, endLabel);
    bytecode->bind(elseLabel);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }
    bytecode->bind(endLabel);
}

void BytecodeGenerator::visitPrintNode(PrintNode *node) {
    Bytecode *bytecode = getBytecode();
    
    uint32_t operands = node->operands();
    for (uint32_t i = 0; i < operands; ++i) {
        node->operandAt(i)->visit(this);
        switch (getTypeOnStackTop()) {
            case VT_INT:
                bytecode->addInsn(BC_IPRINT);
                break;
            case VT_DOUBLE:
                bytecode->addInsn(BC_DPRINT);
                break;
            case VT_STRING:
                bytecode->addInsn(BC_DPRINT);
                break;
            default: 
                throw std::runtime_error("undefined variable type");
        }
    }
}

void BytecodeGenerator::visitLoadNode(LoadNode *node) {
    loadValueFromVar(node->var());
}

void BytecodeGenerator::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);
    TokenKind op = node->op();
    Bytecode *bytecode = getBytecode();
    const AstVar *var = node->var();
    switch (op) {
        case tASSIGN:
            break;
        case tINCRSET:
        case tDECRSET: {
            loadValueFromVar(var);
            VarType type = var->type();
            if (type == VT_INT) {
                bytecode->addInsn(op == tINCRSET ? BC_IADD : BC_ISUB);
            } else if (type == VT_DOUBLE) {
                bytecode->addInsn(op == tINCRSET ? BC_DADD : BC_DSUB);
            } else {
                bytecode->addInsn(BC_INVALID);
            }
            break;
        }
        default:
            throw std::runtime_error("undefined variable type");
    }
    storeValueToVar(var);
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode *node) {
    Bytecode *bytecode = getBytecode();
    bytecode->addInsn(BC_ILOAD);
    bytecode->addInt64(node->literal());
    setTypeOnStackTop(VT_INT);
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    Bytecode *bytecode = getBytecode();
    bytecode->addInsn(BC_DLOAD);
    bytecode->addDouble(node->literal());
    setTypeOnStackTop(VT_DOUBLE);
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode *node) {
    Bytecode *bytecode = getBytecode();
    bytecode->addInsn(BC_SLOAD);
    bytecode->addUInt16(_code->makeStringConstant(node->literal()));
    setTypeOnStackTop(VT_STRING);
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
    Bytecode *bytecode = getBytecode();
    switch (node->kind()) {
        case tSUB: {
            switch (getTypeOnStackTop()) {
                case VT_INT:
                    bytecode->addInsn(BC_INEG);
                    break;
                case VT_DOUBLE:
                    bytecode->addInsn(BC_DNEG);
                    break;
                default:
                    throw std::runtime_error("illegal type on stack");
            }
        }

        case tNOT: {
            if (getTypeOnStackTop() != VT_INT) {
                throw std::runtime_error("illegal type on stack");
            }
            Label oneLabel(bytecode);
            Label endLabel(bytecode);

            bytecode->addInsn(BC_ILOAD1);
            bytecode->addBranch(BC_IFICMPE, oneLabel);
            // x == 0
            bytecode->addInsn(BC_ILOAD1);
            bytecode->addBranch(BC_JA, endLabel);
            bytecode->bind(oneLabel);
            // x == 1
            bytecode->addInsn(BC_ILOAD0);
            bytecode->bind(endLabel);
            break;
        }
        default:
            throw std::runtime_error("illegal operation kind");
    }
}

void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode *node) {
    switch (node->kind()) {
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            processArithmeticOperation(node);
            break;
        case tOR:
        case tAND:
        case tAOR:
        case tAAND:
        case tAXOR:
            processLogicOperation(node);
            break;
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            processComparingOperation(node);
            break;
        default:
            throw std::runtime_error("illegal operation kind");
    }
}

VarType BytecodeGenerator::processBinaryOperands(BinaryOpNode *node, bool castToInt) {
    node->left()->visit(this);
    if (castToInt) {
        castVarOnStackTop(VT_INT);
    }
    VarType leftType = getTypeOnStackTop();

    node->right()->visit(this);
    castVarOnStackTop(leftType);
    return leftType;
}

void BytecodeGenerator::processArithmeticOperation(BinaryOpNode *node) {
    VarType leftType = processBinaryOperands(node);
    if (!(leftType == VT_INT || leftType == VT_DOUBLE)) {
        throw std::runtime_error("illegal type on stack");
    }
    bool intType = leftType == VT_INT;

    Bytecode *bytecode = getBytecode();
    
    switch (node->kind()) {
        case tADD:
            bytecode->addInsn(intType ? BC_IADD : BC_DADD);
            break;
        case tSUB:
            bytecode->addInsn(intType ? BC_ISUB : BC_DSUB);
            break;
        case tMUL:
            bytecode->addInsn(intType ? BC_IMUL : BC_DMUL);
            break;
        case tDIV:
            bytecode->addInsn(intType ? BC_IDIV : BC_DDIV);
            break;
        case tMOD: {
            if (!intType) {
                throw std::runtime_error("illegal type on stack");
            }
            bytecode->addInsn(BC_IMOD);
            break;
        }
        default:
            throw std::runtime_error("illegal operation kind");
    }
    setTypeOnStackTop(leftType);
}

void BytecodeGenerator::processLogicOperation(BinaryOpNode *node) {
    VarType leftType = processBinaryOperands(node);
    if (leftType != VT_INT) {
        throw std::runtime_error("illegal type on stack");
    }

    Bytecode *bytecode = getBytecode();

    switch (node->kind()) {
        case tAOR:
        case tOR:
            bytecode->addInsn(BC_IAOR);
            break;
        case tAAND:
        case tAND:
            bytecode->addInsn(BC_IAAND);
            break;
        case tAXOR:
            bytecode->addInsn(BC_IAXOR);
            break;
        default:
            throw std::runtime_error("illegal operation kind");
    }

    setTypeOnStackTop(leftType);
}

void BytecodeGenerator::processComparingOperation(BinaryOpNode *node) {
    processBinaryOperands(node, true);

    Bytecode *bytecode = getBytecode();
    Instruction instruction;

    switch (node->kind()) {
        case tEQ:
            instruction = BC_IFICMPE;
            break;
        case tNEQ:
            instruction = BC_IFICMPNE;
            break;
        case tGT:
            instruction = BC_IFICMPG;
            break;
        case tGE:
            instruction = BC_IFICMPGE;
            break;
        case tLT:
            instruction = BC_IFICMPL;
            break;
        case tLE:
            instruction = BC_IFICMPLE;
            break;
        default:
            throw std::runtime_error("illegal operation kind");
    }
    bytecode->addInsn(instruction);
    setTypeOnStackTop(VT_INT);
}

void BytecodeGenerator::visitCallNode(CallNode *node) {
    TranslatedFunction *function = _code->functionByName(node->name());
    if (function == nullptr) {
        throw std::runtime_error("no function named " + node->name());
    }

    for (uint32_t i = node->parametersNumber(); i > 0; --i) {
        node->parameterAt(i - 1)->visit(this);
        castVarOnStackTop(function->parameterType(i - 1));
    }

    Bytecode *bytecode = getBytecode();
    bytecode->addInsn(BC_CALL);
    bytecode->addUInt16(function->id());
    setTypeOnStackTop(function->returnType());
}

void BytecodeGenerator::visitNativeCallNode(NativeCallNode *node __attribute__ ((unused))) {
    // TODO
}

void BytecodeGenerator::visitReturnNode(ReturnNode *node) {
    BytecodeFunction *function = _context->getFunction();
    VarType returnType = function->returnType();

    AstNode *returnExpr = node->returnExpr();
    if (returnExpr != nullptr) {
        returnExpr->visit(this);
        castVarOnStackTop(returnType);
    }
    setTypeOnStackTop(returnType);
    getBytecode()->addInsn(BC_RETURN);
}

void BytecodeGenerator::visitFunctionNode(FunctionNode *node) {
    node->body()->visit(this);
}

void BytecodeGenerator::visitBlockNode(BlockNode *node) {
    Scope::VarIterator varIterator(node->scope());
    while (varIterator.hasNext()) {
        _context->addVar(varIterator.next());
    }

    Scope::FunctionIterator functionIterator(node->scope());
    while (functionIterator.hasNext()) {
        AstFunction *function = functionIterator.next();
        BytecodeFunction *bytecodeFunction = new BytecodeFunction(function);
        uint16_t functionId = _code->addFunction(bytecodeFunction);
        bytecodeFunction->setScopeId(functionId);
        function->setInfo(bytecodeFunction);
    }

    functionIterator = Scope::FunctionIterator(node->scope());
    while (functionIterator.hasNext()) {
        generateCodeForFunction(functionIterator.next());
    }

    node->visitChildren(this);
//    TODO
//    setTosType(VT_VOID);
}

// ----------------------------- private -----------------------------

Bytecode *BytecodeGenerator::getBytecode() {
    return _context->getBytecode();
}

void BytecodeGenerator::storeValueToVar(AstVar const *var) {
    Context* context = findOwnerContextOfVar(var->name());
    bool inSameContext = context == _context;

    Instruction insn;
    switch (var->type()) {
        case VT_INT:
            insn = inSameContext ? BC_STOREIVAR : BC_STORECTXIVAR;
            break;
        case VT_DOUBLE:
            insn = inSameContext ? BC_STOREDVAR : BC_STORECTXDVAR;
            break;
        case VT_STRING:
            insn = inSameContext ? BC_STORESVAR : BC_STORECTXSVAR;
            break;
        default:
            throw std::runtime_error("undefined variable type");
    }

    getBytecode()->addInsn(insn);
    getBytecode()->addUInt16(context->getVarId(var->name()));
    if (!inSameContext) {
        getBytecode()->addUInt16(context->getContextId());
    }

    setTypeOnStackTop(var->type());
}

void BytecodeGenerator::loadValueFromVar(AstVar const *var) {
    Context* context = findOwnerContextOfVar(var->name());
    bool inSameContext = context == _context;

    Instruction insn;
    switch (var->type()) {
        case VT_INT:
            insn = inSameContext ? BC_LOADIVAR : BC_LOADCTXIVAR;
            break;
        case VT_DOUBLE:
            insn = inSameContext ? BC_LOADDVAR : BC_LOADCTXDVAR;
            break;
        case VT_STRING:
            insn = inSameContext ? BC_LOADSVAR : BC_LOADCTXSVAR;
            break;
        default:
            throw std::runtime_error("undefined variable type");
    }

    getBytecode()->addInsn(insn);
    getBytecode()->addUInt16(context->getVarId(var->name()));
    if (!inSameContext) {
        getBytecode()->addUInt16(context->getContextId());
    }

    setTypeOnStackTop(var->type());
}

VarType BytecodeGenerator::getTypeOnStackTop() {
    return _context->getTypeOnStackTop();
}

void BytecodeGenerator::setTypeOnStackTop(VarType type) {
    _context->setTypeOnStackTop(type);
}

void BytecodeGenerator::castVarOnStackTop(VarType type) {
    VarType typeOnStack = getTypeOnStackTop();
    if (typeOnStack == type) {
        return;
    }

    Bytecode *bytecode = getBytecode();
    if (typeOnStack == VT_INT && type == VT_DOUBLE) {
        bytecode->addInsn(BC_I2D);
    } else if (typeOnStack == VT_DOUBLE && type == VT_INT) {
        bytecode->addInsn(BC_D2I);
    } else if (typeOnStack == VT_STRING && type == VT_INT) {
        bytecode->addInsn(BC_S2I);
    } else if (typeOnStack == VT_STRING && type == VT_DOUBLE) {
        bytecode->addInsn(BC_S2I);
        bytecode->addInsn(BC_I2D);
    } else {
        throw std::runtime_error("cast error");
    }
    setTypeOnStackTop(type);
}

Context *BytecodeGenerator::findOwnerContextOfVar(string name) {
    for (Context* context = _context; context != nullptr; context = context->getParentContext()) {
        if (context->getVar(name) != nullptr) {
            return context;
        }
    }
    return nullptr;
}
