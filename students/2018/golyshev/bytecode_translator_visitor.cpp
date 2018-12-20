#include "../../../libs/asmjit-master/src/asmjit/asmjit.h"
#include <dlfcn.h>
#include <bytecode_translator_visitor.hpp>
#include <interpreter_code_impl.hpp>

using namespace mathvm;

using std::runtime_error;

BytecodeTranslatorVisitor::BytecodeTranslatorVisitor(Code* code) : code_(code) {}

void BytecodeTranslatorVisitor::translateProgram(AstFunction* function) {
    auto bytecodeFunction = createFunction(function);
    code_->addFunction(bytecodeFunction);

    translateFunction(function);

    bytecodeFunction->bytecode()->addInsn(Instruction::BC_STOP);
}

BytecodeFunction* BytecodeTranslatorVisitor::createFunction(AstFunction* function) {
    static uint16_t SCOPE_ID = 0;

    auto bytecodeFunction = new BytecodeFunction(function);
    bytecodeFunction->setScopeId(SCOPE_ID++);
    bytecodeFunction->setLocalsNumber(0);
    return bytecodeFunction;
}

void BytecodeTranslatorVisitor::translateFunction(AstFunction* function) {
    auto bytecodeFunction = code_->functionByName(function->name());
    enterContext(dynamic_cast<BytecodeFunction*>(bytecodeFunction));
    function->node()->visit(this);
    bytecodeFunction->setLocalsNumber(context().varsNumber());
    leaveContext();
}

void BytecodeTranslatorVisitor::enterContext(BytecodeFunction* function) {
    contexts_.emplace_back(function);

    for (uint32_t i = 0; i < function->parametersNumber(); i++) {
        context().declareVar(function->parameterName(i));
    }
}

void BytecodeTranslatorVisitor::visitForNode(ForNode* node) {
    auto range = dynamic_cast<BinaryOpNode const*>(node->inExpr());
    if (range == nullptr || range->kind() != tRANGE) throw runtime_error("For cannot be without range");

    range->left()->visit(this);
    storeVar(*node->var());

    Label exprLabel(currentBytecode());
    Label exitLabel(currentBytecode());
    currentBytecode()->bind(exprLabel);

    range->right()->visit(this);
    loadVar(*node->var());

    currentBytecode()->addBranch(BC_IFICMPG, exitLabel);

    node->body()->visit(this);

    loadVar(*node->var());
    currentBytecode()->addInsn(BC_ILOAD1);
    currentBytecode()->addInsn(BC_IADD);
    storeVar(*node->var());

    currentBytecode()->addBranch(BC_JA, exprLabel);
    currentBytecode()->bind(exitLabel);
}

void BytecodeTranslatorVisitor::visitPrintNode(PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);

        Instruction insn;
        switch (context().tosType()) {
            case VT_DOUBLE:
                insn = BC_DPRINT;
                break;
            case VT_INT:
                insn = BC_IPRINT;
                break;
            case VT_STRING:
                insn = BC_SPRINT;
                break;
            default:
                throw runtime_error("Invalid type '"s + typeToName(context().tosType()) + "' on TOS!");
        }

        currentBytecode()->addInsn(insn);
    }
}

void BytecodeTranslatorVisitor::visitLoadNode(LoadNode* node) {
    loadVar(*node->var());
}

void BytecodeTranslatorVisitor::visitIfNode(IfNode* node) {
    node->ifExpr()->visit(this);
    currentBytecode()->addInsn(BC_ILOAD0);

    if (context().tosType() != VT_INT) {
        throw runtime_error("Only int value can be treated as boolean");
    }

    Label goToElse(currentBytecode());
    currentBytecode()->addBranch(BC_IFICMPE, goToElse);
    node->thenBlock()->visit(this);

    if (!node->elseBlock()) {
        currentBytecode()->bind(goToElse);
    } else {
        Label skipElse(currentBytecode());
        currentBytecode()->addBranch(BC_JA, skipElse);
        currentBytecode()->bind(goToElse);

        node->elseBlock()->visit(this);

        currentBytecode()->bind(skipElse);
    }
}

void BytecodeTranslatorVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    switch (node->kind()) {
        case tADD:
        case tMUL:
        case tSUB:
        case tDIV:
        case tMOD:
            binaryMathOperation(node);
            break;

        case tOR:
        case tAND:
            binaryBooleanLogicOperation(node);
            break;

        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            binaryRelationOperation(node);
            break;
        case tAOR:
        case tAAND:
        case tAXOR:
            binaryMathLogicOperation(node);
            break;
        case tRANGE:
        default:
            throw runtime_error("Unknown operand "s + tokenOp(node->kind()));
    }
}

void BytecodeTranslatorVisitor::visitCallNode(CallNode* node) {
    auto functionToCall = code_->functionByName(node->name());

    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        auto reversedIndex = node->parametersNumber() - i - 1;

        node->parameterAt(reversedIndex)->visit(this);
        castFromTo(context().tosType(), functionToCall->parameterType(reversedIndex));
    }

    currentBytecode()->addInsn(BC_CALL);
    currentBytecode()->addUInt16(functionToCall->id());

    context().setTosType(functionToCall->returnType());
}

void BytecodeTranslatorVisitor::visitStoreNode(StoreNode* node) {
    auto pVar = *node->var();
    auto varType = node->var()->type();

    node->value()->visit(this);
    castFromTo(context().tosType(), varType);

    switch (node->op()) {
        case tASSIGN:
            break;

        case tINCRSET:
            loadVar(pVar);
            currentBytecode()->addInsn(varType == VT_INT ? BC_IADD : BC_DADD);
            break;

        case tDECRSET:
            loadVar(pVar);
            currentBytecode()->addInsn(varType == VT_INT ? BC_ISUB: BC_DSUB);
            break;

        default:
            throw runtime_error(
                "Unsupported type of store "s + tokenOp(node->op())
            );
    }

    context().setTosType(varType);
    storeVar(pVar);
}

// TODO seek for more optimal jumps
void BytecodeTranslatorVisitor::visitWhileNode(WhileNode* node) {
    Label exprLabel(currentBytecode());
    Label exitLabel(currentBytecode());
    currentBytecode()->bind(exprLabel);

    node->whileExpr()->visit(this);
    if (context().tosType() != VT_INT) throw runtime_error("While expr should be computed to int!");

    currentBytecode()->addInsn(BC_ILOAD0);
    currentBytecode()->addBranch(BC_IFICMPE, exitLabel);
    node->loopBlock()->visit(this);

    currentBytecode()->addBranch(BC_JA, exprLabel);
    currentBytecode()->bind(exitLabel);
}

void BytecodeTranslatorVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    auto stringId = code_->makeStringConstant(node->literal());
    if (stringId == 0) {
        currentBytecode()->addInsn(BC_SLOAD0);
    } else {
        currentBytecode()->addInsn(BC_SLOAD);
        currentBytecode()->addUInt16(stringId);
    }

    context().setTosType(VT_STRING);
}

void BytecodeTranslatorVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    if (node->literal() > 1 || node->literal() < -1) {
        currentBytecode()->addInsn(BC_ILOAD);
        currentBytecode()->addInt64(node->literal());
    } else {
        Instruction loads[] = {BC_ILOADM1, BC_ILOAD0, BC_ILOAD1};
        currentBytecode()->addInsn(loads[node->literal() + 1]);
    }

    context().setTosType(VT_INT);
}

void BytecodeTranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    if (node->literal() == 0) {
        currentBytecode()->addInsn(BC_DLOAD0);
    } else if (node->literal() == 1) {
        currentBytecode()->addInsn(BC_DLOAD1);
    } else if (node->literal() == -1) {
        currentBytecode()->addInsn(BC_DLOADM1);
    } else {
        currentBytecode()->addInsn(BC_DLOAD);
        currentBytecode()->addDouble(node->literal());
    }

    context().setTosType(VT_DOUBLE);
}

void BytecodeTranslatorVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);

    switch (node->kind()) {
        case tNOT:
            negateAsBool();
            break;
        case tSUB:
            switch (context().tosType()) {
                case VT_DOUBLE:
                    currentBytecode()->addInsn(BC_DNEG);
                    break;
                case VT_INT:
                    currentBytecode()->addInsn(BC_INEG);
                    break;
                default:
                    throw runtime_error("Invalid type '"s + typeToName(context().tosType()) + "' on TOS!");
            }
            break;
        default:
            throw runtime_error(
                "Not implemented operation for "s + typeToName(context().tosType())
            );
    }
}

