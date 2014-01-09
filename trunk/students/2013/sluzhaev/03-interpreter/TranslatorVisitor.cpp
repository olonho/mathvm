#include <string>
#include <dlfcn.h>
#include "parser.h"
#include "TranslatorVisitor.h"

using namespace mathvm;

Status* BytecodeTranslatorImpl::translate(std::string const& program, Code** code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status && status->isError()) {
        return status;
    }
    BytecodeImpl* bytecode = new BytecodeImpl();
    *code = bytecode;
    try {
        TranslatorVisitor visitor(bytecode);
        visitor.run(parser.top());
    } catch (TranslationError e) {
        return new Status(e.getMessage(), e.getPosition());
    }
    return 0;
}

TranslatorVisitor::TranslatorVisitor(BytecodeImpl* code) : code(code), currentScope(0), lastExpressionType(VT_INVALID) {}

void TranslatorVisitor::run(AstFunction* function) {
    BytecodeFunction* bytecodeFunction = (BytecodeFunction*)code->functionByName(function->name());
    if (!bytecodeFunction) {
        bytecodeFunction = new BytecodeFunction(function);
        code->addFunction(bytecodeFunction);
    }
    
    BytecodeFunction* oldFunction = currentFunction;
    currentFunction = bytecodeFunction;
    currentScope = new VariableScope(currentScope, currentFunction->id());

    for (Signature::const_iterator it = currentFunction->signature().begin() + 1; it != currentFunction->signature().end(); ++it) {
        AstVar* variable = function->scope()->lookupVariable(it->second);
        currentScope->addVariable(variable);
        storeVariable(variable);
    }

    function->node()->visit(this);
    
    if (!currentScope->getParent()) {
        currentFunction->bytecode()->addInsn(BC_STOP);
    }
    
    VariableScope* parent = currentScope->getParent();
    delete currentScope;
    currentScope = parent;
    currentFunction = oldFunction;
}

void TranslatorVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    currentFunction->bytecode()->addInsn(BC_ILOAD);
    currentFunction->bytecode()->addInt64(node->literal());
    lastExpressionType = VT_INT;
}

void TranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    currentFunction->bytecode()->addInsn(BC_DLOAD);
    currentFunction->bytecode()->addDouble(node->literal());
    lastExpressionType = VT_DOUBLE;
}

void TranslatorVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    currentFunction->bytecode()->addInsn(BC_SLOAD);
    currentFunction->bytecode()->addUInt16(code->makeStringConstant(node->literal()));
    lastExpressionType = VT_STRING;
}

void TranslatorVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);
    Label setFalse(currentFunction->bytecode());
    Label convEnd (currentFunction->bytecode());
    switch (node->kind()) {
        case tNOT:
            convertToBool();
            currentFunction->bytecode()->addInsn(BC_ILOAD1);
            currentFunction->bytecode()->addInsn(BC_ISUB);
            break;
        case tSUB:
            if (lastExpressionType != VT_INT && lastExpressionType != VT_DOUBLE) {
                throw TranslationError("Math operation with not numeric types", node->position());
            }
            currentFunction->bytecode()->addInsn(lastExpressionType == VT_DOUBLE ? BC_DNEG : BC_INEG);
            break;
        default:
            throw TranslationError(std::string("Unsupported unary operation: ") + tokenOp(node->kind()), node->position());
    }
}

void TranslatorVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    node->right()->visit(this);
    VarType rightType = lastExpressionType;
    node->left()->visit(this);
    VarType leftType = lastExpressionType;

    if ((leftType != VT_INT && leftType != VT_DOUBLE) || (rightType != VT_INT && rightType != VT_DOUBLE)) {
        throw TranslationError("Unsupported types in binary expression", node->position());
    }

    switch (node->kind()) {
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            processArithmeticOperation(node, leftType, rightType, node->kind());
            break;
        case tAND:
        case tOR:
            processLogicalOperation(node, leftType, rightType, node->kind());
            break;
	case tAAND:
	case tAOR:
	case tAXOR:
            processBitwiseOperation(node, leftType, rightType, node->kind());
            break;
        case tEQ:
        case tNEQ:
        case tGE:
        case tGT:
        case tLE:
        case tLT:
            processRelationalOperation(node, leftType, rightType, node->kind());
            break;
        default:
            throw TranslationError(std::string("Unsupported binary operation: ") + tokenOp(node->kind()), node->position());
    }
}

