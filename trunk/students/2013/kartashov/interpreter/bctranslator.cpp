#include "bctranslator.h"
#include "parser.h"

#include <dlfcn.h>

#define MAKE_INSN(b, t, e) (t == VT_INT ? b##I##e : (t == VT_DOUBLE ? b##D##e : (t == VT_STRING ? b##S##e : BC_INVALID)))
#define MAKE_INSN_NUM(b, t, e) (t == VT_INT ? b##I##e : (t == VT_DOUBLE ? b##D##e : BC_INVALID))
#define MAKE_INSN_SEL(cond, b1, b2, t, e) (cond ? MAKE_INSN(b1, t, e) : MAKE_INSN(b2, t, e))

#define SCOPEVAR_ID(v) (v.first)
#define SCOPEVAR_CTX(v) (v.second->function->id())

using namespace mathvm;

#ifdef TRANSLATOR
struct InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl() : Code() {}

    Status *execute(vector<Var *> &vars) {
        return 0;
    }
};
#else
#include "bcinterpreter.h"
#endif

//----------------------------------------------------------------

class TError {
public:
    TError(const std::string &msg, uint32_t pos = -1) : msg(msg), pos(pos) {}

    static TError logicTypeError(uint32_t p = -1)      { return TError("Unsupported type for logic operation", p); }
    static TError mathTypeError(uint32_t p = -1)       { return TError("Unsupported type for math operation", p); }
    static TError forVarError(uint32_t p = -1)         { return TError("Non integer variable in for expression", p); }
    static TError forNotRangeError(uint32_t p = -1)    { return TError("Non-range type in for expression", p); }
    static TError forRangeTypeError(uint32_t p = -1)   { return TError("Non-integer range in for expression", p); }
    static TError mathOpWithNANs(uint32_t p = -1)      { return TError("Math operation with NANs", p); }
    static TError unsupportedOperation(uint32_t p = -1) { return TError("Unsupported operation", p);
    }

    std::string msg;
    uint32_t pos;
};

//----------------------------------------------------------------

Status *BCTranslator::translate(const string &program, Code **code) {
    Parser p;
    Status *s = p.parseProgram(program);
    if(s && s->isError()) return s;

    this->code = new InterpreterCodeImpl();
    *code = this->code;
    currentScope = 0;
    tosType = VT_INVALID;
    try {
        translateFunction(p.top());
    } catch (TError e) {
        return new Status(e.msg, e.pos);
    }

    return 0;
}

void BCTranslator::translateFunction(AstFunction *f) {
//    BytecodeFunction *bcf = new BytecodeFunction(f);
    BytecodeFunction *bcf = (BytecodeFunction*)code->functionByName(f->name());
    if(!bcf) {
        bcf = new BytecodeFunction(f);
        code->addFunction(bcf);
    }
    enterScope(bcf);

    //store vars in signature order
    for(Signature::const_iterator i = bcf->signature().begin() + 1; i != bcf->signature().end(); ++i) {
        AstVar *v = f->scope()->lookupVariable(i->second);
        currentScope->addVar(v);
        storeVar(v);
    }

    f->node()->visit(this);

    if(currentScope->parent == 0) currentBC()->addInsn(BC_STOP);

    exitScope();
}

void BCTranslator::convertTOSType(VarType to) {
    if(tosType == to) return;
    if(tosType == VT_INT && to == VT_DOUBLE) currentBC()->addInsn(BC_I2D);
    else if(tosType == VT_DOUBLE && to == VT_INT) currentBC()->addInsn(BC_D2I);
    else if(tosType == VT_STRING && to == VT_INT) currentBC()->addInsn(BC_S2I);
    else {
        throw TError(std::string(typeToName(tosType)) + " to " + typeToName(to) + " type conversion is not supported");
    }
}

void BCTranslator::convertTOSToBool() {
    convertTOSType(VT_INT);

    Label setFalse(currentBC());
    Label convEnd(currentBC());
    currentBC()->addInsn(BC_ILOAD0);
    currentBC()->addBranch(BC_IFICMPE, setFalse);
    currentBC()->addInsn(BC_ILOAD1);
    currentBC()->addBranch(BC_JA, convEnd);
    currentBC()->bind(setFalse);
    currentBC()->addInsn(BC_ILOAD0);
    currentBC()->bind(convEnd);
}