void BytecodeTranslatorVisitor::visitNativeCallNode(NativeCallNode* node) {
    using namespace asmjit;

    static JitRuntime rt; // me being lazy

    InterpreterCodeImpl::NativeFunction result;

    void* functionToCall = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (!functionToCall) {
        throw runtime_error("Cannot find native function " + node->nativeName());
    }

    auto paramsNumber = node->nativeSignature().size() - 1;
    auto returnType = node->nativeSignature()[0].first;

    CodeHolder code;                        // Holds code and relocation information.
    code.init(rt.getCodeInfo());            // Initialize to the same arch as JIT runtime.

    FuncSignatureX signature;
    switch (returnType) {
        case VT_STRING:
            signature.setRetT<char*>();
            break;

        case VT_DOUBLE:
            signature.setRetT<double>();
            break;

        case VT_INT:
            signature.setRetT<int64_t>();
            break;

        case VT_VOID:
            signature.setRetT<void>();
            break;

        default:
            throw runtime_error("Unsupported return type");
    }

    for (uint16_t i = 0; i < paramsNumber; i++) {
        auto paramType = node->nativeSignature()[i + 1].first;
        switch (paramType) {
            case VT_STRING: {
                signature.addArgT<char*>();
                break;
            }

            case VT_DOUBLE: {
                signature.addArgT<double>();
                break;
            }

            case VT_INT: {
                signature.addArgT<int64_t>();
                break;
            }

            default:
                throw runtime_error("Unsupported native function param type");
        }
    }

    X86Compiler cc(&code);
    cc.addFunc(FuncSignature2<void, NativeVal*, NativeVal*>());

    X86Gp args = cc.newIntPtr("args");
    X86Gp res = cc.newIntPtr("res");

    cc.setArg(0, args);
    cc.setArg(1, res);

    X86Gp gpRet = cc.newGpq();
    X86Xmm xmmRet = cc.newXmmSd();

    vector<X86Xmm> xmms;
    vector<X86Gp> gpqs;
    vector<bool> isXmms;

    for (uint16_t i = 0; i < paramsNumber; i++) {
        auto paramType = node->nativeSignature()[i + 1].first;
        switch (paramType) {
            case VT_STRING: {
                gpqs.push_back(cc.newGpq()); // Gpq is for 8 byte register
                isXmms.push_back(false);
                break;
            }

            case VT_DOUBLE: {
                xmms.push_back(cc.newXmmSd()); // Sd for double precision
                isXmms.push_back(true);
                break;
            }

            case VT_INT: {
                gpqs.push_back(cc.newGpq()); // Gpq is for 8 byte register
                isXmms.push_back(false);
                break;
            }

            default:
                throw runtime_error("Unsupported native function param type");
        }
    }

    {
        uint16_t iXmm = 0;
        uint16_t iGpq = 0;
        for (uint16_t i = 0; i < paramsNumber; i++) {
            auto paramType = node->nativeSignature()[i + 1].first;
            switch (paramType) {
                case VT_DOUBLE: {
                    cc.movsd(xmms[iXmm++], x86::qword_ptr(args, i * 8));
                    break;
                }

                case VT_STRING: {
                    cc.mov(gpqs[iGpq++], x86::qword_ptr(args, i * 8));
                    break;
                }

                case VT_INT: {
                    cc.mov(gpqs[iGpq++], x86::qword_ptr(args, i * 8));
                    break;
                }

                default:
                    throw runtime_error("Unsupported native function param type");
            }
        }
    }

    auto call = cc.call(imm_ptr(functionToCall), signature);

    {
        uint16_t iXmm = 0;
        uint16_t iGpq = 0;
        for (auto it = isXmms.rbegin(); it != isXmms.rend(); ++it) {
            if (*it) {
                call->setArg(iXmm + iGpq, xmms[iXmm]);
                iXmm++;
            } else {
                call->setArg(iXmm + iGpq, gpqs[iGpq]);
                iGpq++;
            }
        }
    }

    switch (returnType) {
        case VT_STRING:
            call->setRet(0, gpRet);
            cc.mov(x86::ptr(res), gpRet);
            break;

        case VT_DOUBLE:
            call->setRet(0, xmmRet);
            cc.movsd(x86::ptr(res), xmmRet);
            break;

        case VT_INT:
            call->setRet(0, gpRet);
            cc.mov(x86::ptr(res), gpRet);
            break;

        case VT_VOID:
            break;

        default:
            throw runtime_error("Unsupported return type");
    }

    cc.endFunc();
    cc.finalize();

    Error err = rt.add(&result, &code);
    if (err) throw runtime_error("Cannot convert this function");

    auto id = code_->makeNativeFunction(node->nativeName(), node->nativeSignature(), (void*)result);
    currentBytecode()->addInsn(BC_CALLNATIVE);
    currentBytecode()->addUInt16(id);
}

