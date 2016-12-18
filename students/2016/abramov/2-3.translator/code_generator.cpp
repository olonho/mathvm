#include <stdexcept>
#include <dlfcn.h>
#include <cmath>

#include "code_generator.h"
#include "helpers.h"
#include "mathvm.h"

using namespace mathvm;

CodeGenerator::CodeGenerator(Code* input)
    : _code(input)
    , _context(nullptr)
{}

CodeGenerator::~CodeGenerator()
{}

void CodeGenerator::visitBinaryOpNode(BinaryOpNode* node)
{
    const TokenKind kind = node->kind();
    switch (kind)
    {
        case tOR:
        case tAND:
            handleLogicalOp(node);
            break;
        case tAOR:
        case tAAND:
        case tAXOR:
            handleBitOp(node);
            break;
        case tADD:
        case tSUB:
        case tDIV:
        case tMUL:
        case tMOD:
            handleArithmeticOp(node);
            break;
        case tEQ:
        case tNEQ:
        case tGE:
        case tLE:
        case tGT:
        case tLT:
            handleCompareOp(node);
            break;
        default:
            throw GeneratorException("Invalid Binary Operation");
    }
}

void CodeGenerator::visitUnaryOpNode(UnaryOpNode* node)
{
    const TokenKind kind = node->kind();
    switch (kind) 
    {
    case tSUB:
        handleNegOp(node);
        break;
    case tNOT:
        handleNotOp(node);
        break;
    default:
        throw GeneratorException("Illegal unary operator");
    }
}

void CodeGenerator::visitStringLiteralNode(StringLiteralNode* node)
{
    uint16_t const stringId = _code->makeStringConstant(node->literal());
    getBytecode()->addInsn(BC_SLOAD);
    getBytecode()->addUInt16(stringId);
    _tosType = VT_STRING;
}

void CodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
    const double literal = node->literal();
    getBytecode()->addInsn(BC_DLOAD);
    getBytecode()->addDouble(literal);
    _tosType = VT_DOUBLE;
}

void CodeGenerator::visitIntLiteralNode(IntLiteralNode* node)
{
    const int64_t literal = node->literal();
    getBytecode()->addInsn(BC_ILOAD);
    getBytecode()->addInt64(literal);
    _tosType = VT_INT;
}

void CodeGenerator::visitLoadNode(LoadNode* node)
{
    loadVariable(node->var());
}

void CodeGenerator::visitStoreNode(StoreNode* node)
{
    node->value()->visit(this);
    const VarType varType = node->var()->type();
    castTypeTOS(varType);
    
    if (node->op() == tINCRSET || node->op() == tDECRSET)
    {
        loadVariable(node->var());
        handleOnEqOp(node->op(), varType);
    }
    storeVariable(node->var());
}

void CodeGenerator::visitForNode(ForNode* node)
{        
    BinaryOpNode* expr = node->inExpr()->asBinaryOpNode();
    expr->left()->visit(this);
    castTypeTOS(VT_INT); // TODO: check
    storeVariable(node->var());
    Label start = Label(getBytecode());
    getBytecode()->bind(start);
    
    expr->right()->visit(this);
    castTypeTOS(VT_INT);
    loadVariable(node->var());
    Label end = Label(getBytecode());
    getBytecode()->addBranch(BC_IFICMPG, end);
    
    // body
    node->body()->visit(this);
    
    // counter
    loadVariable(node->var());
    getBytecode()->addInsn(BC_ILOAD1);
    getBytecode()->addInsn(BC_IADD);
    storeVariable(node->var());
    
    // next iteration
    getBytecode()->addBranch(BC_JA, start);
    // out
    getBytecode()->bind(end);

    _tosType = VT_VOID;   
}

void CodeGenerator::visitWhileNode(WhileNode* node)
{
    Label start(getBytecode()->currentLabel());
    node->whileExpr()->visit(this);

    // checks condition
    Label end(getBytecode());
    getBytecode()->addInsn(BC_ILOAD0);
    getBytecode()->addBranch(BC_IFICMPE, end);

    // visits body
    node->loopBlock()->visit(this);
    getBytecode()->addBranch(BC_JA, start);
    // out
    getBytecode()->bind(end);
}

