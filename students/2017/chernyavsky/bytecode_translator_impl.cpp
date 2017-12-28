#include <iomanip>
#include <parser.h>
#include <sstream>
#include <dlfcn.h>
#include "bytecode_translator.h"
#include "interpreter_code.h"

namespace mathvm {

    BytecodeTranslateVisitor::BytecodeTranslateVisitor(Code* code)
        : _code(code)
        , _currentContext(nullptr) {
    }

    BytecodeTranslateVisitor::~BytecodeTranslateVisitor() {
        while (_currentContext) {
            clearContext();
        }
    }

    void BytecodeTranslateVisitor::translate(AstFunction* top) {
        auto* bcTop = new BytecodeFunction(top);
        _code->addFunction(bcTop);
        translateFunction(top);
        bcTop->bytecode()->addInsn(BC_STOP);
    }

    void BytecodeTranslateVisitor::translateFunction(AstFunction* func) {
        auto* bcFunc = (BytecodeFunction*) _code->functionByName(func->name());
        prepareContext(bcFunc, func->scope());

        for (uint i = 0; i < func->parametersNumber(); ++i) {
            AstVar* var = func->scope()->lookupVariable(func->parameterName(i), false);
            translateStore(var);
        }

        func->node()->visit(this);
        bcFunc->setScopeId(static_cast<id_t>(getContext()->getId()));  // what can i do?
        bcFunc->setLocalsNumber(getContext()->getLocalsNumber());

        clearContext();
    }

    void BytecodeTranslateVisitor::translateLoad(const AstVar* var) {
        Context* context = findContextByVarName(var->name());

        Instruction insn;
        switch (var->type()) {
            case VT_DOUBLE:
                insn = context == getContext() ? BC_LOADDVAR : BC_LOADCTXDVAR;
                break;
            case VT_INT:
                insn = context == getContext() ? BC_LOADIVAR : BC_LOADCTXIVAR;
                break;
            case VT_STRING:
                insn = context == getContext() ? BC_LOADSVAR : BC_LOADCTXSVAR;
                break;
            default:
                throw std::runtime_error("Unknown type");
        }

        getBytecode()->addInsn(insn);
        getBytecode()->addUInt16(context->getVarId(var->name()));
        if (context != getContext()) {
            getBytecode()->addUInt16(context->getId());
        }

        setTosType(var->type());
    }

    void BytecodeTranslateVisitor::translateStore(const AstVar* var) {
        Context* context = findContextByVarName(var->name());

        Instruction insn;
        switch (var->type()) {
            case VT_DOUBLE:
                insn = context == getContext() ? BC_STOREDVAR : BC_STORECTXDVAR;
                break;
            case VT_INT:
                insn = context == getContext() ? BC_STOREIVAR : BC_STORECTXIVAR;
                break;
            case VT_STRING:
                insn = context == getContext() ? BC_STORESVAR : BC_STORECTXSVAR;
                break;
            default:
                throw std::runtime_error("Unknown type");
        }

        getBytecode()->addInsn(insn);
        getBytecode()->addUInt16(context->getVarId(var->name()));
        if (context != getContext()) {
            getBytecode()->addUInt16(context->getId());
        }

        setTosType(var->type());
    }

    Context* BytecodeTranslateVisitor::getContext() {
        return _currentContext;
    }

    Context* BytecodeTranslateVisitor::findContextByVarName(const string& name) {
        for (Context* context = _currentContext; context; context = context->getParent()) {
            if (context->findVar(name)) {
                return context;
            }
        }

        return nullptr;
    }

    void BytecodeTranslateVisitor::prepareContext(BytecodeFunction* function, Scope* scope) {
        _currentContext = new Context(function, _currentContext, scope);
    }

    void BytecodeTranslateVisitor::clearContext() {
        Context* parentContext = _currentContext->getParent();
        delete _currentContext;
        _currentContext = parentContext;
    }

