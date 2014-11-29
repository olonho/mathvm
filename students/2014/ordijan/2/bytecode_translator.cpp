#include "bytecode_translator.h"
#include "bytecode_interpreter.h"

#include <dlfcn.h>              /* native call */

#include <limits>
#include <stdexcept>
#include <string>
#include <algorithm>

class TranslationError: public std::runtime_error {

    uint32_t _position;

public:
    TranslationError(const char* msg, uint32_t pos):
        std::runtime_error(msg), _position(pos) {
    }

    uint32_t position() const {
        return _position;
    }
};

bool doubleEquals(double a, double b) {
    /*
     * Bug note: don't remove std:: in std::abs
     *           (otherwise abs(int) is called)
     */
    return std::abs(a - b) < std::numeric_limits<double>::epsilon();
}

namespace mathvm {

bool isNative(AstFunction* f) {
    return f->node()->body()->nodes() > 0
        && f->node()->body()->nodeAt(0)->isNativeCallNode();
}

NativeCallNode* getNativeCallNode(AstFunction* f) {
    return f->node()->body()->nodeAt(0)->asNativeCallNode();
}

#define ERROR(msg) throw TranslationError(std::string(msg).c_str(), location())
#define EMIT(insn) bytecode()->addInsn((Instruction)(insn))
#define EMIT_ID(id) bytecode()->addUInt16(id)
#define EMIT_LOAD(x) loadStore(x)
#define EMIT_STORE(x) loadStore(x, false)
#define EMIT_INT(i) bytecode()->addInt64(i)
#define EMIT_DOUBLE(d) bytecode()->addDouble(d)
#define EMIT_BRANCH(type, label) bytecode()->addBranch(type, label)
#define EMIT_BIND(label) bytecode()->bind(label)
#define VISIT(node) node->visit(this)
#define TYPED(insn) (topType() == VT_INT ? BC_I##insn : BC_D##insn)
#define CONTEXT(f, l, sc, st) ContextSwitcher _CS(this, Context({f, l, sc, st}))

void BytecodeTranslatorVisitor::startWith(AstFunction* f) {
    declareFunction(f);
    visitAstFunction(f);
}

void BytecodeTranslatorVisitor::visitBinaryOpNode(BinaryOpNode * node) {
    onVisitNode(node);

    TokenKind op = node->kind();
    if (op == tOR || op == tAND) {
        processLazyLogic(node);
        return;
    }

    VISIT(node->right()); ensureTopIsNumeric();
    VISIT(node->left());  ensureTopIsNumeric();

    if (tEQ <= op && op <= tLE)
        processComparison(node->kind());
    else if (tADD <= op && op <= tDIV)
        processNumericOperation(node->kind());
    else if (op == tMOD || op == tAXOR || op == tAOR || op == tAAND) {
        popType(VT_INT);
        ensureTopType(VT_INT);
        EMIT(op == tAXOR ? BC_IAXOR :
             op == tMOD  ? BC_IMOD  :
             op == tAOR  ? BC_IAOR  :
                           BC_IAAND);

    } else ERROR("Unknown binary op");
}

void BytecodeTranslatorVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    onVisitNode(node);

    VISIT(node->operand());

    switch (node->kind()) {
    case tNOT: {
        ensureTopType(VT_INT); /* if (!3.14) should fail */
        Label L_True(bytecode());
        Label L_End(bytecode());
        EMIT(BC_ILOAD0);
        EMIT_BRANCH(BC_IFICMPE, L_True);
        EMIT(BC_ILOAD0);
        EMIT_BRANCH(BC_JA, L_End);
        EMIT_BIND(L_True);
        EMIT(BC_ILOAD1);
        EMIT_BIND(L_End);
        break;
    }
    case tSUB:
        ensureTopIsNumeric();
        EMIT(TYPED(NEG));
        break;
    default:
        ERROR("Unknown unary op");
    }
}

void BytecodeTranslatorVisitor::visitStringLiteralNode(StringLiteralNode * node) {
    onVisitNode(node);

    if (node->literal().empty())
        EMIT(BC_SLOAD0);
    else {
        EMIT(BC_SLOAD);
        EMIT_ID(code()->makeStringConstant(node->literal()));
    }

    pushType(VT_STRING);
}

void BytecodeTranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    onVisitNode(node);

    const double value = node->literal();
    if (doubleEquals(value, -1.0))
        EMIT(BC_DLOADM1);
    else if (doubleEquals(value, 0.0))
        EMIT(BC_DLOAD0);
    else if (doubleEquals(value, 1.0))
        EMIT(BC_DLOAD1);
    else {
        EMIT(BC_DLOAD);
        EMIT_DOUBLE(value);
    }

    pushType(VT_DOUBLE);
}

void BytecodeTranslatorVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    onVisitNode(node);

    switch (node->literal()) {
    case -1: EMIT(BC_ILOADM1); break;
    case  0: EMIT(BC_ILOAD0);  break;
    case  1: EMIT(BC_ILOAD1);  break;
    default:
        EMIT(BC_ILOAD);
        EMIT_INT(node->literal());
        break;
    }

    pushType(VT_INT);
}

void BytecodeTranslatorVisitor::visitLoadNode(LoadNode* node) {
    onVisitNode(node);

    EMIT_LOAD(node->var());

    pushType(node->var()->type());
}

void BytecodeTranslatorVisitor::visitStoreNode(StoreNode* node) {
    onVisitNode(node);

    visitTyped(node->value(), node->var()->type());
    pushType(node->var()->type());

    if (node->op() == tINCRSET || node->op() == tDECRSET) {
        ensureTopIsNumeric();
        EMIT_LOAD(node->var());

        if (node->op() == tINCRSET)
            EMIT(TYPED(ADD));
        else
            EMIT(TYPED(SUB));

    } else if (node->op() != tASSIGN)
        ERROR("Invalid store operation");

    EMIT_STORE(node->var());
    popType();
    pushType(VT_VOID); /* a = b = c is forbidden */
}

void BytecodeTranslatorVisitor::visitForNode(ForNode* node) {
    onVisitNode(node);

    const AstVar* i = node->var();
    if (i->type() != VT_INT)
        ERROR("Non-iterable type in for loop");

    const BinaryOpNode* expr = node->inExpr()->asBinaryOpNode();
    if (expr == NULL || expr->kind() != tRANGE)
        ERROR("Invalid range in for loop");

    CONTEXT(function(), locals(), node->body()->scope(), typeStack());

    beforeProcessBlock();

    bool needTempVar = !expr->right()->isIntLiteralNode();
    AstVar* temp = NULL;
    if (needTempVar) {
        if (!scope()->declareVariable("<tempForEnd>", VT_INT))
            ERROR("internal error: temp name is unavailable");
        temp = scope()->lookupVariable("<tempForEnd>", false);
    }

    Label L_Begin(bytecode());
    Label L_End(bytecode());

    VISIT(expr->left());
    EMIT_STORE(i);

    if (needTempVar) {
        VISIT(expr->right());
        EMIT_STORE(temp);
        popType(VT_INT);
    }

    popType(VT_INT);
    EMIT_BIND(L_Begin);
    if (needTempVar)
        EMIT_LOAD(temp);
    else
        VISIT(expr->right());
    EMIT_LOAD(i);
    EMIT_BRANCH(BC_IFICMPG, L_End);

    processBlockNode(node->body());
    afterProcessBlock();

    /* i += 1 */
    EMIT_LOAD(i);
    EMIT(BC_ILOAD1);
    EMIT(BC_IADD);
    EMIT_STORE(i);

    EMIT_BRANCH(BC_JA, L_Begin);
    EMIT_BIND(L_End);

    pushType(VT_VOID);
}

void BytecodeTranslatorVisitor::visitWhileNode(WhileNode* node) {
    onVisitNode(node);

    Label L_Begin(bytecode());
    Label L_End(bytecode());

    EMIT_BIND(L_Begin);
    falseJump(node->whileExpr(), L_End);
    VISIT(node->loopBlock());
    EMIT_BRANCH(BC_JA, L_Begin);
    EMIT_BIND(L_End);
}

void BytecodeTranslatorVisitor::visitIfNode(IfNode* node) {
    onVisitNode(node);

    Label L_End(bytecode());

    if (!node->elseBlock()) {
        falseJump(node->ifExpr(), L_End);
        VISIT(node->thenBlock());
        EMIT_BIND(L_End);
    } else {
        Label L_Else(bytecode());
        falseJump(node->ifExpr(), L_Else);
        visitTyped(node->thenBlock(), VT_VOID);
        EMIT_BRANCH(BC_JA, L_End);
        EMIT_BIND(L_Else);
        VISIT(node->elseBlock());
        EMIT_BIND(L_End);
    }
}