void TranslatorVisitor::visitCallNode(CallNode* node) {
    TranslatedFunction* function = code->functionByName(node->name());
    if (!function) {
        throw TranslationError("Function " + node->name() + "wasn't declared", node->position());
    }
    for (int i = node->parametersNumber() - 1; i >= 0; --i) {
        node->parameterAt(i)->visit(this);
        if (function->parameterType(i) == VT_INT) {
            if (lastExpressionType == VT_DOUBLE) {
                currentFunction->bytecode()->addInsn(BC_D2I);
            } else if (lastExpressionType == VT_STRING) {
                currentFunction->bytecode()->addInsn(BC_S2I);
            }
        } else if (function->parameterType(i) == VT_DOUBLE) {
            if (lastExpressionType == VT_INT) {
                currentFunction->bytecode()->addInsn(BC_I2D);
            }
        }
    }

    currentFunction->bytecode()->addInsn(BC_CALL);
    currentFunction->bytecode()->addInt16(function->id());
    if (function->returnType() != VT_VOID) {
        lastExpressionType = function->returnType();
    }
}

void TranslatorVisitor::visitLoadNode(LoadNode* node) {
    loadVariable(node->var());
}

void TranslatorVisitor::visitStoreNode(StoreNode* node) {
    node->value()->visit(this);
    VarType type = node->var()->type();
    switch(node->op()) {
        case tASSIGN:
            break;
        case tINCRSET:
            loadVariable(node->var());
            currentFunction->bytecode()->addInsn(type == VT_INT ? BC_IADD : (type == VT_DOUBLE ? BC_DADD : BC_INVALID));
            break;
        case tDECRSET:
            loadVariable(node->var());
            currentFunction->bytecode()->addInsn(type == VT_INT ? BC_ISUB : (type == VT_DOUBLE ? BC_DSUB : BC_INVALID));
            break;
        default:
            throw TranslationError("Unsupported operation in store node", node->position());
    }
    storeVariable(node->var());
}

void TranslatorVisitor::visitBlockNode(BlockNode* node) {
    currentFunction->setLocalsNumber(currentFunction->localsNumber() + node->scope()->variablesCount());
    Scope::VarIterator vars(node->scope());
    
    while (vars.hasNext()) {
        currentScope->addVariable(vars.next());
    }
    
    Scope::FunctionIterator funcs(node->scope());
    while (funcs.hasNext()) {
        code->addFunction(new BytecodeFunction (funcs.next()));
    }
    
    funcs = Scope::FunctionIterator(node->scope());
    while (funcs.hasNext()) {
        run(funcs.next());
    }
    
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
    }
}

void TranslatorVisitor::visitFunctionNode(FunctionNode* node) {
    if (node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
    } else {
        node->body()->visit(this);
    }
}

void TranslatorVisitor::visitReturnNode(ReturnNode* node) {
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
        VarType returnType = currentFunction->returnType();
        if (lastExpressionType == VT_INT && returnType == VT_DOUBLE) {
            currentFunction->bytecode()->addInsn(BC_I2D);
        } else if (lastExpressionType == VT_DOUBLE && returnType == VT_INT) {
            currentFunction->bytecode()->addInsn(BC_D2I);
        } else if (lastExpressionType == VT_STRING && returnType == VT_INT) {
            currentFunction->bytecode()->addInsn(BC_S2I);
        }
    }
    currentFunction->bytecode()->addInsn(BC_RETURN);
}

void TranslatorVisitor::visitNativeCallNode(NativeCallNode* node) {
    void* addr = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (!addr) {
        throw TranslationError("Native function '" + node->nativeName() + "' not found", node->position());
    }
    uint16_t id = code->makeNativeFunction(node->nativeName(), node->nativeSignature(), addr);

    currentFunction->bytecode()->addInsn(BC_CALLNATIVE);
    currentFunction->bytecode()->addUInt16(id);
    currentFunction->bytecode()->addInsn(BC_RETURN);
}

