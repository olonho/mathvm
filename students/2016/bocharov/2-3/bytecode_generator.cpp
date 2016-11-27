#include "bytecode_generator.h"

#include <dlfcn.h>

namespace mathvm
{

std::map<VarType, std::map<TokenKind, Instruction>> BytecodeGeneratorVisitor::instructions = {
    { VT_INT, { { tAAND, BC_IAAND }, { tAOR, BC_IAOR }, { tAXOR, BC_IAXOR }, { tADD, BC_IADD },
                { tSUB , BC_ISUB }, { tMUL, BC_IMUL }, { tDIV, BC_IDIV }, { tMOD, BC_IMOD },
                { tINCRSET, BC_IADD }, { tDECRSET, BC_ISUB } } },
    { VT_DOUBLE, { { tADD, BC_DADD }, { tSUB , BC_DSUB }, { tMUL, BC_DMUL }, { tDIV, BC_DDIV },
                { tINCRSET, BC_DADD }, { tDECRSET, BC_DSUB } } }
};


template<>
void BytecodeGeneratorVisitor::generateLiteral<int64_t>(const int64_t& val)
{
    bytecode()->addInsn(BC_ILOAD);
    bytecode()->addInt64(val);
    setLastExprType(VT_INT);
}

template<>
void BytecodeGeneratorVisitor::generateLiteral<double>(const double& val)
{
    bytecode()->addInsn(BC_DLOAD);
    bytecode()->addDouble(val);
    setLastExprType(VT_DOUBLE);
}

template<>
void BytecodeGeneratorVisitor::generateLiteral<std::string>(const std::string& val)
{
    bytecode()->addInsn(BC_SLOAD);
    uint16_t id = m_code->makeStringConstant(val);
    bytecode()->addUInt16(id);
    setLastExprType(VT_STRING);
}


BytecodeGeneratorVisitor::BytecodeGeneratorVisitor(Code * code)
    : m_code(code)
{}

void BytecodeGeneratorVisitor::generateCode(AstFunction * top)
{
    pushContext(nullptr);

    declareFunction(top);
    top->node()->visit(this);

    popContext();
}

void BytecodeGeneratorVisitor::visitUnaryOpNode(UnaryOpNode * node)
{
    TokenKind op = node->kind();
    switch (op) {
        case tNOT:
            generateNotOp(node);
            break;
        case tSUB:
            generateNegateOp(node);
            break;
        default: {
            std::string message = "unknown token type: " + std::string(tokenStr(op));
            throw CodeGenerationError(message, node->position());
        }
    }
}

void BytecodeGeneratorVisitor::visitBinaryOpNode(BinaryOpNode * node)
{
    TokenKind op = node->kind();
    switch (op) {
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            generateArithmeticOp(node);
            break;
        case tAAND:
        case tAOR:
        case tAXOR:
            generateBitOp(node);
            break;
        case tEQ:
        case tNEQ:
        case tGE:
        case tGT:
        case tLE:
        case tLT:
            generateCompareOp(node);
            break;
        case tAND:
        case tOR:
            generateLogicOp(node);
            break;
        default: {
            std::string message = "unknown token type: " + std::string(tokenStr(op));
            throw CodeGenerationError(message, node->position());
        }
    }
}

void BytecodeGeneratorVisitor::visitDoubleLiteralNode(DoubleLiteralNode * node)
{
    generateLiteral(node->literal());
}

void BytecodeGeneratorVisitor::visitIntLiteralNode(IntLiteralNode * node)
{
    generateLiteral(node->literal());
}

void BytecodeGeneratorVisitor::visitStringLiteralNode(StringLiteralNode * node)
{
    generateLiteral(node->literal());
}

void BytecodeGeneratorVisitor::visitIfNode(IfNode * node)
{
    generateIfElse(node);
}

void BytecodeGeneratorVisitor::visitForNode(ForNode * node)
{
    BinaryOpNode * range = node->inExpr()->asBinaryOpNode();
    if (!range) {
        throw CodeGenerationError("in statement should be binary node", node->position());
    }

    if (node->var()->type() != VT_INT) {
        throw CodeGenerationError("for cylcle var should be int", node->position());
    }

    generateFor(node->var(), range, node->body());
}

void BytecodeGeneratorVisitor::visitWhileNode(WhileNode * node)
{
    generateWhile(node);
}

void BytecodeGeneratorVisitor::visitLoadNode(LoadNode * node)
{
    generateLoad(node->var());
}

void BytecodeGeneratorVisitor::visitStoreNode(StoreNode * node)
{
    node->value()->visit(this);
    VarType type = node->var()->type();
    generateCast(type);

    TokenKind op = node->op();
    switch (op) {
        case tINCRSET:
        case tDECRSET:
            generateLoad(node->var());
            bytecode()->addInsn(instructions[type][op]);
            break;
        case tASSIGN:
            break;
        default: {
            std::string message = std::string("wrong store operation: ") + tokenOp(op);
            throw CodeGenerationError(message, node->position());
        }
    }

    generateStore(node->var());
}

void BytecodeGeneratorVisitor::visitPrintNode(PrintNode * node)
{
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        generatePrint();
    }
}