void BytecodeTranslatorVisitor::visitBlockNode(BlockNode* node) {
    onVisitNode(node);

    CONTEXT(function(), locals(), node->scope(), typeStack());

    beforeProcessBlock();
    processBlockNode(node);
    afterProcessBlock();

    pushType(VT_VOID);
}

void BytecodeTranslatorVisitor::visitFunctionNode(FunctionNode * node) {
    onVisitNode(node);

    ERROR("Unexpected function node");
}

void BytecodeTranslatorVisitor::visitReturnNode(ReturnNode* node) {
    onVisitNode(node);

    if (node->returnExpr() != NULL)
        visitTyped(node->returnExpr(), function()->returnType());
    else if (function()->returnType() != VT_VOID)
        ERROR("Missing return statement");

    EMIT(BC_RETURN);

    ensureTypeStackEmpty();
    pushType(function()->returnType());
}

void BytecodeTranslatorVisitor::visitCallNode(CallNode* node) {
    onVisitNode(node);

    AstFunction* f = scope()->lookupFunction(node->name());
    if (!f) ERROR("Unknown function " + f->name());
    checkSignature(node, f);

    for (uint16_t i = 0; i < node->parametersNumber(); i++)
        visitTyped(node->parameterAt(i), f->parameterType(i));

    if (isNative(f))
        EMIT(BC_CALLNATIVE);
    else
        EMIT(BC_CALL);

    EMIT_ID(getFunctionId(f));

    pushType(f->returnType());
}

void BytecodeTranslatorVisitor::visitNativeCallNode(NativeCallNode* node) {
    onVisitNode(node);

    ERROR("Unexpected native");
}

void BytecodeTranslatorVisitor::visitPrintNode(PrintNode * node) {
    onVisitNode(node);

    for (uint32_t i = 0; i < node->operands(); ++i) {
        VISIT(node->operandAt(i));

        switch (topType()) {
        case VT_INT:    EMIT(BC_IPRINT); break;
        case VT_DOUBLE: EMIT(BC_DPRINT); break;
        case VT_STRING: EMIT(BC_SPRINT); break;
        default: ERROR("Unprintable parameter");
        }

        popType();
    }

    pushType(VT_VOID);
}

void BytecodeTranslatorVisitor::visitAstFunction(AstFunction* f) {
    if (isNative(f)) return;

    onVisitNode(f->node());

    std::stack<VarType> typeStack;
    CONTEXT(f, 0, f->scope(), &typeStack);

    for (uint32_t i = f->parametersNumber(); i --> 0;) {
        ensureIsParameterType(f->parameterType(i));
        AstVar* x = scope()->lookupVariable(f->parameterName(i));
        assert(x);
        variableInScope(x);
        EMIT_STORE(x);
    }

    visitTyped(f->node()->body(), VT_VOID);

    for (uint32_t i = 0; i < f->parametersNumber(); i++)
        variableOutOfScope(scope()->lookupVariable(f->parameterName(i)));

    if (f->name() == AstFunction::top_name)
        EMIT(BC_STOP);
}