//------------------------------------------------------------

void BCTranslator::visitFunctionNode(FunctionNode *node) {
    if(node->body()->nodeAt(0)->isNativeCallNode())  node->body()->nodeAt(0)->visit(this);
    else node->body()->visit(this);
}

void BCTranslator::visitBlockNode(BlockNode *node) {
    //used in interpreter
    currentScope->function->setLocalsNumber(currentScope->function->localsNumber() + node->scope()->variablesCount());

    Scope::VarIterator vars(node->scope());
    while(vars.hasNext()) currentScope->addVar(vars.next());

    //declare scope functions first
    Scope::FunctionIterator funcs(node->scope());
    while(funcs.hasNext()) code->addFunction(new BytecodeFunction(funcs.next()));

    //now we can translate scope functions
    funcs = Scope::FunctionIterator(node->scope());
    while(funcs.hasNext()) translateFunction(funcs.next());

    for(uint32_t i = 0; i < node->nodes(); ++i) node->nodeAt(i)->visit(this);

}

void BCTranslator::visitCallNode(CallNode *node) {
    TranslatedFunction *f = code->functionByName(node->name());
    if(!f) throw TError("function '" + node->name() + "' is not defined", node->position());

    for(int i = node->parametersNumber() - 1; i >= 0; --i) {
        node->parameterAt(i)->visit(this);
        convertTOSType(f->parameterType(i));
    }

    currentBC()->addInsn(BC_CALL);
    currentBC()->addUInt16(f->id());
    if(f->returnType() != VT_VOID) tosType = f->returnType();
}

void BCTranslator::visitNativeCallNode(NativeCallNode *node) {
    void *addr = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if(!addr) throw TError("Native function '" + node->nativeName() + "' not found", node->position());
    uint16_t id = code->makeNativeFunction(node->nativeName(), node->nativeSignature(), addr);

    currentBC()->addInsn(BC_CALLNATIVE);
    currentBC()->addUInt16(id);
    currentBC()->addInsn(BC_RETURN);
}

void BCTranslator::visitReturnNode(ReturnNode *node) {
    if(node->returnExpr()) {
        //we can make some optimizations: e.g. return from if block w/o JA
        node->returnExpr()->visit(this);
        convertTOSType(currentScope->function->returnType());
    }
    currentBC()->addInsn(BC_RETURN);
}

//------------------------------------------------------------

VarType BCTranslator::makeNumTypeCast(VarType ltype, VarType rtype, VarType toType, bool soft) {
    //if not soft then force cast to toType else skip cast if ltype == rtype
    if(!isNumType(ltype) || !isNumType(rtype)) throw TError::mathOpWithNANs();
    if(ltype == rtype && soft) return ltype;

    Instruction insn = toType == VT_INT ? BC_D2I : BC_I2D;
    if(ltype != toType) {
        currentBC()->addInsn(insn);
        tosType = toType;
    }
    if(rtype != toType) {
        currentBC()->addInsn(BC_SWAP);
        currentBC()->addInsn(insn);
        currentBC()->addInsn(BC_SWAP);
    }

    return toType;
}

void BCTranslator::makeComparisonOperation(TokenKind op) {
    Label setFalse(currentBC());
    Label setTrue(currentBC());

    Instruction i;
    switch(op) {
    case tEQ: i = BC_IFICMPE; break;
    case tNEQ: i = BC_IFICMPNE; break;
    case tGT: i = BC_IFICMPG; break;
    case tGE: i = BC_IFICMPGE; break;
    case tLT: i = BC_IFICMPL; break;
    case tLE: i = BC_IFICMPLE; break;
    default: throw TError::unsupportedOperation();
    }

    currentBC()->addBranch(i, setTrue);
    currentBC()->addInsn(BC_ILOAD0);
    currentBC()->addBranch(BC_JA, setFalse);
    currentBC()->bind(setTrue);
    currentBC()->addInsn(BC_ILOAD1);
    currentBC()->bind(setFalse);
    tosType = VT_INT;
}