bool BytecodeTranslatorVisitor::isUnusedExpression(AstNode* child) const {
    if (auto callNode = dynamic_cast<CallNode*>(child)) {
        auto returnType = code_->functionByName(callNode->name())->returnType();
        return returnType != VT_VOID && returnType != VT_INVALID;
    }

    return dynamic_cast<BinaryOpNode*>(child)
           || dynamic_cast<UnaryOpNode*>(child)
           || dynamic_cast<DoubleLiteralNode*>(child)
           || dynamic_cast<IntLiteralNode*>(child)
           || dynamic_cast<DoubleLiteralNode*>(child);
}

void BytecodeTranslatorVisitor::visitBlockNode(BlockNode* node) {
    Scope::VarIterator varIterator(node->scope());
    utils::iterate(varIterator, [this](AstVar* var) {
        context().declareVar(var->name());
    });

    Scope::FunctionIterator functionIterator(node->scope());
    utils::iterate(functionIterator, [this](AstFunction* function) {
        auto bytecodeFunction = createFunction(function);
        code_->addFunction(bytecodeFunction);
    });

    functionIterator = Scope::FunctionIterator(node->scope());
    utils::iterate(functionIterator, [this](AstFunction* function) {
        translateFunction(function);
    });

    for (uint32_t i = 0; i < node->nodes(); i++) {
        auto child = node->nodeAt(i);

        child->visit(this);

        if (isUnusedExpression(child)) {
            currentBytecode()->addInsn(BC_POP);
        }
    }
}

void BytecodeTranslatorVisitor::visitReturnNode(ReturnNode* node) {
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
        castFromTo(context().tosType(), context().returnType());
    }

    currentBytecode()->addInsn(BC_RETURN);
}

void BytecodeTranslatorVisitor::visitFunctionNode(FunctionNode* node) {
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        switch (node->parameterType(i)) {
            case VT_DOUBLE:
                currentBytecode()->addInsn(BC_STOREDVAR);
                break;
            case VT_INT:
                currentBytecode()->addInsn(BC_STOREIVAR);
                break;
            case VT_STRING:
                currentBytecode()->addInsn(BC_STORESVAR);
                break;
            default:
                throw runtime_error("Unsupported argument type");
        }
        currentBytecode()->addUInt16(context().findVarIndex(node->parameterName(i)));
    }

    node->body()->visit(this);
}

Bytecode* BytecodeTranslatorVisitor::currentBytecode() {
    return context().bytecode();
}

CallContext& BytecodeTranslatorVisitor::context() {
    return contexts_.back();
}

void BytecodeTranslatorVisitor::leaveContext() {
    contexts_.pop_back();
}

class VarHelper {
    VarType type_;
    uint16_t varIndex_;
    CallContext const* targetContext_{nullptr};

public:
    VarHelper(VarType type, uint16_t index) : type_(type), varIndex_(index) {}

    void emitStoreBytecode(Bytecode* bytecode) const {
        if (targetContext_) {
            emitContextStore(bytecode);
        } else if (varIndex_ < 4) {
            emitLocalStore(bytecode);
        } else {
            emitStore(bytecode);
        }
    }

    void emitLoadBytecode(Bytecode* bytecode) const {
        if (targetContext_) {
            emitContextLoad(bytecode);
        } else if (varIndex_ < 4) {
            emitLocalLoad(bytecode);
        } else {
            emitLoad(bytecode);
        }
    }