void BytecodeGeneratorVisitor::visitCallNode(CallNode * node)
{
    AstFunction * function = topCtx()->getFunction(node->name());
    if (!function) {
        auto message = std::string("function ") + node->name() + " doesn't exists";
        throw CodeGenerationError(message, node->position());
    }

    if (function->parametersNumber() != node->parametersNumber()) {
        std::string message = "wrong number of parameters for call " + node->name();
        throw CodeGenerationError(message, node->position());
    }

    for (uint32_t i = node->parametersNumber(); i > 0; --i) {
        node->parameterAt(i - 1)->visit(this);
        generateCast(function->parameterType(i - 1));
    }

    uint16_t id = ((BytecodeFunction *) function->info())->scopeId();
    if (function->node()->body()->nodes() > 0 &&
            function->node()->body()->nodeAt(0)->isNativeCallNode()) {
        generateNativeCall(id);
    } else {
        generateCall(id);
    }

    setLastExprType(function->returnType());
}

void BytecodeGeneratorVisitor::visitNativeCallNode(NativeCallNode * node)
{
    auto name = node->nativeName();
    void * sym = dlsym(RTLD_DEFAULT, name.c_str());
    if (!sym) {
        std::string message = "couldn't find native with name " + name;
        throw CodeGenerationError(message, node->position());
    }
    uint64_t id = m_code->makeNativeFunction(name, node->nativeSignature(), sym);
    topCtx()->getFunction(name)->setInfo(reinterpret_cast<void *>(id));
}

void BytecodeGeneratorVisitor::visitReturnNode(ReturnNode * node)
{
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
    }
    generateReturn(topCtx()->returnType());
}

void BytecodeGeneratorVisitor::visitBlockNode(BlockNode * node)
{
    Scope::VarIterator vars = node->scope();
    while (vars.hasNext()) {
        topCtx()->addVar(vars.next()->name());
    }
    topCtx()->function()->setLocalsNumber(topCtx()->localsNumber());

    Scope::FunctionIterator functions = node->scope();
    while (functions.hasNext()) {
        auto fn = functions.next();
        declareFunction(fn);
    }

    functions = node->scope();
    while (functions.hasNext()) {
        auto fn = functions.next();
        fn->node()->visit(this);
    }

    node->visitChildren(this);
}

void BytecodeGeneratorVisitor::visitFunctionNode(FunctionNode * node)
{
    auto function = topCtx()->getFunction(node->name());
    if (!function) {
        throw CodeGenerationError("function definition before declaration", node->position());
    }

    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
        return;
    }

    pushContext((BytecodeFunction *) function->info());
    for (uint32_t i = 0; i < function->parametersNumber(); ++i) {
        AstVar param(function->parameterName(i), function->parameterType(i), nullptr);
        topCtx()->addVar(param.name());
        generateStore(&param);
    }
    node->body()->visit(this);

    popContext();
}