void BCTranslator::makeMathOperation(TokenKind op) {
    switch(op) {
    case tADD: currentBC()->addInsn(MAKE_INSN_NUM(BC_, tosType, ADD)); break;
    case tSUB: currentBC()->addInsn(MAKE_INSN_NUM(BC_, tosType, SUB)); break;
    case tMUL: currentBC()->addInsn(MAKE_INSN_NUM(BC_, tosType, MUL)); break;
    case tDIV: currentBC()->addInsn(MAKE_INSN_NUM(BC_, tosType, DIV)); break;
    case tMOD:
        if(tosType != VT_INT) throw TError::mathTypeError();
        currentBC()->addInsn(BC_IMOD);
        break;
    default: throw TError::unsupportedOperation();
    }
}

void BCTranslator::makeLogicOperation(TokenKind op) {
    switch(op) {
    case tAAND: currentBC()->addInsn(BC_IAAND); break;
    case tAOR: currentBC()->addInsn(BC_IAOR); break;
    case tAXOR: currentBC()->addInsn(BC_IAXOR); break;
    default: throw TError::unsupportedOperation();
    }
}

//------------------------------------------------------------

void BCTranslator::visitBinaryOpNode(BinaryOpNode *node) {
    node->right()->visit(this); // upper (left) op lower (right)
    VarType rtype = tosType;
    node->left()->visit(this);
    VarType ltype = tosType;

    switch(node->kind()) {
    case tOR:
        convertTOSToBool();
        currentBC()->addInsn(BC_SWAP);
        convertTOSToBool();
        currentBC()->addInsn(BC_SWAP);
        currentBC()->addInsn(BC_IADD);
        convertTOSToBool();
        break;
    case tAND:
        convertTOSToBool();
        currentBC()->addInsn(BC_SWAP);
        convertTOSToBool();
        currentBC()->addInsn(BC_SWAP);
        currentBC()->addInsn(BC_IMUL);
        break;
    case tEQ: case tNEQ: case tGE: case tGT: case tLE: case tLT:
        makeNumTypeCast(ltype, rtype, VT_INT, false);
        makeComparisonOperation(node->kind());
        break;
    case tADD: case tSUB: case tMUL: case tDIV: case tMOD:
        makeNumTypeCast(ltype, rtype, VT_DOUBLE, true);
        makeMathOperation(node->kind());
        break;
    case tAOR: case tAXOR: case tAAND:
        makeNumTypeCast(ltype, rtype, VT_INT, false);
        makeLogicOperation(node->kind());
        break;
    default: throw TError::unsupportedOperation(node->position());
    }
}

void BCTranslator::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
    switch(node->kind()) {
    case tNOT:
//        if(tosType != VT_INT) throw TError::logicTypeError(node->position());
        convertTOSToBool();
        currentBC()->addInsn(BC_ILOAD1);
        currentBC()->addInsn(BC_ISUB);
        break;
    case tSUB:
        if(!isNumType(tosType)) throw TError::mathOpWithNANs(node->position());
        currentBC()->addInsn(MAKE_INSN_NUM(BC_, tosType, NEG));
        break;
    default:
        throw std::string("Unsupported unary operation: ") + tokenOp(node->kind());
    }
}

//------------------------------------------------------------

void BCTranslator::visitIfNode(IfNode *node) {
    Label ifElse(currentBC());
    Label ifEnd(currentBC());

    node->ifExpr()->visit(this);
    currentBC()->addInsn(BC_ILOAD0);
    currentBC()->addBranch(BC_IFICMPE, ifElse); //if (condition == 0) goto ifElse label
    node->thenBlock()->visit(this);
    currentBC()->addBranch(BC_JA, ifEnd);       //goto ifEnd label after then block
    currentBC()->bind(ifElse);                  //start else block here
    if(node->elseBlock()) node->elseBlock()->visit(this);
    currentBC()->bind(ifEnd);
}