    void setContext(CallContext const* context) {
        targetContext_ = context;
    }

private:
    void emitLocalLoad(Bytecode* bytecode) const {
        Instruction intLoads[] = {BC_LOADIVAR0, BC_LOADIVAR1, BC_LOADIVAR2, BC_LOADIVAR3};
        Instruction doubleLoads[] = {BC_LOADDVAR0, BC_LOADDVAR1, BC_LOADDVAR2, BC_LOADDVAR3};
        Instruction strLoads[] = {BC_LOADSVAR0, BC_LOADSVAR1, BC_LOADSVAR2, BC_LOADSVAR3};

        Instruction insn;
        switch (type_) {
            case VT_DOUBLE:
                insn = doubleLoads[varIndex_];
                break;
            case VT_INT:
                insn = intLoads[varIndex_];
                break;
            case VT_STRING:
                insn = strLoads[varIndex_];
                break;
            default:
                throw runtime_error(
                    "Unsupported type of variable "s + typeToName(type_)
                );
        }

        bytecode->addInsn(insn);
    }

    void emitLocalStore(Bytecode* bytecode) const {
        Instruction intLoads[] = {BC_STOREIVAR0, BC_STOREIVAR1, BC_STOREIVAR2, BC_STOREIVAR3};
        Instruction doubleLoads[] = {BC_STOREDVAR0, BC_STOREDVAR1, BC_STOREDVAR2, BC_STOREDVAR3};
        Instruction strLoads[] = {BC_STORESVAR0, BC_STORESVAR1, BC_STORESVAR2, BC_STORESVAR3};

        Instruction insn;
        switch (type_) {
            case VT_DOUBLE:
                insn = doubleLoads[varIndex_];
                break;
            case VT_INT:
                insn = intLoads[varIndex_];
                break;
            case VT_STRING:
                insn = strLoads[varIndex_];
                break;
            default:
                throw runtime_error(
                    "Unsupported type of variable "s + typeToName(type_)
                );
        }

        bytecode->addInsn(insn);
    }

    void emitStore(Bytecode* bytecode) const {
        Instruction insn;
        switch (type_) {
            case VT_DOUBLE:
                insn = BC_STOREDVAR;
                break;
            case VT_INT:
                insn = BC_STOREIVAR;
                break;
            case VT_STRING:
                insn = BC_STORESVAR;
                break;
            default:
                throw runtime_error(
                    "Unsupported type of variable "s + typeToName(type_)
                );
        }

        bytecode->addInsn(insn);
        bytecode->addUInt16(varIndex_);
    }

    void emitLoad(Bytecode* bytecode) const {
        Instruction insn;
        switch (type_) {
            case VT_DOUBLE:
                insn = BC_LOADDVAR;
                break;
            case VT_INT:
                insn = BC_LOADIVAR;
                break;
            case VT_STRING:
                insn = BC_LOADSVAR;
                break;
            default:
                throw runtime_error(
                    "Unsupported type of variable "s + typeToName(type_)
                );
        }

        bytecode->addInsn(insn);
        bytecode->addUInt16(varIndex_);
    }

    void emitContextLoad(Bytecode* bytecode) const {
        Instruction insn;
        switch (type_) {
            case VT_DOUBLE:
                insn = BC_LOADCTXDVAR;
                break;
            case VT_INT:
                insn = BC_LOADCTXIVAR;
                break;
            case VT_STRING:
                insn = BC_LOADCTXSVAR;
                break;
            default:
                throw runtime_error(
                    "Unsupported type of variable "s + typeToName(type_)
                );
        }

        bytecode->addInsn(insn);
        bytecode->addUInt16(targetContext_->scopeId());
        bytecode->addUInt16(varIndex_);
    }

    void emitContextStore(Bytecode* bytecode) const {
        Instruction insn;
        switch (type_) {
            case VT_DOUBLE:
                insn = BC_STORECTXDVAR;
                break;
            case VT_INT:
                insn = BC_STORECTXIVAR;
                break;
            case VT_STRING:
                insn = BC_STORECTXSVAR;
                break;
            default:
                throw runtime_error(
                    "Unsupported type of variable "s + typeToName(type_)
                );
        }

        bytecode->addInsn(insn);
        bytecode->addUInt16(targetContext_->scopeId());
        bytecode->addUInt16(varIndex_);
    }

};

void BytecodeTranslatorVisitor::loadVar(AstVar const& var) {
    auto& targetContext = locateContextWithVariable(var.name());
    uint16_t index = targetContext.findVarIndex(var.name());
    VarHelper loader(var.type(), index);

    if (targetContext.scopeId() != context().scopeId()) {
        loader.setContext(&targetContext);
    }

    loader.emitLoadBytecode(currentBytecode());
    context().setTosType(var.type());
}