Bytecode * BytecodeGeneratorVisitor::bytecode()
{
    return topCtx()->bytecode();
}

void BytecodeGeneratorVisitor::declareFunction(AstFunction * fn)
{
    topCtx()->addFunction(fn);

    BytecodeFunction * bcf = new BytecodeFunction(fn);
    uint16_t id = m_code->addFunction(bcf);
    bcf->setScopeId(id);
    fn->setInfo(bcf);
}

void BytecodeGeneratorVisitor::generateIfElse(IfNode * node)
{
//    node->ifExpr()->visit(this);
//    generateCast(VT_INT);

//    Label elseBranch(bytecode());
//    Label ifElseEnd(bytecode());

//    bytecode()->addInsn(BC_ILOAD1);
//    bytecode()->addBranch(BC_IFICMPNE, elseBranch);
//    node->thenBlock()->visit(this);
//    bytecode()->addBranch(BC_JA, ifElseEnd);

//    bytecode()->bind(elseBranch);
//    if (node->elseBlock())
//        node->elseBlock()->visit(this);
//    bytecode()->bind(ifElseEnd);

    Label unlessLabel(bytecode());
    node->ifExpr()->visit(this);
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, unlessLabel);

    node->thenBlock()->visit(this);

    if (node->elseBlock() != 0) {
        Label afterElse(bytecode());
        bytecode()->addBranch(BC_JA, afterElse);
        bytecode()->bind(unlessLabel);
        node->elseBlock()->visit(this);
        bytecode()->bind(afterElse);
    } else {
        bytecode()->bind(unlessLabel);
    }
}

void BytecodeGeneratorVisitor::generateWhile(WhileNode * node)
{
    Label loopStart(bytecode());
    Label loopEnd(bytecode());

    bytecode()->bind(loopStart);
    node->whileExpr()->visit(this);
    generateCast(VT_INT);
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, loopEnd);
    node->loopBlock()->visit(this);
    bytecode()->addBranch(BC_JA, loopStart);
    bytecode()->bind(loopEnd);
}

void BytecodeGeneratorVisitor::generateFor(AstVar const * var, BinaryOpNode * range, BlockNode * body)
{
    // for(
    // i = left;
    range->left()->visit(this);
    generateStore(var);

    Label loopStart(bytecode());
    Label loopEnd(bytecode());

    // i < right
    bytecode()->bind(loopStart);
    generateLoad(var);
    range->right()->visit(this);
    bytecode()->addBranch(BC_IFICMPL, loopEnd);

    body->visit(this);

    // i += 1)
    generateLoad(var);
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addInsn(BC_IADD);
    generateStore(var);

    bytecode()->addBranch(BC_JA, loopStart);
    bytecode()->bind(loopEnd);
}


void BytecodeGeneratorVisitor::generateArithmeticOp(BinaryOpNode * node)
{
    node->right()->visit(this);
    VarType rightType = lastExprType();
    node->left()->visit(this);
    VarType leftType = lastExprType();
    VarType type = leftType;
    if (leftType != rightType) {
        // c-style cast (if any is double -> all double)
        if (leftType == VT_DOUBLE) {
            // topmost is double -> need to swap and cast
            bytecode()->addInsn(BC_SWAP);
            setLastExprType(VT_INT);
        }
        generateCast(VT_DOUBLE);

        if (leftType == VT_DOUBLE) {
            // need to swap backward to save semantic
            bytecode()->addInsn(BC_SWAP);
            setLastExprType(VT_DOUBLE);
        }

        type = VT_DOUBLE;
    }

    bytecode()->addInsn(instructions[type][node->kind()]);
    setLastExprType(type);
}

void BytecodeGeneratorVisitor::generateBitOp(BinaryOpNode * node)
{
    node->left()->visit(this);
    VarType leftType = lastExprType();
    if (leftType != VT_INT) {
        throw CodeGenerationError("bit operations allowed only for int type", node->position());
    }
    node->right()->visit(this);
    VarType rightType = lastExprType();
    if (rightType != VT_INT) {
        throw CodeGenerationError("bit operations allowed only for int type", node->position());
    }

    VarType type = VT_INT;
    bytecode()->addInsn(instructions[type][node->kind()]);
}