void CodeGenerator::visitIfNode(IfNode* node)
{
    Label elseBlock(getBytecode());
    // checks condition
    node->ifExpr()->visit(this);
    getBytecode()->addInsn(BC_ILOAD0);
    // gets "false"
    getBytecode()->addBranch(BC_IFICMPE, elseBlock);
    
    node->thenBlock()->visit(this);
    if (node->elseBlock()) 
    {
        Label presentElse(getBytecode());
        getBytecode()->addBranch(BC_JA, presentElse);
        getBytecode()->bind(elseBlock);
        node->elseBlock()->visit(this);
        getBytecode()->bind(presentElse);
    }
    else
    {
        getBytecode()->bind(elseBlock);
    }
}

void CodeGenerator::visitBlockNode(BlockNode* node)
{
    Scope* scope = node->scope();
    for (Scope::VarIterator varIterator(scope); varIterator.hasNext(); )
    {
        _context->addVariable(varIterator.next());
    }

    for (Scope::FunctionIterator functionIterator(scope); functionIterator.hasNext(); ) 
    {
        AstFunction* astFunction = functionIterator.next();
        const string& funcitonName = astFunction->name();
        if (!_code->functionByName(funcitonName)) 
        {
            _code->addFunction(new BytecodeFunction(astFunction));
        } 
    }

    for (Scope::FunctionIterator functionIterator = Scope::FunctionIterator(node->scope()); functionIterator.hasNext(); ) 
    {
        AstFunction* astFunction = functionIterator.next();
        BytecodeFunction* bytecodeFunction = (BytecodeFunction*) _code->functionByName(astFunction->name());
        processFunction(astFunction,  bytecodeFunction);
    }

    for (size_t i = 0; i < node->nodes(); ++i) 
    {
        node->nodeAt(i)->visit(this);
    }
    
    _tosType = VT_VOID;
}

void CodeGenerator::visitFunctionNode(FunctionNode* node)
{
    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) 
    {
        node->body()->nodeAt(0)->visit(this);
    } 
    else 
    {
        node->body()->visit(this);
    }
    
    _tosType = node->returnType();
}

void CodeGenerator::visitReturnNode(ReturnNode* node)
{
    const VarType returnType = _context->getBytecodeFunction()->returnType();
    if (node->returnExpr()) 
    {
        node->returnExpr()->visit(this);
        castTypeTOS(returnType);
    }
    
    _tosType = returnType;
    getBytecode()->addInsn(BC_RETURN);
}

void CodeGenerator::visitCallNode(CallNode* node)
{
    BytecodeFunction* function = (BytecodeFunction*) _code->functionByName(node->name());
    if (!function || node->parametersNumber() != function->parametersNumber()) 
    {
        throw GeneratorException("Function is not found: " + node->name());
    }
    for (int i = node->parametersNumber() - 1; i >= 0; --i) 
    {
        uint32_t index = std::abs(i);
        node->parameterAt(index)->visit(this);
        castTypeTOS(function->parameterType(index));
    }
    
    getBytecode()->addInsn(BC_CALL);
    getBytecode()->addUInt16(function->id());
    
    _tosType = function->returnType();
}

void CodeGenerator::visitNativeCallNode(NativeCallNode* node)
{
    void* nativeCode = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (!nativeCode) 
    {
        throw GeneratorException("Wrong native function: " + node->nativeName());
    }
    
    uint16_t functionId = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), nativeCode);
    getBytecode()->addInsn(BC_CALLNATIVE);
    getBytecode()->addUInt16(functionId);

    _tosType = node->nativeSignature()[0].first;
}

void CodeGenerator::visitPrintNode(PrintNode* node)
{
    for (size_t i = 0; i < node->operands(); ++i) 
    {
        node->operandAt(i)->visit(this);
        // _tosType was updated
        const Instruction printInstruction = getPrintInstruction(_tosType);
        getBytecode()->addInsn(printInstruction);
    }
}

Status* CodeGenerator::handleFunction(AstFunction* functionStart) 
{
    BytecodeFunction* bytecodeStart = new BytecodeFunction(functionStart);
    _code->addFunction(bytecodeStart);
    processFunction(functionStart, bytecodeStart);
    
    return Status::Ok();
}