void BytecodeTranslatorVisitor::storeVar(const AstVar& var) {
    auto& targetContext = locateContextWithVariable(var.name());
    uint16_t varIndex = targetContext.findVarIndex(var.name());

    VarHelper helper(var.type(), varIndex);
    if (targetContext.scopeId() != context().scopeId()) {
        helper.setContext(&targetContext);
    }

    helper.emitStoreBytecode(currentBytecode());
    context().setTosType(VT_VOID);
}

void BytecodeTranslatorVisitor::binaryMathOperation(BinaryOpNode* node) {
    node->right()->visit(this);
    auto rightType = (context().tosType());
    node->left()->visit(this);
    auto leftType = (context().tosType());

    auto commonType = rightType == VT_INT
                      ? leftType
                      : VT_DOUBLE;

    if (commonType == VT_DOUBLE) {
        if (leftType == VT_INT) {
            currentBytecode()->addInsn(BC_I2D);
        }

        if (rightType == VT_INT) {
            currentBytecode()->addInsn(BC_SWAP);
            currentBytecode()->addInsn(BC_I2D);
            currentBytecode()->addInsn(BC_SWAP);
        }
    }

    Instruction insn;
    switch (node->kind()) {
        case tADD: {
            insn = commonType == VT_INT ? BC_IADD : BC_DADD;
            break;
        }
        case tSUB: {
            insn = commonType == VT_INT ? BC_ISUB : BC_DSUB;
            break;
        };
        case tMUL: {
            insn = commonType == VT_INT ? BC_IMUL : BC_DMUL;
            break;
        };
        case tDIV: {
            insn = commonType == VT_INT ? BC_IDIV : BC_DDIV;
            break;
        };

        case tMOD: {
            if (commonType != VT_INT) throw runtime_error("Cannot mod doubles!");
            insn = BC_IMOD;
            break;
        }
        default:
            throw runtime_error("Not supported math binop "s + tokenOp(node->kind()));
    }

    currentBytecode()->addInsn(insn);
    context().setTosType(commonType);
}

void BytecodeTranslatorVisitor::binaryBooleanLogicOperation(BinaryOpNode* pNode) {
    Label setZero(currentBytecode());
    Label setOne(currentBytecode());
    Label skip(currentBytecode());

    pNode->left()->visit(this);
    if (context().tosType() != VT_INT) throw runtime_error("Type is not int");
    castToBool();

    switch (pNode->kind()) {
        case tOR:
            currentBytecode()->addInsn(BC_ILOAD0);
            currentBytecode()->addBranch(BC_IFICMPNE, setOne);
            break;
        case tAND: {
            currentBytecode()->addInsn(BC_ILOAD0);
            currentBytecode()->addBranch(BC_IFICMPE, setZero);
            break;
        }
        default:
            throw runtime_error("Unsupported boolean logic operator");
    }

    pNode->right()->visit(this);
    if (context().tosType() != VT_INT) throw runtime_error("Type is not int");
    castToBool();

    currentBytecode()->addBranch(BC_JA, skip);

    currentBytecode()->bind(setOne);
    currentBytecode()->addInsn(BC_ILOAD1);
    currentBytecode()->addBranch(BC_JA, skip);

    currentBytecode()->bind(setZero);
    currentBytecode()->addInsn(BC_ILOAD0);
    currentBytecode()->addBranch(BC_JA, skip);

    currentBytecode()->bind(skip);

    context().setTosType(VT_INT);
}