void TranslatorVisitor::visitPrintNode(PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        currentFunction->bytecode()->addInsn(lastExpressionType == VT_INT ? BC_IPRINT :
                    (lastExpressionType == VT_DOUBLE ? BC_DPRINT : (lastExpressionType == VT_STRING ? BC_SPRINT : BC_INVALID)));
    }
}

void TranslatorVisitor::visitForNode(ForNode* node) {
    Label start(currentFunction->bytecode());
    Label end(currentFunction->bytecode());
    
    if (node->var()->type() != VT_INT) {
        throw TranslationError("Non-range type in for expression", node->position());
    }
    
    if (!node->inExpr()->isBinaryOpNode()) {
        throw TranslationError("Non-range type in for expression", node->position());
    }
    
    BinaryOpNode* inExpr = node->inExpr()->asBinaryOpNode();
    if (inExpr->kind() != tRANGE) {
        throw TranslationError("Non-range type in for expression", node->position());
    }

    inExpr->left()->visit(this);
    if (lastExpressionType != VT_INT) {
        throw TranslationError("Non-range type in for expression", node->position());
    }
    storeVariable(node->var());

    currentFunction->bytecode()->bind(start);
    inExpr->right()->visit(this);
    
    if (lastExpressionType != VT_INT) {
        throw TranslationError("Non-range type in for expression", node->position());
    }

    loadVariable(node->var());
    
    currentFunction->bytecode()->addBranch(BC_IFICMPG, end);
    node->body()->visit(this);

    loadVariable(node->var());
    currentFunction->bytecode()->addInsn(BC_ILOAD1);
    currentFunction->bytecode()->addInsn(BC_IADD);
    storeVariable(node->var());
    currentFunction->bytecode()->addBranch(BC_JA, start);
    currentFunction->bytecode()->bind(end);
}

void TranslatorVisitor::visitWhileNode(WhileNode* node) {
    Label start(currentFunction->bytecode());
    Label end (currentFunction->bytecode());
    currentFunction->bytecode()->bind(start);
    node->whileExpr()->visit(this);
    currentFunction->bytecode()->addInsn(BC_ILOAD0);
    currentFunction->bytecode()->addBranch(BC_IFICMPE, end);
    node->loopBlock()->visit(this);
    currentFunction->bytecode()->addBranch(BC_JA, start);
    currentFunction->bytecode()->bind(end);
}

void TranslatorVisitor::visitIfNode(IfNode* node) {
    Label ifElse (currentFunction->bytecode());
    Label ifEnd (currentFunction->bytecode());
    node->ifExpr()->visit(this);
    currentFunction->bytecode()->addInsn(BC_ILOAD0);
    currentFunction->bytecode()->addBranch(BC_IFICMPE, ifElse);
    node->thenBlock()->visit(this);
    currentFunction->bytecode()->addBranch(BC_JA, ifEnd);
    currentFunction->bytecode()->bind(ifElse);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }
    currentFunction->bytecode()->bind(ifEnd);
}

void TranslatorVisitor::loadVariable(const AstVar* variable) {
    std::pair<uint16_t, uint16_t> scopeVariable = currentScope->getVariable(variable);
    VarType type = variable->type();
    if (scopeVariable.second == currentFunction->id()) {
        switch (scopeVariable.first) {
            case 0:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_LOADIVAR0 :
                    (type == VT_DOUBLE ? BC_LOADDVAR0 : (type == VT_STRING ? BC_LOADSVAR0 : BC_INVALID)));
                break;
            case 1:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_LOADIVAR1 :
                    (type == VT_DOUBLE ? BC_LOADDVAR1 : (type == VT_STRING ? BC_LOADSVAR1 : BC_INVALID)));
                break;
            case 2:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_LOADIVAR2 :
                    (type == VT_DOUBLE ? BC_LOADDVAR2 : (type == VT_STRING ? BC_LOADSVAR2 : BC_INVALID)));
                break;
            case 3:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_LOADIVAR3 :
                    (type == VT_DOUBLE ? BC_LOADDVAR3 : (type == VT_STRING ? BC_LOADSVAR3 : BC_INVALID)));
                break;
            default:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_LOADIVAR :
                    (type == VT_DOUBLE ? BC_LOADDVAR : (type == VT_STRING ? BC_LOADSVAR : BC_INVALID)));
                currentFunction->bytecode()->addUInt16(scopeVariable.first);
        }
    } else {
        currentFunction->bytecode()->addInsn(type == VT_INT ? BC_LOADCTXIVAR :
            (type == VT_DOUBLE ? BC_LOADCTXDVAR : (type == VT_STRING ? BC_LOADCTXSVAR : BC_INVALID)));
        currentFunction->bytecode()->addUInt16(scopeVariable.second);
        currentFunction->bytecode()->addUInt16(scopeVariable.first);
    }
    lastExpressionType = variable->type();
}