    Bytecode* BytecodeTranslateVisitor::getBytecode() {
        return getContext()->getBytecode();
    }

    VarType BytecodeTranslateVisitor::getTosType() {
        return getContext()->getTosType();
    }

    void BytecodeTranslateVisitor::setTosType(VarType type) {
        getContext()->setTosType(type);
    }

    void BytecodeTranslateVisitor::castTos(VarType type) {
        if (getTosType() == type) {
            return;
        }

        if (getTosType() == VT_DOUBLE && type == VT_INT) {
            getBytecode()->addInsn(BC_D2I);
        } else if (getTosType() == VT_INT && type == VT_DOUBLE) {
            getBytecode()->addInsn(BC_I2D);
        } else if (getTosType() == VT_STRING && type == VT_DOUBLE) {
            getBytecode()->addInsn(BC_S2I);
            getBytecode()->addInsn(BC_I2D);
        } else if (getTosType() == VT_STRING && type == VT_INT) {
            getBytecode()->addInsn(BC_S2I);
        } else {
            throw std::runtime_error("Cast error");
        }

        setTosType(type);
    }

    void BytecodeTranslateVisitor::visitBinaryOpNode(BinaryOpNode* node) {
        switch (node->kind()) {
            case tOR:
            case tAND:
                visitLogicalOp(node);
                break;
            case tAOR:
            case tAAND:
            case tAXOR:
                visitBitwiseOp(node);
                break;
            case tEQ:
            case tNEQ:
            case tGT:
            case tGE:
            case tLT:
            case tLE:
                visitComparingOp(node);
                break;
            case tADD:
            case tSUB:
            case tMUL:
            case tDIV:
            case tMOD:
                visitArithmeticOp(node);
                break;
            default: {
                throw std::runtime_error("Unknown binary operation");
            }
        }
    }

    void BytecodeTranslateVisitor::visitLogicalOp(BinaryOpNode* node) {
        node->left()->visit(this);
        if (getTosType() != VT_INT) {
            throw std::runtime_error("Invalid logical op argument");
        }

        getBytecode()->addInsn(BC_ILOAD0);

        Instruction insn;
        switch (node->kind()) {
            case tAND:
                insn = BC_IFICMPE;
                break;
            case tOR:
                insn = BC_IFICMPNE;
                break;
            default:
                throw std::runtime_error("Unknown logical op");
        }

        Label restoreStart(getBytecode());
        Label restoreEnd(getBytecode());

        getBytecode()->addBranch(insn, restoreStart);

        node->right()->visit(this);
        castTos(VT_INT);
        getBytecode()->addBranch(BC_JA, restoreEnd);

        getBytecode()->bind(restoreStart);
        getBytecode()->addInsn(node->kind() == tAND ? BC_ILOAD0 : BC_ILOAD1);
        getBytecode()->bind(restoreEnd);

        setTosType(VT_INT);
    }

    void BytecodeTranslateVisitor::visitBitwiseOp(BinaryOpNode* node) {
        node->right()->visit(this);
        castTos(VT_INT);
        node->left()->visit(this);
        castTos(VT_INT);

        Instruction insn;
        switch (node->kind()) {
            case tAOR:
                insn = BC_IAOR;
                break;
            case tAAND:
                insn = BC_IAAND;
                break;
            case tAXOR:
                insn = BC_IAXOR;
                break;
            default:
                throw std::runtime_error("Unknown bitwise operation op");
        }

        getBytecode()->addInsn(insn);
        setTosType(VT_INT);
    }