void CodeGenerator::handleLogicalOp(BinaryOpNode* node) 
{
    const TokenKind kind = node->kind();
    
    node->left()->visit(this);
    castTypeTOS(VT_INT);
    
    Instruction instruction = (kind == tAND) ? BC_IFICMPE : BC_IFICMPNE;
    getBytecode()->addInsn(BC_ILOAD0);
    
    Label start(getBytecode());
    getBytecode()->addBranch(instruction, start);
    
    node->right()->visit(this);
    castTypeTOS(VT_INT);
    
    Label end(getBytecode());
    getBytecode()->addBranch(BC_JA, end);
    
    getBytecode()->bind(start);
    getBytecode()->addInsn(kind == tAND ? BC_ILOAD0 : BC_ILOAD1);
    getBytecode()->bind(end);
}

void CodeGenerator::handleBitOp(BinaryOpNode* node) 
{
    const auto types = visitTwoArguments(node);
    VarType rightType = types.first;
    VarType leftType = types.second;
    
    if (rightType != VT_INT || leftType != VT_INT)
    {
        throw GeneratorException("Invalid type for bit operation");
    }
    
    const Instruction bitInstruction = getBitInstruction(node->kind());
    getBytecode()->addInsn(bitInstruction);
}

void CodeGenerator::handleArithmeticOp(BinaryOpNode* node) 
{
    const auto types = visitTwoArguments(node);
    VarType rightType = types.first;
    VarType leftType = types.second;
    
    const TokenKind kind = node->kind();
    if (kind == TokenKind::tMOD)
    {
        if (!(rightType == VT_INT && leftType == VT_INT))
        {
            throw GeneratorException("Invalid type for MOD operation");
        }
    }
    
    const VarType arithmeticOpType = castBinaryOp(leftType, rightType);
    const Instruction arithmeticInst = getArithmeticInstruction(kind, arithmeticOpType);
    getBytecode()->addInsn(arithmeticInst);
}

void CodeGenerator::handleCompareOp(BinaryOpNode* node) 
{
    const auto types = visitTwoArguments(node);
    VarType rightType = types.first;
    VarType leftType = types.second;
    VarType compareOpType = castBinaryOp(leftType, rightType);
    const TokenKind kind = node->kind();
    
    getBytecode()->addInsn(compareOpType == VT_INT ? BC_ICMP : BC_DCMP);
    
    getBytecode()->addInsn(BC_ILOAD0);
    getBytecode()->addInsn(BC_SWAP);

    Label start = Label(getBytecode());
    Label end = Label(getBytecode());

    const Instruction compareInst = getComapreInstruction(kind);
    getBytecode()->addBranch(compareInst, start);
    getBytecode()->addInsn(BC_ILOAD0);
    getBytecode()->addBranch(BC_JA, end);

    getBytecode()->bind(start);
    getBytecode()->addInsn(BC_ILOAD1);
    getBytecode()->bind(end);
    
}

std::pair<VarType, VarType> CodeGenerator::visitTwoArguments(BinaryOpNode* node) 
{
    node->right()->visit(this);
    VarType firstType = _tosType;
    node->left()->visit(this);
    VarType secondType = _tosType;
    
    return std::make_pair(firstType, secondType);
}

void CodeGenerator::handleNegOp(UnaryOpNode* node) 
{
    node->operand()->visit(this);
    if (_tosType == VT_INT)
    {
        getBytecode()->addInsn(BC_INEG);
    } else if (_tosType == VT_DOUBLE)
    {
        getBytecode()->addInsn(BC_DNEG);
    } else
    {
        throw GeneratorException("Invalid type for tSUB operation"); 
    }
}

void CodeGenerator::handleNotOp(UnaryOpNode* node) 
{
    node->operand()->visit(this);
    if (_tosType != VT_INT) 
    {
        throw GeneratorException("Invalid type for Not Unary operation");
    }
    
    getBytecode()->addInsn(BC_ILOAD0);
    Label start = Label(getBytecode());
    
    getBytecode()->addBranch(BC_IFICMPE, start);
    getBytecode()->addInsn(BC_ILOAD0);
    
    Label end = Label(getBytecode());
    getBytecode()->addBranch(BC_JA, end);
    
    getBytecode()->bind(start);
    getBytecode()->addInsn(BC_ILOAD1);
    getBytecode()->bind(end);
    
    _tosType = VT_INT;
}