void BytecodeTranslatorVisitor::binaryRelationOperation(BinaryOpNode const* node) {
    node->right()->visit(this);
    auto rightType = (context().tosType());
    node->left()->visit(this);
    auto leftType = (context().tosType());

    auto commonType = rightType == VT_INT
                      ? leftType
                      : VT_DOUBLE;

    if (commonType == VT_DOUBLE) {
        if (leftType == VT_INT) {
            currentBytecode()->addInsn(BC_I2D);
        }

        if (rightType == VT_INT) {
            currentBytecode()->addInsn(BC_SWAP);
            currentBytecode()->addInsn(BC_I2D);
            currentBytecode()->addInsn(BC_SWAP);
        }
    }

    if (commonType == VT_INT) {
        currentBytecode()->addInsn(BC_ICMP);
    } else {
        currentBytecode()->addInsn(BC_DCMP);
    }

    context().setTosType(VT_INT);

    switch (node->kind()) {
        case tEQ: {
            negateAsBool();
            break;
        }
        case tNEQ: {
            break;
        }

        case tGT: { // a > b
            currentBytecode()->addInsn(BC_ILOAD1);
            currentBytecode()->addInsn(BC_ICMP);
            negateAsBool();
            break;
        }

        case tGE: { // a >= b
            currentBytecode()->addInsn(BC_ILOADM1);
            currentBytecode()->addInsn(BC_ICMP);
            break;
        }

        case tLT: { // a < b
            currentBytecode()->addInsn(BC_ILOADM1);
            currentBytecode()->addInsn(BC_ICMP);
            negateAsBool();
            break;
        }

        case tLE: { // a <= b
            currentBytecode()->addInsn(BC_ILOAD1);
            currentBytecode()->addInsn(BC_ICMP);
            break;
        }

        default:
            throw runtime_error("Not supported relation op "s + tokenOp(node->kind()));
    }

    context().setTosType(VT_INT);
}

void BytecodeTranslatorVisitor::negateAsBool() {
    if (context().tosType() != VT_INT && context().tosType() != VT_STRING)
        throw runtime_error("Can negate only ints and strings");

    Label setOneLabel(currentBytecode());
    Label skip(currentBytecode());
    Label finish(currentBytecode());

    currentBytecode()->addInsn(BC_ILOAD0);
    currentBytecode()->addBranch(BC_IFICMPE, setOneLabel);

    currentBytecode()->addInsn(BC_ILOAD0);
    currentBytecode()->addBranch(BC_JA, finish);

    currentBytecode()->bind(setOneLabel);
    currentBytecode()->addInsn(BC_ILOAD1);

    currentBytecode()->bind(finish);

    context().setTosType(VT_INT);
}

void BytecodeTranslatorVisitor::binaryMathLogicOperation(BinaryOpNode const* node) {
    node->right()->visit(this);
    if (context().tosType() != VT_INT) throw runtime_error("Type is not int for "s + tokenOp(node->kind()));

    node->left()->visit(this);
    if (context().tosType() != VT_INT) throw runtime_error("Type is not int for "s + tokenOp(node->kind()));

    switch (node->kind()) {
        case tAAND:
            currentBytecode()->addInsn(BC_IAAND);
            break;
        case tAOR:
            currentBytecode()->addInsn(BC_IAOR);
            break;
        case tAXOR:
            currentBytecode()->addInsn(BC_IAXOR);
            break;
        default:
            throw runtime_error("Not supported arithmetic logic binop "s + tokenOp(node->kind()));
    }

    context().setTosType(VT_INT);
}

CallContext const& BytecodeTranslatorVisitor::locateContextWithVariable(string const& name) const {
    for (auto it = contexts_.rbegin(); it != contexts_.rend(); ++it) {
        if (it->containsVar(name)) {
            return *it;
        }
    }

    throw runtime_error("There's no context with variable named " + name);
}

void BytecodeTranslatorVisitor::castFromTo(VarType from, VarType to) {
    if (from == to) return;

    auto is_int_or_double = [](auto t) { return t == VT_INT || t == VT_DOUBLE; };

    if (!is_int_or_double(from) || !is_int_or_double(to)) {
        throw runtime_error("Cannot perform conversion on string types");
    }

    if (from == VT_INT) {
        currentBytecode()->addInsn(BC_I2D);
        context().setTosType(VT_DOUBLE);
    } else {
        currentBytecode()->addInsn(BC_D2I);
        context().setTosType(VT_INT);
    }
}

void BytecodeTranslatorVisitor::castToBool() {
    Label skip(context().bytecode());
    Label setFalse((context().bytecode()));

    context().bytecode()->addInsn(BC_ILOAD0);
    context().bytecode()->addBranch(BC_IFICMPE, setFalse);
    context().bytecode()->addInsn(BC_ILOAD1);
    context().bytecode()->addBranch(BC_JA, skip);
    context().bytecode()->bind(setFalse);
    context().bytecode()->addInsn(BC_ILOAD0);
    context().bytecode()->bind(skip);
}