void BCTranslator::visitForNode(ForNode *node) {
    Label forStart(currentBC());
    Label forEnd(currentBC());

    //assume that for template is (int_var in (intExpr1)..(intExpr2)) and intExpr1 < intExpr2
    if(node->var()->type() != VT_INT) throw TError::forVarError(node->position());
    if(!node->inExpr()->isBinaryOpNode()) throw TError::forNotRangeError(node->position());
    BinaryOpNode *inExpr = node->inExpr()->asBinaryOpNode();
    if(inExpr->kind() != tRANGE) throw TError::forNotRangeError(node->position());

    inExpr->left()->visit(this);
    if(tosType != VT_INT) throw TError::forRangeTypeError(node->position());
    storeVar(node->var());

    currentBC()->bind(forStart);
    inExpr->right()->visit(this);
    if(tosType != VT_INT) throw TError::forRangeTypeError(node->position());
    loadVar(node->var());
    currentBC()->addBranch(BC_IFICMPG, forEnd);

    node->body()->visit(this);

    loadVar(node->var());
    currentBC()->addInsn(BC_ILOAD1);
    currentBC()->addInsn(BC_IADD);
    storeVar(node->var());
    currentBC()->addBranch(BC_JA, forStart);
    currentBC()->bind(forEnd);
}

void BCTranslator::visitWhileNode(WhileNode *node) {
    Label whileStart(currentBC());
    Label whileEnd(currentBC());

    currentBC()->bind(whileStart);
    node->whileExpr()->visit(this);     //check types?
    currentBC()->addInsn(BC_ILOAD0);
    currentBC()->addBranch(BC_IFICMPE, whileEnd);
    node->loopBlock()->visit(this);
    currentBC()->addBranch(BC_JA, whileStart);
    currentBC()->bind(whileEnd);
}

//------------------------------------------------------------

void BCTranslator::processVar(const AstVar *var, bool loading) {
    std::pair<uint16_t, TScope*> scopeVar = currentScope->getVar(var);

    if(SCOPEVAR_CTX(scopeVar) == currentScope->function->id()) {
        switch(SCOPEVAR_ID(scopeVar)) {
        case 0: currentBC()->addInsn(MAKE_INSN_SEL(loading, BC_LOAD, BC_STORE, var->type(), VAR0)); break;
        case 1: currentBC()->addInsn(MAKE_INSN_SEL(loading, BC_LOAD, BC_STORE, var->type(), VAR1)); break;
        case 2: currentBC()->addInsn(MAKE_INSN_SEL(loading, BC_LOAD, BC_STORE, var->type(), VAR2)); break;
        case 3: currentBC()->addInsn(MAKE_INSN_SEL(loading, BC_LOAD, BC_STORE, var->type(), VAR3)); break;
        default:
            currentBC()->addInsn(MAKE_INSN_SEL(loading, BC_LOAD, BC_STORE, var->type(), VAR));
            currentBC()->addUInt16(SCOPEVAR_ID(scopeVar));
        }
    } else {
        currentBC()->addInsn(MAKE_INSN_SEL(loading, BC_LOADCTX, BC_STORECTX, var->type(), VAR));
        currentBC()->addUInt16(SCOPEVAR_CTX(scopeVar));
        currentBC()->addUInt16(SCOPEVAR_ID(scopeVar));
    }
}

//------------------------------------------------------------

void BCTranslator::visitIntLiteralNode(IntLiteralNode *node) {
    currentBC()->addInsn(BC_ILOAD);
    currentBC()->addInt64(node->literal());
    tosType = VT_INT;
}

void BCTranslator::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    currentBC()->addInsn(BC_DLOAD);
    currentBC()->addDouble(node->literal());
    tosType = VT_DOUBLE;
}

void BCTranslator::visitStringLiteralNode(StringLiteralNode *node) {
    currentBC()->addInsn(BC_SLOAD);
    currentBC()->addUInt16(code->makeStringConstant(node->literal()));
    tosType = VT_STRING;
}

void BCTranslator::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);

    switch(node->op()) {
    case tASSIGN: break;
    case tINCRSET:
        loadVar(node->var());
        currentBC()->addInsn(MAKE_INSN_NUM(BC_, node->var()->type(), ADD));
        break;
    case tDECRSET:
        loadVar(node->var());
        currentBC()->addInsn(MAKE_INSN_NUM(BC_, node->var()->type(), SUB));
        break;
    default:
        throw TError(std::string("Unsupported operation in StoreNode: ") + tokenOp(node->op()), node->position());
    }

    storeVar(node->var());
}

void BCTranslator::visitLoadNode(LoadNode *node) {
    loadVar(node->var());
}

void BCTranslator::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        currentBC()->addInsn(MAKE_INSN(BC_, tosType, PRINT));
    }
}