    void BytecodeTranslateVisitor::visitComparingOp(BinaryOpNode* node) {
        node->right()->visit(this);
        castTos(VT_INT);
        node->left()->visit(this);
        castTos(VT_INT);

        Instruction insn;
        switch (node->kind()) {
            case tEQ:
                insn = BC_IFICMPNE;
                break;
            case tNEQ:
                insn = BC_IFICMPE;
                break;
            case tGT:
                insn = BC_IFICMPLE;
                break;
            case tGE:
                insn = BC_IFICMPL;
                break;
            case tLT:
                insn = BC_IFICMPGE;
                break;
            case tLE:
                insn = BC_IFICMPG;
                break;
            default:
                throw std::runtime_error("Unknown comparing operation op");
        }

        Label falseStart(getBytecode());
        Label falseEnd(getBytecode());

        getBytecode()->addBranch(insn, falseStart);

        getBytecode()->addInsn(BC_ILOAD1);
        getBytecode()->addBranch(BC_JA, falseEnd);

        getBytecode()->bind(falseStart);
        getBytecode()->addInsn(BC_ILOAD0);
        getBytecode()->bind(falseEnd);

        setTosType(VT_INT);
    }

    void BytecodeTranslateVisitor::visitArithmeticOp(BinaryOpNode* node) {
        node->right()->visit(this);
        VarType rtype = getTosType();
//        getBytecode()->addInsn(BC_DUMP);
        node->left()->visit(this);
        VarType ltype = getTosType();
//        getBytecode()->addInsn(BC_DUMP);

        if ((ltype != VT_DOUBLE && ltype != VT_INT) ||
            (rtype != VT_DOUBLE && rtype != VT_INT)) {
            throw std::runtime_error("Invalid arithmetic op types");
        }

        if (ltype != rtype) {
            if (ltype == VT_DOUBLE) {
                getBytecode()->addInsn(BC_SWAP);
                setTosType(VT_INT);
            }
            castTos(VT_DOUBLE);
            if (ltype == VT_DOUBLE) {
                getBytecode()->addInsn(BC_SWAP);
            }
        }

        Instruction insn;
        switch (node->kind()) {
            case tADD:
                insn = getTosType() == VT_DOUBLE ? BC_DADD : BC_IADD;
                break;
            case tSUB:
                insn = getTosType() == VT_DOUBLE ? BC_DSUB : BC_ISUB;
                break;
            case tMUL:
                insn = getTosType() == VT_DOUBLE ? BC_DMUL : BC_IMUL;
                break;
            case tDIV:
                insn = getTosType() == VT_DOUBLE ? BC_DDIV : BC_IDIV;
                break;
            case tMOD:
                if (getTosType() == VT_INT) {
                    insn = BC_IMOD;
                    break;
                }
            default:
                throw std::runtime_error("Unknown arithmetic op");
        }

        getBytecode()->addInsn(insn);
        setTosType(getTosType());
    }

    void BytecodeTranslateVisitor::visitUnaryOpNode(UnaryOpNode* node) {
        switch (node->kind()) {
            case tNOT:
                visitNotOp(node);
                break;
            case tSUB:
                visitNegOp(node);
                break;
            default:
                throw std::runtime_error("Unknown unary operation");
        }
    }

    void BytecodeTranslateVisitor::visitNotOp(UnaryOpNode* node) {
        node->operand()->visit(this);
        if (getTosType() != VT_INT) {
            throw std::runtime_error("Invalid not argument");
        }

        Label elseStart(getBytecode());
        Label elseEnd(getBytecode());

        getBytecode()->addInsn(BC_ILOAD0);
        getBytecode()->addBranch(BC_IFICMPE, elseStart);

        getBytecode()->addInsn(BC_ILOAD0);
        getBytecode()->addBranch(BC_JA, elseEnd);

        getBytecode()->bind(elseStart);
        getBytecode()->addInsn(BC_ILOAD1);
        getBytecode()->bind(elseEnd);
    }

    void BytecodeTranslateVisitor::visitNegOp(UnaryOpNode* node) {
        node->operand()->visit(this);
        switch (getTosType()) {
            case VT_DOUBLE:
                getBytecode()->addInsn(BC_DNEG);
                break;
            case VT_INT:
                getBytecode()->addInsn(BC_INEG);
                break;
            default:
                throw std::runtime_error("Invalid neg argument");
        }
    }

