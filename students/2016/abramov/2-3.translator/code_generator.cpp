#include <stdexcept>

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
    _tosType = VT_STRING;
    
    getBytecode()->addInsn(BC_SLOAD);
    getBytecode()->addUInt16(stringId);
}

void CodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
    const double literal = node->literal();
    _tosType = VT_DOUBLE;
    
    getBytecode()->addInsn(BC_DLOAD);
    getBytecode()->addDouble(literal);
}

void CodeGenerator::visitIntLiteralNode(IntLiteralNode* node)
{
    const int64_t literal = node->literal();
    _tosType = VT_INT;
    
    getBytecode()->addInsn(BC_ILOAD);
    getBytecode()->addInt64(literal);
}

void CodeGenerator::visitLoadNode(LoadNode* node)
{
    const std::string& name = node->var()->name();
    const VarType varType = node->var()->type();
    
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

void CodeGenerator::visitStoreNode(StoreNode* node)
{
    // TODO
}

void CodeGenerator::visitForNode(ForNode* node)
{
    // TODO
}

void CodeGenerator::visitWhileNode(WhileNode* node)
{
    // TODO
}

void CodeGenerator::visitIfNode(IfNode* node)
{
    // TODO
}

void CodeGenerator::visitBlockNode(BlockNode* node)
{
    // TODO
}

void CodeGenerator::visitFunctionNode(FunctionNode* node)
{
    // TODO
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
    // TODO
}

void CodeGenerator::visitNativeCallNode(NativeCallNode* node)
{
    // TODO
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

Status* CodeGenerator::addFunction(AstFunction* function) 
{
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

void CodeGenerator::castTypeTOS(VarType target) 
{    
    if (_tosType == VT_INT && target == VT_DOUBLE) {
        getBytecode()->addInsn(BC_I2D);
        return;
    }
    else if (_tosType == VT_DOUBLE && target == VT_INT) {
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