void CodeGenerator::handleOnEqOp(TokenKind kind, VarType type) 
{
    if (kind == tINCRSET)
    {
        const Instruction addInstuction = (type == VT_INT) ? BC_IADD : BC_DADD;
        getBytecode()->addInsn(addInstuction);
        return;
    }
    
    if (kind == tDECRSET)
    {
        const Instruction subInstuction = (type == VT_INT) ? BC_ISUB : BC_DSUB;
        getBytecode()->addInsn(subInstuction);
        return;
    }
}

void CodeGenerator::loadVariable(const AstVar* variable) 
{
    const std::string& name = variable->name();
    const VarType varType = variable->type();
    
    const Context::VarInfo info = _context->getVariable(name);
    if (info._contextId == _context->getContextId()) 
    {
        const auto localInst = getLocalLoadInstruction(varType);
        getBytecode()->addInsn(localInst);
    } 
    else 
    {
        const auto scopeInst = getContextLoadInstruction(varType);
        getBytecode()->addInsn(scopeInst);
        getBytecode()->addUInt16(info._contextId);
    }

    _tosType = varType;
    getBytecode()->addUInt16(info._varId);
}

void CodeGenerator::storeVariable(const AstVar* variable) 
{
    const std::string& name = variable->name();
    const VarType varType = variable->type();
    
    const Context::VarInfo info = _context->getVariable(name);
    if (info._contextId == _context->getContextId()) 
    {
        const auto localInst = getLocalStoreInstruction(varType);
        getBytecode()->addInsn(localInst);
    } 
    else 
    {
        const auto scopeInst = getContexStoreInstruction(varType);
        getBytecode()->addInsn(scopeInst);
        getBytecode()->addUInt16(info._contextId);
    }

    _tosType = varType;
    getBytecode()->addUInt16(info._varId);
}

void CodeGenerator::processFunction(AstFunction* astFunc, BytecodeFunction* bytecodeFunc) 
{
    Context* newContext = new Context(bytecodeFunc, astFunc->scope(), _context);
    _context = newContext;

    for (size_t i = 0; i < astFunc->parametersNumber(); ++i) 
    {
        const std::string& parameterName = astFunc->parameterName(i);
        AstVar* var = astFunc->scope()->lookupVariable(parameterName, false);
        storeVariable(var);
    }
    
    astFunc->node()->visit(this);
    
    bytecodeFunc->setLocalsNumber(_context->getVarsSize());
    bytecodeFunc->setScopeId(_context->getContextId());
    _context = newContext->getParentContext();
    
    // free created context
    delete newContext;
}

void CodeGenerator::castTypeTOS(VarType target) 
{
    if (target == _tosType) 
    {
        return;
    }
    if (_tosType == VT_INT && target == VT_DOUBLE) 
    {
        getBytecode()->addInsn(BC_I2D);
        return;
    }
    else if (_tosType == VT_DOUBLE && target == VT_INT) 
    {
        
        getBytecode()->addInsn(BC_D2I);
        return;
    }
    else
    {
        throw GeneratorException("Invalid type cast for TOS");
    }
}

VarType CodeGenerator::castBinaryOp(VarType lhs, VarType rhs)
{
    if (lhs == rhs) 
    {
        return lhs;
    }
    if (rhs == VT_DOUBLE && lhs == VT_INT) 
    {
        getBytecode()->addInsn(BC_I2D);
        
        return VT_DOUBLE;
    }
    if (rhs == VT_INT && lhs == VT_DOUBLE) 
    {
        getBytecode()->addInsn(BC_SWAP);
        getBytecode()->addInsn(BC_I2D);
        getBytecode()->addInsn(BC_SWAP);
        
        return VT_DOUBLE;
    }
    
    throw GeneratorException("Invalid types in binary operation");
}

Bytecode* CodeGenerator::getBytecode()
{
    return _context->getBytecodeFunction()->bytecode();
}