    void BytecodeTranslateVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
        getBytecode()->addInsn(BC_DLOAD);
        getBytecode()->addDouble(node->literal());
        setTosType(VT_DOUBLE);
    }

    void BytecodeTranslateVisitor::visitIntLiteralNode(IntLiteralNode* node) {
        getBytecode()->addInsn(BC_ILOAD);
        getBytecode()->addInt64(node->literal());
        setTosType(VT_INT);
    }

    void BytecodeTranslateVisitor::visitStringLiteralNode(StringLiteralNode* node) {
        getBytecode()->addInsn(BC_SLOAD);
        getBytecode()->addUInt16(_code->makeStringConstant(node->literal()));
        setTosType(VT_STRING);
    }

    void BytecodeTranslateVisitor::visitLoadNode(LoadNode* node) {
        translateLoad(node->var());
    }

    void BytecodeTranslateVisitor::visitStoreNode(StoreNode* node) {
        node->value()->visit(this);
        VarType type = node->var()->type();
        castTos(type);

        switch (node->op()) {
            case tASSIGN:
                break;
            case tINCRSET:
            case tDECRSET: {
                translateLoad(node->var());
                Instruction insn = BC_INVALID;
                if (type == VT_DOUBLE && node->op() == tINCRSET) {
                    insn = BC_DADD;
                } else if (type == VT_DOUBLE && node->op() == tDECRSET) {
                    insn = BC_DSUB;
                } else if (type == VT_INT && node->op() == tINCRSET) {
                    insn = BC_IADD;
                } else if (type == VT_INT && node->op() == tDECRSET) {
                    insn = BC_ISUB;
                }
                getBytecode()->addInsn(insn);
                break;
            }
            default:
                throw std::runtime_error("Unknown store operation");
        }

        translateStore(node->var());
    }

    void BytecodeTranslateVisitor::visitForNode(ForNode* node) {
        if (node->var()->type() != VT_INT) {
            throw std::runtime_error("Invalid for-expression");
        }

        BinaryOpNode* inOp = node->inExpr()->asBinaryOpNode();

        inOp->left()->visit(this);
        if (getTosType() != VT_INT) {
            throw std::runtime_error("Invalid for-expression");
        }
        translateStore(node->var());

        Label forStart(getBytecode());
        Label forEnd(getBytecode());

        getBytecode()->bind(forStart);
        translateLoad(node->var());
        inOp->right()->visit(this);
        getBytecode()->addBranch(BC_IFICMPL, forEnd);

        node->body()->visit(this);

        translateLoad(node->var());
        getBytecode()->addInsn(BC_ILOAD1);
        getBytecode()->addInsn(BC_IADD);
        translateStore(node->var());

        getBytecode()->addBranch(BC_JA, forStart);
        getBytecode()->bind(forEnd);
    }

    void BytecodeTranslateVisitor::visitWhileNode(WhileNode* node) {
        Label whileStart(getBytecode());
        Label whileEnd(getBytecode());

        getBytecode()->bind(whileStart);
        node->whileExpr()->visit(this);
        if (getTosType() != VT_INT) {
            throw std::runtime_error("Invalid while-expression");
        }

        getBytecode()->addInsn(BC_ILOAD0);
        getBytecode()->addBranch(BC_IFICMPE, whileEnd);

        node->loopBlock()->visit(this);

        getBytecode()->addBranch(BC_JA, whileStart);
        getBytecode()->bind(whileEnd);
    }

    void BytecodeTranslateVisitor::visitIfNode(IfNode* node) {
        node->ifExpr()->visit(this);
        if (getTosType() != VT_INT) {
            throw std::runtime_error("Invalid if-expression");
        }

        Label elseStart(getBytecode());
        Label elseEnd(getBytecode());

        getBytecode()->addInsn(BC_ILOAD0);
        getBytecode()->addBranch(BC_IFICMPE, elseStart);

        node->thenBlock()->visit(this);

        getBytecode()->addBranch(BC_JA, elseEnd);
        getBytecode()->bind(elseStart);
        if (node->elseBlock()) {
            node->elseBlock()->visit(this);
        }
        getBytecode()->bind(elseEnd);
    }

    void BytecodeTranslateVisitor::visitBlockNode(BlockNode* node) {
        Scope::VarIterator vars(node->scope());
        while (vars.hasNext()) {
            getContext()->addVar(vars.next()->name());
        }

        Scope::FunctionIterator funcs(node->scope());
        while (funcs.hasNext()) {
            AstFunction* func = funcs.next();
            if (_code->functionByName(func->name())) {
                throw std::runtime_error("Duplicate function");
            }
            auto* bcFunc = new BytecodeFunction(func);
            id_t funcId = _code->addFunction(bcFunc);
            bcFunc->setScopeId(funcId);
            func->setInfo(bcFunc);
        }

        funcs = Scope::FunctionIterator(node->scope());
        while (funcs.hasNext()) {
            translateFunction(funcs.next());
        }

        node->visitChildren(this);
        setTosType(VT_VOID);
    }

    void BytecodeTranslateVisitor::visitFunctionNode(FunctionNode* node) {
        node->body()->visit(this);
    }

    void BytecodeTranslateVisitor::visitReturnNode(ReturnNode* node) {
        VarType returnType = getContext()->getFunction()->returnType();

        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
            castTos(returnType);
        } else {
            setTosType(returnType);
        }

        getBytecode()->addInsn(BC_RETURN);
    }

    void BytecodeTranslateVisitor::visitCallNode(CallNode* node) {
        auto* func = (BytecodeFunction*) _code->functionByName(node->name());

        if (func == nullptr) {
            throw std::runtime_error("Unknown function");
        }

        if (func->parametersNumber() != node->parametersNumber()) {
            throw std::runtime_error("Invalid function parameters");
        }

        for (uint32_t i = node->parametersNumber(); i > 0; --i) {
            node->parameterAt(i - 1)->visit(this);
            castTos(func->parameterType(i - 1));
        }

        getBytecode()->addInsn(BC_CALL);
        getBytecode()->addUInt16(func->id());
        setTosType(func->returnType());
    }

    void BytecodeTranslateVisitor::visitNativeCallNode(NativeCallNode* node) {
        const string& funcName = node->nativeName();
        void* funcAddress = dlsym(RTLD_DEFAULT, funcName.c_str());
        if (funcAddress == nullptr) {
            throw std::runtime_error("Unknown native function");
        }

        uint16_t funcId = _code->makeNativeFunction(funcName, node->nativeSignature(), funcAddress);

        getBytecode()->addInsn(BC_CALLNATIVE);
        getBytecode()->addUInt16(funcId);
    }

    void BytecodeTranslateVisitor::visitPrintNode(PrintNode* node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            switch (getTosType()) {
                case VT_DOUBLE:
                    getBytecode()->addInsn(BC_DPRINT);
                    break;
                case VT_INT:
                    getBytecode()->addInsn(BC_IPRINT);
                    break;
                case VT_STRING:
                    getBytecode()->addInsn(BC_SPRINT);
                    break;
                default:
                    throw std::runtime_error("Invalid TOS type");
            }
            setTosType(VT_VOID);
        }
    }

    Status* BytecodeTranslatorImpl::translate(const string& program, Code** result) {
        Parser parser;

        Status* status = parser.parseProgram(program);
        if (status->isOk()) {
            try {
                *result = new InterpreterCodeImpl;
                BytecodeTranslateVisitor translator(*result);
                translator.translate(parser.top());
            } catch (const std::exception& e) {
                return Status::Error(e.what());
            }
        }

        return status;
    }

}