void BytecodeTranslatorVisitor::onVisitNode(AstNode* node) {
    _location = node->position();

#ifdef DEBUG
#define PRINT_VISIT(type, name)                                           \
    if (node->is##type()) { fprintf(stderr, "visit " #type "\n"); return; }

    FOR_NODES(PRINT_VISIT)
#undef PRINT_VISIT
#endif
}

void BytecodeTranslatorVisitor::visitTyped(AstNode* node, VarType type) {
    VISIT(node);
    tryCast(type);
    popType(type);
}

void BytecodeTranslatorVisitor::declareFunction(AstFunction* f) {
    if (isNative(f)) {
        processNativeCallNode(f);
        return;
    }
    BytecodeFunction* bf = new BytecodeFunction(f);
    code()->addFunction(bf);
    f->setInfo(bf);
}

uint16_t BytecodeTranslatorVisitor::getFunctionId(AstFunction* f) const {
    if (isNative(f)) {
        return code()->getNativeId(f->name());
    }
    return ((BytecodeFunction*) f->info())->id();
}

BytecodeTranslatorVisitor::VarDescriptor*
BytecodeTranslatorVisitor::getDescriptor(const AstVar* x) const {
    if (!x->info()) ERROR("Unknown variable " + x->name());
    return (VarDescriptor *)x->info();
}

void BytecodeTranslatorVisitor::variableInScope(AstVar* x) {
    assert(x->info() == NULL);
    if (locals() == (uint16_t) -1)
        ERROR("Too many vars");
    x->setInfo(new VarDescriptor({functionId(), locals()++, false}));
    bytecodeFunction()->setLocalsNumber(std::max(locals(), (uint16_t) bytecodeFunction()->localsNumber()));
}

void BytecodeTranslatorVisitor::variableOutOfScope(AstVar* x) {
    assert(x->info());
    delete (VarDescriptor*)x->info();
    locals()--;
}

VarType BytecodeTranslatorVisitor::topType() const {
    if (typeStack()->empty())
        ERROR("internal error: stack underflow");
    return typeStack()->top();
}

void BytecodeTranslatorVisitor::pushType(VarType type) {
    typeStack()->push(type);
    code()->onStackGrow(functionId(), typeStack()->size());
}

void BytecodeTranslatorVisitor::popType(VarType expected) {
    if (topType() != expected)
        ERROR(string("Expected: ") + typeToName(expected) +
              ", got: " + typeToName(topType()));
    popType();
}

VarType BytecodeTranslatorVisitor::popType() {
    VarType type = topType();
    typeStack()->pop();
    return type;
}

void BytecodeTranslatorVisitor::tryCast(VarType to) {
    static Instruction castTable[VAR_TYPES_NUMBER][VAR_TYPES_NUMBER] = {BC_INVALID};
    castTable[VT_INT][VT_DOUBLE] = BC_I2D;
    castTable[VT_DOUBLE][VT_INT] = BC_D2I;

    VarType from = topType();
    if (from == to) return;

    Instruction cast = castTable[from][to];
    if (!cast)
        ERROR(string("Cast error from ") + typeToName(from) +
              " to " + typeToName(to));

    warningIf(cast == BC_D2I, "Casting double to int. I warned you!");

    EMIT(cast);
    popType();
    pushType(to);
}

void BytecodeTranslatorVisitor::castTopsToCommonType() {
    VarType hi = popType();
    VarType lo = popType();

    if (hi != lo) {
        if (hi == VT_DOUBLE) {
            EMIT(BC_SWAP);
            EMIT(BC_I2D);
            EMIT(BC_SWAP);
        } else {
            EMIT(BC_I2D);
        }
        hi = VT_DOUBLE;
    }

    pushType(hi);
    pushType(hi);
}

void BytecodeTranslatorVisitor::warningIf(bool bad, const char* msg) const {
    if (bad)
        fprintf(stderr, "WARNING: %s\n", msg);
}

bool BytecodeTranslatorVisitor::isUnused(AstNode* node) const {
    return node->isIntLiteralNode()    || node->isStringLiteralNode()
        || node->isDoubleLiteralNode() || node->isLoadNode()
        || node->isBinaryOpNode()      || node->isUnaryOpNode();
}

void BytecodeTranslatorVisitor::ensureTypeStackEmpty() const {
    if (!typeStack()->empty())
        ERROR("internal error: stack leak");
}

void BytecodeTranslatorVisitor::ensureIsParameterType(VarType type) const {
    if (type == VT_INVALID || type == VT_VOID)
        ERROR("Invalid signature");
}

void BytecodeTranslatorVisitor::ensureTopType(VarType expected) const {
    if (topType() != expected)
        ERROR(string("Expected ") + typeToName(expected));
}

void BytecodeTranslatorVisitor::ensureTopIsNumeric() const {
    if (topType() != VT_INT && topType() != VT_DOUBLE)
        ERROR("Expected numeric type");
}

void BytecodeTranslatorVisitor::loadStore(const AstVar* x, bool load) {
    VarDescriptor* descriptor = getDescriptor(x);
    if (!load) descriptor->initialized = true;
    warningIf(load && !descriptor->initialized,
              ("Use of uninitialized var " + x->name()).c_str());

    if (descriptor->functionId == functionId())
        loadStoreLocal(descriptor->localIndex, x->type(), load);
    else
        loadStoreGlobal(descriptor, x->type(), load);
}

void BytecodeTranslatorVisitor::loadStoreLocal(uint16_t id, VarType type, bool load) {
    static Instruction tableLocal[2 * VAR_TYPES_NUMBER] =
        {BC_INVALID, BC_INVALID, BC_LOADDVAR,  BC_LOADIVAR,  BC_LOADSVAR,
         BC_INVALID, BC_INVALID, BC_STOREDVAR, BC_STOREIVAR, BC_STORESVAR};
    static Instruction tableRegister[2 * VAR_TYPES_NUMBER] =
        {BC_INVALID, BC_INVALID, BC_LOADDVAR0,  BC_LOADIVAR0,  BC_LOADSVAR0,
         BC_INVALID, BC_INVALID, BC_STOREDVAR0, BC_STOREIVAR0, BC_STORESVAR0};

    uint16_t offset = type + (load ? 0 : VAR_TYPES_NUMBER);

    assert(tableLocal[offset] != BC_INVALID);

    if (id < VAR_REGISTERS_NUMBER)
        EMIT(tableRegister[offset] + id);
    else {
        EMIT(tableLocal[offset]);
        EMIT_ID(id);
    }
}

void BytecodeTranslatorVisitor::loadStoreGlobal(const VarDescriptor* descriptor, VarType type, bool load) {
    static Instruction tableGlobal[2 * VAR_TYPES_NUMBER] =
        {BC_INVALID, BC_INVALID, BC_LOADCTXDVAR,  BC_LOADCTXIVAR,  BC_LOADCTXSVAR,
         BC_INVALID, BC_INVALID, BC_STORECTXDVAR, BC_STORECTXIVAR, BC_STORECTXSVAR};

    uint16_t offset = type + (load ? 0 : VAR_TYPES_NUMBER);

    assert(tableGlobal[offset] != BC_INVALID);

    EMIT(tableGlobal[offset]);
    EMIT_ID(descriptor->functionId);
    EMIT_ID(descriptor->localIndex);
}

void BytecodeTranslatorVisitor::checkSignature(CallNode* node, AstFunction* f) const {
    uint32_t expected = f->parametersNumber();
    uint32_t got = node->parametersNumber();
    if (expected != got)
        ERROR(node->name() + ": expected " +
              std::to_string(expected) + " arguments");
}

void BytecodeTranslatorVisitor::processLazyLogic(BinaryOpNode* node) {
    Label L_Lazy(bytecode());
    Label L_End(bytecode());

    bool isOr = node->kind() == tOR;

    VISIT(node->left());
    popType(VT_INT);

    EMIT(BC_ILOAD0);
    EMIT_BRANCH(isOr ? BC_IFICMPNE : BC_IFICMPE, L_Lazy);

    VISIT(node->right());
    ensureTopType(VT_INT);

    EMIT_BRANCH(BC_JA, L_End);
    EMIT_BIND(L_Lazy);
    EMIT(isOr ? BC_ILOAD1 : BC_ILOAD0);
    EMIT_BIND(L_End);
}

void BytecodeTranslatorVisitor::processComparison(TokenKind kind) {
    castTopsToCommonType();

    Instruction cmp = TYPED(CMP);
    int hack = topType() == VT_INT ? 3 : 0;
    popType();
    popType();
    pushType(VT_INT);

    if (kind == tNEQ) {
        EMIT(cmp);
        return;
    }

    if (!hack) {
        EMIT(cmp);
        EMIT(BC_ILOAD0);
    }

    Label L_Fail(bytecode());
    Label L_End(bytecode());

    switch (kind) {
    case tEQ: EMIT_BRANCH(BC_IFICMPE,  L_Fail); break;
    case tGT: EMIT_BRANCH(BC_IFICMPG,  L_Fail); break;
    case tGE: EMIT_BRANCH(BC_IFICMPGE, L_Fail); break;
    case tLT: EMIT_BRANCH(BC_IFICMPL,  L_Fail); break;
    default : EMIT_BRANCH(BC_IFICMPLE, L_Fail); break;
    }

    EMIT(BC_ILOAD1 - hack);
    EMIT_BRANCH(BC_JA, L_End);
    EMIT_BIND(L_Fail);
    EMIT(BC_ILOAD0 + hack);
    EMIT_BIND(L_End);
}

void BytecodeTranslatorVisitor::processNumericOperation(TokenKind op) {
    castTopsToCommonType();

    switch (op) {
    case tADD: EMIT(TYPED(ADD)); break;
    case tSUB: EMIT(TYPED(SUB)); break;
    case tMUL: EMIT(TYPED(MUL)); break;
    default  : EMIT(TYPED(DIV)); break;
    }

    popType();
    ensureTopIsNumeric();
}

void BytecodeTranslatorVisitor::beforeProcessBlock() {
    for (Scope::VarIterator iter(scope()); iter.hasNext();)
        variableInScope(iter.next());

    for (Scope::FunctionIterator iter(scope()); iter.hasNext();)
        declareFunction(iter.next());

    for (Scope::FunctionIterator iter(scope()); iter.hasNext();)
        visitAstFunction(iter.next());
}

void BytecodeTranslatorVisitor::processBlockNode(BlockNode* node) {
    for (uint32_t i = 0; i < node->nodes(); i++) {
        AstNode* ith = node->nodeAt(i);

        warningIf(isUnused(ith), "unused result of statement");

        VISIT(ith);
        VarType result = popType();

        if (result != VT_VOID && !ith->isReturnNode())
            EMIT(BC_POP);
    }
}

void BytecodeTranslatorVisitor::afterProcessBlock() {
    for (Scope::VarIterator iter(scope()); iter.hasNext();)
        variableOutOfScope(iter.next());
}

void BytecodeTranslatorVisitor::processNativeCallNode(AstFunction* f) {
    NativeCallNode* node = getNativeCallNode(f);
    void* address = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (address == NULL)
        ERROR("Cannot link native: " + node->nativeName());

    code()->makeNativeFunction(f->name(),
                               node->nativeSignature(),
                               address);
}

void BytecodeTranslatorVisitor::falseJump(AstNode* node, Label& label) {
    bool isComparison = false;
    if (node->isBinaryOpNode()) {
        TokenKind kind = node->asBinaryOpNode()->kind();
        if (tEQ <= kind && kind <= tLE)
            isComparison = true;
    }
    if (!isComparison) {
        visitTyped(node, VT_INT);
        EMIT(BC_ILOAD0);
        EMIT_BRANCH(BC_IFICMPE, label);
    } else {
        /* Premature optimization is the root of all evil */
        VISIT(node->asBinaryOpNode()->right()); ensureTopIsNumeric();
        VISIT(node->asBinaryOpNode()->left());  ensureTopIsNumeric();
        TokenKind kind = node->asBinaryOpNode()->kind();

        castTopsToCommonType();

        Instruction cmp = TYPED(CMP);
        bool hack = topType() == VT_INT;
        popType();
        popType();

        if (!hack) {
            EMIT(cmp);
            EMIT(BC_ILOAD0);
        }

        switch (kind) {
        case tEQ:  EMIT_BRANCH(BC_IFICMPNE, label); break;
        case tNEQ: EMIT_BRANCH(BC_IFICMPE,  label); break;
        case tGT:  EMIT_BRANCH(hack ? BC_IFICMPLE : BC_IFICMPGE, label); break;
        case tGE:  EMIT_BRANCH(hack ? BC_IFICMPL  : BC_IFICMPG,  label); break;
        case tLT:  EMIT_BRANCH(hack ? BC_IFICMPGE : BC_IFICMPLE, label); break;
        default:   EMIT_BRANCH(hack ? BC_IFICMPG  : BC_IFICMPL,  label); break;
        }
    }
}


#undef ERROR
#undef EMIT
#undef EMIT_ID
#undef EMIT_LOAD
#undef EMIT_STORE
#undef EMIT_INT
#undef EMIT_DOUBLE
#undef EMIT_BRANCH
#undef EMIT_BIND
#undef VISIT
#undef TYPED
#undef CONTEXT

/***********************************************************/

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code) {
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError())
        return status;

    *code = new InterpreterCodeImpl();
    BytecodeTranslatorVisitor visitor((InterpreterCodeImpl*) *code);
    try {
        visitor.startWith(parser.top());
    } catch (const TranslationError& e) {
        delete status;
        return Status::Error(e.what(), e.position());
    }

    return status;
}

Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "interpreter") {
        return new BytecodeTranslatorImpl();
    }
    assert(false && "Jit is not implemented");
    return 0;
}

}