void TranslatorVisitor::storeVariable(const AstVar* variable) {
    std::pair<uint16_t, uint16_t> scopeVariable = currentScope->getVariable(variable);
    VarType type = variable->type();

    if (scopeVariable.second == currentFunction->id()) {
        switch (scopeVariable.first) {
            case 0:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_STOREIVAR0 :
                    (type == VT_DOUBLE ? BC_STOREDVAR0 : (type == VT_STRING ? BC_STORESVAR0 : BC_INVALID)));
                break;
            case 1:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_STOREIVAR1 :
                    (type == VT_DOUBLE ? BC_STOREDVAR1 : (type == VT_STRING ? BC_STORESVAR1 : BC_INVALID)));
                break;
            case 2:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_STOREIVAR2 :
                    (type == VT_DOUBLE ? BC_STOREDVAR2 : (type == VT_STRING ? BC_STORESVAR2 : BC_INVALID)));
                break;
            case 3:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_STOREIVAR3 :
                    (type == VT_DOUBLE ? BC_STOREDVAR3 : (type == VT_STRING ? BC_STORESVAR3 : BC_INVALID)));
                break;
            default:
                currentFunction->bytecode()->addInsn(type == VT_INT ? BC_STOREIVAR :
                    (type == VT_DOUBLE ? BC_STOREDVAR : (type == VT_STRING ? BC_STORESVAR : BC_INVALID)));
                currentFunction->bytecode()->addUInt16(scopeVariable.first);
        }
    } else {
        currentFunction->bytecode()->addInsn(type == VT_INT ? BC_STORECTXIVAR :
            (type == VT_DOUBLE ? BC_STORECTXDVAR : (type == VT_STRING ? BC_STORECTXSVAR : BC_INVALID)));
        currentFunction->bytecode()->addUInt16(scopeVariable.second);
        currentFunction->bytecode()->addUInt16(scopeVariable.first);
    }
}

void TranslatorVisitor::convertToBool() {
	if (lastExpressionType == VT_DOUBLE)
		currentFunction->bytecode()->addInsn(BC_D2I);
	else if (lastExpressionType == VT_STRING)
		currentFunction->bytecode()->addInsn(BC_S2I);
	Label setFalse(currentFunction->bytecode());
	Label convEnd(currentFunction->bytecode());
	currentFunction->bytecode()->addInsn(BC_ILOAD0);
	currentFunction->bytecode()->addBranch(BC_IFICMPE, setFalse);
	currentFunction->bytecode()->addInsn(BC_ILOAD1);
	currentFunction->bytecode()->addBranch(BC_JA, convEnd);
	currentFunction->bytecode()->bind(setFalse);
	currentFunction->bytecode()->addInsn(BC_ILOAD0);
	currentFunction->bytecode()->bind(convEnd);
}

void TranslatorVisitor::processArithmeticOperation(BinaryOpNode* node, VarType leftType, VarType rightType, TokenKind operation) {
    VarType operationType = (leftType == VT_DOUBLE || rightType == VT_DOUBLE) ? VT_DOUBLE : VT_INT;
    
    if (leftType != operationType) {
        currentFunction->bytecode()->addInsn(BC_I2D);
        lastExpressionType = operationType;
    }
    
    if (rightType != operationType) {
        currentFunction->bytecode()->addInsn(BC_SWAP);
        currentFunction->bytecode()->addInsn(BC_I2D);
        currentFunction->bytecode()->addInsn(BC_SWAP);
    }

    switch (operation) {
        case tADD:
            currentFunction->bytecode()->addInsn(operationType == VT_DOUBLE ? BC_DADD : BC_IADD);
            break;
        case tSUB:
            currentFunction->bytecode()->addInsn(operationType == VT_DOUBLE ? BC_DSUB : BC_ISUB);
            break;
        case tMUL:
            currentFunction->bytecode()->addInsn(operationType == VT_DOUBLE ? BC_DMUL : BC_IMUL);
            break;
        case tDIV:
            currentFunction->bytecode()->addInsn(operationType == VT_DOUBLE ? BC_DDIV : BC_IDIV);
            break;
        case tMOD:
            if (operationType != VT_INT) {
                throw TranslationError("Not integer type in mod operation", node->position());
            }
            currentFunction->bytecode()->addInsn(BC_IMOD);
            break;
        default:
            throw TranslationError(std::string("Unsupported arithmetic operation: ") + tokenOp(operation), node->position());
    }
}