void BytecodeGeneratorVisitor::generateCompareOp(BinaryOpNode * node)
{
    Instruction insn = BC_INVALID;
    switch (node->kind()) {
    case tEQ:
        insn = BC_IFICMPE;
        break;
    case tNEQ:
        insn = BC_IFICMPLE;
        break;
    case tLT:
        insn = BC_IFICMPL;
        break;
    case tLE:
        insn = BC_IFICMPLE;
        break;
    case tGT:
        insn = BC_IFICMPG;
        break;
    case tGE:
        insn = BC_IFICMPGE;
        break;
    default:
        throw CodeGenerationError("unknown cmp operation", node->position());
    }

    node->right()->visit(this);
    generateCast(VT_INT);
    node->left()->visit(this);
    generateCast(VT_INT);

    Label cmpTrue(bytecode());
    Label cmpFalse(bytecode());

    bytecode()->addBranch(insn, cmpTrue);
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_JA, cmpFalse);
    bytecode()->bind(cmpTrue);
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->bind(cmpFalse);

    setLastExprType(VT_INT);
}

void BytecodeGeneratorVisitor::generateLogicOp(BinaryOpNode * node)
{
    node->left()->visit(this);
    generateCast(VT_INT);

    Instruction eqInsn = BC_INVALID;
    switch (node->kind()) {
    case tAND:
        bytecode()->addInsn(BC_ILOAD0);
        eqInsn = BC_ILOAD0;
        break;
    case tOR:
        bytecode()->addInsn(BC_ILOAD1);
        eqInsn = BC_ILOAD1;
        break;
    default:
        throw CodeGenerationError("unknown logic op", node->position());
    }

    Label done(bytecode());
    Label firstDecide(bytecode());
    bytecode()->addBranch(BC_IFICMPE, firstDecide);
    node->right()->visit(this);
    generateCast(VT_INT);
    bytecode()->addBranch(BC_JA, done);

    bytecode()->bind(firstDecide);
    bytecode()->addInsn(eqInsn);

    bytecode()->bind(done);

    setLastExprType(VT_INT);
}

void BytecodeGeneratorVisitor::generateNotOp(UnaryOpNode * node)
{
    node->operand()->visit(this);

    if (lastExprType() != VT_INT) {
        throw CodeGenerationError("can't revert not int type", node->position());
    }

    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addInsn(BC_IAXOR);

    assert(m_lastExprType == VT_INT);
}

void BytecodeGeneratorVisitor::generateNegateOp(UnaryOpNode * node)
{
    node->operand()->visit(this);
    VarType type = lastExprType();
    switch (type) {
        case VT_INT:
            bytecode()->addInsn(BC_INEG);
            break;
        case VT_DOUBLE:
            bytecode()->addInsn(BC_DNEG);
            break;
        default: {
            std::string message = std::string("can't negate type ") + typeToName(type);
            throw CodeGenerationError(message, node->position());
        }
    }

    assert(m_lastExprType == type);
}

void BytecodeGeneratorVisitor::generateCall(uint16_t fid)
{
    bytecode()->addInsn(BC_CALL);
    bytecode()->addUInt16(fid);
}

void BytecodeGeneratorVisitor::generateNativeCall(uint16_t fid)
{
    bytecode()->addInsn(BC_CALLNATIVE);
    bytecode()->addUInt16(fid);
}

void BytecodeGeneratorVisitor::generateCast(VarType type)
{
    if (lastExprType() == type)
        return;

    if ((type != VT_DOUBLE && type != VT_INT) ||
            (lastExprType() != VT_DOUBLE && lastExprType() != VT_INT)) {
        char message[100];
        sprintf(message, "can't cast type %s to type %s", typeToName(lastExprType()), typeToName(type));
        throw CodeGenerationError(message, Status::INVALID_POSITION);
    }

    switch (type) {
    case VT_INT:
        bytecode()->addInsn(BC_D2I);
        break;
    case VT_DOUBLE:
        bytecode()->addInsn(BC_I2D);
        break;
    default:
        throw std::runtime_error("can't happen");
    }

    setLastExprType(type);
}