void TranslatorVisitor::processLogicalOperation(BinaryOpNode* node, VarType leftType, VarType rightType, TokenKind operation) {
    convertToBool();
    currentFunction->bytecode()->addInsn(BC_SWAP);
    convertToBool();
    currentFunction->bytecode()->addInsn(BC_SWAP);
    switch (operation) {
        case tAND:
            currentFunction->bytecode()->addInsn(BC_IMUL);
            break;
        case tOR:
            currentFunction->bytecode()->addInsn(BC_IADD);
            break;
        default:
            throw TranslationError(std::string("Unsupported logical operation: ") + tokenOp(operation), node->position());
    }
}

void TranslatorVisitor::processBitwiseOperation(BinaryOpNode* node, VarType leftType, VarType rightType, TokenKind operation) {
    if (leftType == VT_DOUBLE) {
        currentFunction->bytecode()->addInsn(BC_D2I);
        lastExpressionType = VT_INT;
    }
    
    if (rightType == VT_DOUBLE) {
        currentFunction->bytecode()->addInsn(BC_SWAP);
        currentFunction->bytecode()->addInsn(BC_D2I);
        currentFunction->bytecode()->addInsn(BC_SWAP);
    }
    
    switch (operation) {
        case tAAND:
            currentFunction->bytecode()->addInsn(BC_IAAND);
            break;
        case tAOR:
            currentFunction->bytecode()->addInsn(BC_IAOR);
            break;
        case tAXOR:
            currentFunction->bytecode()->addInsn(BC_IAXOR);
            break;
        default:
            throw TranslationError(std::string("Unsupported bitwise operation: ") + tokenOp(operation), node->position());
    }
}

void TranslatorVisitor::processRelationalOperation(BinaryOpNode* node, VarType leftType, VarType rightType, TokenKind operation) {
    if (leftType == VT_DOUBLE) {
        currentFunction->bytecode()->addInsn(BC_D2I);
        lastExpressionType = VT_INT;
    }
    if (rightType == VT_DOUBLE) {
        currentFunction->bytecode()->addInsn(BC_SWAP);
        currentFunction->bytecode()->addInsn(BC_D2I);
        currentFunction->bytecode()->addInsn(BC_SWAP);
    }
    Label setFalse(currentFunction->bytecode());
    Label setTrue(currentFunction->bytecode());
    switch (node->kind()) {
        case tEQ:
            currentFunction->bytecode()->addBranch(BC_IFICMPE, setTrue);
            break;
        case tNEQ:
            currentFunction->bytecode()->addBranch(BC_IFICMPNE, setTrue);
            break;
        case tGE:
            currentFunction->bytecode()->addBranch(BC_IFICMPGE, setTrue);
            break;
        case tGT:
            currentFunction->bytecode()->addBranch(BC_IFICMPG, setTrue);
            break;
        case tLE:
            currentFunction->bytecode()->addBranch(BC_IFICMPLE, setTrue);
            break;
        case tLT:
            currentFunction->bytecode()->addBranch(BC_IFICMPL, setTrue);
            break;
        default:
            throw TranslationError(std::string("Unsupported relational operation: ") + tokenOp(operation), node->position());
    }
    currentFunction->bytecode()->addInsn(BC_ILOAD0);
    currentFunction->bytecode()->addBranch(BC_JA, setFalse);
    currentFunction->bytecode()->bind(setTrue);
    currentFunction->bytecode()->addInsn(BC_ILOAD1);
    currentFunction->bytecode()->bind(setFalse);
}