void BytecodeGeneratorVisitor::generatePrint()
{
    VarType type = lastExprType();
    switch (type) {
        case VT_INT:
            bytecode()->addInsn(BC_IPRINT);
            break;
        case VT_DOUBLE:
            bytecode()->addInsn(BC_DPRINT);
            break;
        case VT_STRING:
            bytecode()->addInsn(BC_SPRINT);
            break;
        default: {
            std::string message = std::string("invalid type for print: ") + typeToName(type);
            throw CodeGenerationError(message, Status::INVALID_POSITION);
        }
    }
}

void BytecodeGeneratorVisitor::generateReturn(VarType type)
{
    if (type != VT_VOID)
        generateCast(type);
    bytecode()->addInsn(BC_RETURN);
    setLastExprType(type);
}

void BytecodeGeneratorVisitor::generateLoad(const AstVar * var)
{
    Instruction ins1 = BC_INVALID;
    Instruction ins2 = BC_INVALID;

    VarType type = var->type();
    switch (type) {
        case VT_INT:
            ins1 = BC_LOADIVAR;
            ins2 = BC_LOADCTXIVAR;
            break;
        case VT_DOUBLE:
            ins1 = BC_LOADDVAR;
            ins2 = BC_LOADCTXDVAR;
            break;
        case VT_STRING:
            ins1 = BC_LOADSVAR;
            ins2 = BC_LOADCTXSVAR;
            break;
        default:
            std::string message = std::string("can't load type ") + typeToName(type);
            throw CodeGenerationError(message, Status::INVALID_POSITION);
    }

    Location loc = topCtx()->location(var->name());
    if (topCtx()->isLocal(var->name())) {
        bytecode()->addInsn(ins1);
        bytecode()->addUInt16(loc.m_varId);
    } else {
        bytecode()->addInsn(ins2);
        bytecode()->addUInt16(loc.m_scopeId);
        bytecode()->addUInt16(loc.m_varId);
    }

    setLastExprType(type);
}

void BytecodeGeneratorVisitor::generateStore(const AstVar * var)
{
    Instruction ins1 = BC_INVALID;
    Instruction ins2 = BC_INVALID;

    VarType type = var->type();
    switch (type) {
        case VT_INT:
            ins1 = BC_STOREIVAR;
            ins2 = BC_STORECTXIVAR;
            break;
        case VT_DOUBLE:
            ins1 = BC_STOREDVAR;
            ins2 = BC_STORECTXDVAR;
            break;
        case VT_STRING:
            ins1 = BC_STORESVAR;
            ins2 = BC_STORECTXSVAR;
            break;
        default:
            std::string message = std::string("can't store type ") + typeToName(type);
            throw CodeGenerationError(message, Status::INVALID_POSITION);
    }

    Location loc = topCtx()->location(var->name());
    if (topCtx()->isLocal(var->name())) {
        bytecode()->addInsn(ins1);
        bytecode()->addUInt16(loc.m_varId);
    } else {
        bytecode()->addInsn(ins2);
        bytecode()->addUInt16(loc.m_scopeId);
        bytecode()->addUInt16(loc.m_varId);
    }
}

VarType BytecodeGeneratorVisitor::lastExprType() const
{
    return m_lastExprType;
}

void BytecodeGeneratorVisitor::setLastExprType(VarType type)
{
    m_lastExprType = type;
}

void BytecodeGeneratorVisitor::pushContext(BytecodeFunction * function)
{
    auto ctx = std::make_shared<ScopeContext>(topCtx(), function);
    m_contexts.push(ctx);
}

void BytecodeGeneratorVisitor::popContext()
{
    m_contexts.pop();
}

ScopeContextPtr BytecodeGeneratorVisitor::topCtx()
{
    return m_contexts.empty() ? nullptr : m_contexts.top();
}


}   // namespace mathvm
