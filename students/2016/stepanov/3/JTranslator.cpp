//
// Created by user on 11/30/16.
//

#include <string>
#include "../../../../libs/asmjit/asmjit.h"
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include <dlfcn.h>
#include <memory>
#include <cmath>
#include "../../../../vm/parser.h"
#include "ScopeData.h"
#include "VmException.h"
#include "RFUtils.h"
#include "JTranslator.h"
#include "jRuntime.h"
#include "JVariableAnnotator.h"
#include "BytecodeRFInterpreter.h"

using namespace asmjit;
using namespace asmjit::x86;


namespace mathvm {
    asmjit::Label doubleMin;
    asmjit::Label doubleZero;

    asmjit::Label jitedFunctions[UINT16_MAX + 12];

    const uint32_t argsTypes[] = {TypeId<void>::kId, TypeId<void>::kId, TypeId<double>::kId,
                                  TypeId<int64_t>::kId, TypeId<char *>::kId};

    const uint32_t argsTypesPointed[] = {TypeId<void>::kId, TypeId<void>::kId, TypeId<double *>::kId,
                                         TypeId<int64_t *>::kId, TypeId<char *>::kId};

    JVisitor::~JVisitor() {

    }

    JVisitor::JVisitor() : assembler(&runtime) {
        printDouble.addArg(kVarTypeFp64);
        printDouble.setRet(kVarTypeInt64);
        printInt.addArg(kVarTypeInt64);
        printInt.setRet(kVarTypeInt64);
        printString.addArg(kVarTypeIntPtr);
        printString.setRet(kVarTypeInt64);
        signatureIII.setRet(kVarTypeInt64);
        signatureIII.addArg(kVarTypeInt64);
        signatureIII.addArg(kVarTypeInt64);
        signatureIDD.setRet(kVarTypeInt64);
        signatureIDD.addArg(kVarTypeFp64);
        signatureIDD.addArg(kVarTypeFp64);

        signatureLongCallD.setRet(kVarTypeFp64);
        signatureLongCallD.addArg(kVarTypeIntPtr);

        signatureLongCallI.setRet(kVarTypeInt64);
        signatureLongCallI.addArg(kVarTypeIntPtr);

        signatureLongCallS.setRet(kVarTypeIntPtr);
        signatureLongCallS.addArg(kVarTypeIntPtr);

        signatureLongCallV.setRet(TypeId<void>::kId);
        signatureLongCallV.addArg(kVarTypeIntPtr);
    }


    Status *JVisitor::runTranslate(Code *code, AstFunction *function) {
#ifdef  SHOW_LOG
        FileLogger logger(stderr);
        assembler.setLogger(&logger);
#endif


        try {
            currentSd = make_shared<JScopeData>(function);
            currentSd->addFunction(function);

            topCompiler = new X86Compiler(&assembler);
            registerFunction(function);


            topCompiler->addFunc(FuncBuilder0<void>(kCallConvHost));
            doubleMin = topCompiler->newLabel();
            doubleZero = topCompiler->newLabel();
            int64_t function_id = 0;
            currentSd->lookupFunctionByName(function->top_name, function_id);

            topCompiler->call(jitedFunctions[function_id],
                              FuncBuilder0<void>(kCallConvHost));

            topCompiler->ret();
            bindAll();
            topCompiler->endFunc();


            while (!waitedFunctions.empty()) {
                FunctionAnalysData next = waitedFunctions.top();
                waitedFunctions.pop();
                currentSd = next.scope;
                translateFunction(next.function);
            }

            topCompiler->finalize();
            delete (topCompiler);

            void *result = assembler.make();

            reinterpret_cast<void (*)(void)>(result)();
        }
        catch (mathvm::VmException *ex) {
            return Status::Error(ex->what(), ex->getPosition());
        }
        catch (std::exception *ex) {
            return Status::Error(ex->what());
        }
        return Status::Ok();
    }

    void JVisitor::registerFunction(AstFunction *function) {
        int64_t function_id = -1;
        AstFunction *currentFunctionBc = currentSd->lookupFunctionByName(function->name(), function_id);
        if (currentFunctionBc == nullptr) {
            throw new std::logic_error("Cannot find finction " + function->name());
        }
        auto new_sd = make_shared<JScopeData>(currentFunctionBc, topCompiler, currentSd);

        for (uint32_t currentParameter = 0; currentParameter < function->parametersNumber(); ++currentParameter) {
            AstVar *variable = function->scope()->lookupVariable(function->parameterName(currentParameter), false);
            new_sd->addVariable(variable, false);
        }

        jitedFunctions[function_id] = new_sd->compiler->newLabel();

        waitedFunctions.push(FunctionAnalysData(function, new_sd));
    }

    void JVisitor::translateFunction(AstFunction *function) {
        //currentSd->compiler->comment(("Start of function " + function->name()).c_str());
        //[TODO] currentFunctionBc -> function
        int64_t function_id = -1;
        AstFunction *currentFunctionBc = currentSd->lookupFunctionByName(function->name(), function_id);
        if (currentFunctionBc == nullptr) {
            throw new std::logic_error("Cannot find finction " + function->name());
        }

        HLFunc thisFunction(currentSd->compiler);
        FuncPrototype prototype;

        NodeScopeData *nsd = functionsNodeScopeData[currentFunctionBc];

        vector<uint32_t> argumentTypes;
        argumentTypes.reserve(currentFunctionBc->parametersNumber() + nsd->captured.size());

        for (auto it = nsd->captured.begin(); it != nsd->captured.end(); ++it) {
            const uint32_t *targetTypePointer = (it->isPointer) ? &(argsTypesPointed[0]) : &(argsTypes[0]);
            argumentTypes.push_back(targetTypePointer[it->type]);
        }

        for (uint32_t i = 0; i < currentFunctionBc->parametersNumber(); ++i) {
            argumentTypes.push_back(argsTypes[currentFunctionBc->parameterType(i)]);
        }

        bool standardFunctionCall = argumentTypes.size() <= kFuncArgCount;

        if (standardFunctionCall) {
            prototype.setup(kCallConvHost, argsTypes[currentFunctionBc->returnType()], &argumentTypes[0],
                            argumentTypes.size());
        } else {
            switch (currentFunctionBc->returnType()) {
                case VT_VOID:
                    prototype = signatureLongCallV;
                    break;
                case VT_DOUBLE:
                    prototype = signatureLongCallD;
                    break;
                case VT_INT:
                    prototype = signatureLongCallI;
                    break;
                case VT_STRING:
                    prototype = signatureLongCallS;
                    break;
                default:
                    assert(false);
            }
        }

        currentSd->compiler->bind(jitedFunctions[function_id]);
        currentSd->compiler->addFunc(prototype);

        uint32_t argumentId = 0;
        if (standardFunctionCall) {
            for (auto it = nsd->captured.begin(); it != nsd->captured.end(); ++it) {
                X86Var *variable = currentSd->addVariable(it->name, it->type, /*firstTime*/false, /*isPointer*/true);
                currentSd->compiler->setArg(argumentId++, *variable);
            }

            for (uint32_t i = 0; i < currentFunctionBc->parametersNumber(); ++i) {
                AstVar *variable = function->scope()->lookupVariable(function->parameterName(i), false);
                X86Var *currentArg = currentSd->addVariable(variable, false);
                currentSd->compiler->setArg(argumentId++, *currentArg);
            }

        } else {
            const X86GpVar argumentsLocation = currentSd->compiler->newIntPtr();
            currentSd->compiler->setArg(0, argumentsLocation);

            for (auto it = nsd->captured.begin(); it != nsd->captured.end(); ++it) {
                X86Var *variable = currentSd->addVariable(it->name, it->type, /* firstTime */ false, /*isPointer*/true);
                switch (it->type) {
                    case VT_DOUBLE:
                        currentSd->compiler->movsd(*(X86XmmVar *) variable,
                                                   ptr(argumentsLocation, argumentId * 8, 8));
                        break;
                    case VT_INT:
                    case VT_STRING:
                        currentSd->compiler->mov(*(X86GpVar *) variable, ptr(argumentsLocation, argumentId * 8, 8));
                        break;
                    default:
                        assert(false);
                }
                ++argumentId;
            }

            for (uint32_t i = 0; i < currentFunctionBc->parametersNumber(); ++i) {
                AstVar *variable = function->scope()->lookupVariable(function->parameterName(i), false);
                X86Var *currentArg = currentSd->addVariable(variable, false);
                switch (currentFunctionBc->parameterType(i)) {
                    case VT_DOUBLE:
                        currentSd->compiler->movsd(*(X86XmmVar *) currentArg,
                                                   ptr(argumentsLocation, argumentId * 8, 8));
                        break;
                    case VT_INT:
                    case VT_STRING:
                        currentSd->compiler->mov(*(X86GpVar *) currentArg, ptr(argumentsLocation, argumentId * 8, 8));
                        break;
                    default:
                        assert(false);
                }
                ++argumentId;
            }
        }
        function->node()->visit(this);

        if (function->returnType() == VT_VOID) {
            currentSd->compiler->ret();
        }

        currentSd->compiler->endFunc();
        //currentSd->compiler->comment(("End of function " + function->name()).c_str());
    }

    void JVisitor::dumpByteCode(ostream &out) {

    }

    void JVisitor::scopeEvaluator(Scope *scope) {
        Scope::VarIterator it = Scope::VarIterator(scope);
        while (it.hasNext()) {
            AstVar *next = it.next();
            currentSd->addVariable(next, true);
        }
        Scope::FunctionIterator itFun = Scope::FunctionIterator(scope);
        while (itFun.hasNext()) {
            int64_t function_id = -1;
            AstFunction *next = itFun.next();
            AstFunction *currentByteFunction = currentSd->lookupFunctionByName(next->name(), function_id, false);
            if (currentByteFunction == nullptr) {
                currentSd->addFunction(next);
            }
        }
        //only after index all names
        itFun = Scope::FunctionIterator(scope);
        while (itFun.hasNext()) {
            AstFunction *next = itFun.next();
            registerFunction(next);
        }
    }

    void JVisitor::storeVariable(const AstVar *var) {
        AsmScopeVariable *variableInfo = currentSd->lookupVariableInfo(var->name());
        if (variableInfo == nullptr) {
            throw new std::logic_error("Cannot find variable " + var->name());
        }

        if (variableInfo->readOnly) {
            variableInfo->readOnly = false;
            switch (variableInfo->type) {
                case VT_DOUBLE:
                    variableInfo->var = new asmjit::X86XmmVar();
                    *variableInfo->var = currentSd->compiler->newXmm();
                    break;
                case VT_INT:
                    variableInfo->var = new asmjit::X86GpVar();
                    *variableInfo->var = currentSd->compiler->newInt64();
                    break;
                case VT_STRING:
                    variableInfo->var = new asmjit::X86GpVar();
                    *variableInfo->var = currentSd->compiler->newIntPtr();
                    break;
                default:
                    break;
            }
        }


        X86Var *top = jStack.top();
        jStack.pop();
        switch (var->type()) {
            case VT_DOUBLE:
                if (variableInfo->isPointer) {
                    currentSd->compiler->movq(ptr(*((X86GpVar *) variableInfo->var)), *((X86XmmVar *) top));
                } else {
                    currentSd->compiler->movq(*((X86XmmVar *) variableInfo->var), *((X86XmmVar *) top));
                }
                break;
            case VT_INT:
            case VT_STRING:
                //[TODO]
                if (variableInfo->isPointer) {
                    currentSd->compiler->mov(ptr(*((X86GpVar *) variableInfo->var)), *((X86GpVar *) top));
                } else {
                    currentSd->compiler->mov(*((X86GpVar *) variableInfo->var), *((X86GpVar *) top));
                }
                break;
            default:
                assert(false);
        }


        currentSd->topType = VT_VOID;
    }

    void JVisitor::loadVariable(const AstVar *var) {
        AsmScopeVariable *variableInfo = currentSd->lookupVariableInfo(var->name());
        if (variableInfo == nullptr) {
            throw new std::logic_error("Cannot find variable " + var->name());
        }
        if (variableInfo->readOnly) {
            variableInfo->readOnly = false;
            switch (variableInfo->type) {
                case VT_DOUBLE:
                    variableInfo->var = new asmjit::X86XmmVar();
                    *variableInfo->var = currentSd->compiler->newXmm();
                    break;
                case VT_INT:
                    variableInfo->var = new asmjit::X86GpVar();
                    *variableInfo->var = currentSd->compiler->newInt64();
                    break;
                case VT_STRING:
                    variableInfo->var = new asmjit::X86GpVar();
                    *variableInfo->var = currentSd->compiler->newIntPtr();
                    break;
                default:
                    break;
            }
        }

        if (variableInfo->isPointer) {
            X86Var *unpointedValue;
            switch (var->type()) {
                case VT_DOUBLE:
                    unpointedValue = new X86XmmVar();
                    *unpointedValue = currentSd->compiler->newXmm();
                    currentSd->compiler->movsd(*(X86XmmVar *) unpointedValue, ptr(*(X86GpVar *) (variableInfo->var)));
                    jStack.push(unpointedValue);
                    break;
                case VT_STRING:
                case VT_INT: {
                    unpointedValue = new X86GpVar();
                    *unpointedValue = currentSd->compiler->newInt64();
                    currentSd->compiler->mov(*(X86GpVar *) unpointedValue, ptr(*(X86GpVar *) (variableInfo->var)));
                    jStack.push(unpointedValue);
                }
                    break;
                default:
                    assert(false);
            }
        } else {
            jStack.push(variableInfo->var);
        }

        currentSd->topType = var->type();
    }

    void JVisitor::prepareTopType(VarType param) {
        X86Var *top = jStack.top();
        X86Var *casted;
        switch (currentSd->topType) {
            case VT_DOUBLE:
                switch (param) {
                    case VT_DOUBLE:
                        break;
                    case VT_INT:
                        casted = new X86GpVar();
                        *casted = currentSd->compiler->newInt64();
                        currentSd->compiler->cvttsd2si(*(X86GpVar *) casted, *(X86XmmVar *) top);
                        jStack.pop();
                        jStack.push(casted);
                        break;
                    default:
                        throw new std::logic_error("incorrect cast");
                }
                break;
            case VT_INT:
                switch (param) {
                    case VT_DOUBLE:
                        casted = new X86XmmVar();
                        *casted = currentSd->compiler->newXmm();
                        currentSd->compiler->cvtsi2sd(*(X86XmmVar *) casted, *(X86GpVar *) top);
                        jStack.pop();
                        jStack.push(casted);
                        break;
                    case VT_INT:
                        break;
                    default:
                        throw new std::logic_error("incorrect cast");
                }
                break;
            case VT_STRING:
                switch (param) {
                    case VT_STRING:
                    case VT_INT:
                        break;
                    default:
                        throw new std::logic_error("incorrect cast to string");
                }
                break;
            default:
                throw new std::logic_error("incorrect cast");
        }
        currentSd->topType = param;
    }

    void JVisitor::visitForNode(ForNode *node) {
        BinaryOpNode *range = node->inExpr()->asBinaryOpNode();
        asmjit::Label endLabel = currentSd->compiler->newLabel();

        if (node->var() == nullptr) {
            throw new VmException("cannot find for index variable", node->position());
        }

        range->left()->visit(this);
        storeVariable(node->var());
        range->right()->visit(this);

        X86GpVar rightBottom = currentSd->compiler->newInt64();
        X86Var *top = jStack.top();
        jStack.pop();

        currentSd->compiler->mov(rightBottom, *((X86GpVar *) top));

        const asmjit::Label continueLabel = currentSd->compiler->newLabel();

        X86GpVar *currentCounter;
        currentSd->compiler->bind(continueLabel);
        loadVariable(node->var());
        currentCounter = (X86GpVar *) jStack.top();
        jStack.pop();
        currentSd->compiler->cmp(*currentCounter, rightBottom);
        currentSd->compiler->jg(endLabel);
        visitBlockNode(node->body());
        loadVariable(node->var());
        currentCounter = (X86GpVar *) jStack.top();
        currentSd->compiler->inc(*currentCounter);
        storeVariable(node->var());
        currentSd->compiler->jmp(continueLabel);
        currentSd->compiler->bind(endLabel);
    }

    void JVisitor::visitPrintNode(PrintNode *node) {
        for (uint32_t i = 0; i < node->operands(); i++) {
            node->operandAt(i)->visit(this);
            X86Var *targetVar;
            X86CallNode *call;
            switch (currentSd->topType) {
                case VT_DOUBLE:
                    targetVar = jStack.top();
                    jStack.pop();
                    call = currentSd->compiler->call(imm_ptr(print_double), printDouble);
                    call->setArg(0, *(X86XmmVar *) targetVar);
                    break;
                case VT_INT:
                    targetVar = jStack.top();
                    jStack.pop();
                    call = currentSd->compiler->call(imm_ptr(print_int), printInt);
                    call->setArg(0, *(X86GpVar *) targetVar);
                    break;
                case VT_STRING:
                    targetVar = jStack.top();
                    jStack.pop();
                    call = currentSd->compiler->call(imm_ptr(print_string), printString);
                    call->setArg(0, *(X86GpVar *) targetVar);
                    break;
                case VT_INVALID:
                case VT_VOID:
                default:
                    throw new std::logic_error("print unexpected type");
            }
            currentSd->topType = VT_VOID;
        }
    }

    void JVisitor::visitLoadNode(LoadNode *node) {
        loadVariable(node->var());
    }

    void JVisitor::visitIfNode(IfNode *node) {
        node->ifExpr()->visit(this);

        asmjit::Label elseLabel = currentSd->compiler->newLabel();
        asmjit::Label endLabel = currentSd->compiler->newLabel();
        bool elseExists = node->elseBlock() != nullptr;

        prepareTopType(VT_INT);
        X86GpVar *top = (X86GpVar *) jStack.top();
        jStack.pop();

        currentSd->compiler->cmp(*top, 0);
        currentSd->compiler->je((elseExists) ? elseLabel : endLabel);
        currentSd->topType = VT_VOID;
        visitBlockNode(node->thenBlock());
        if (elseExists) {
            currentSd->compiler->jmp(endLabel);
            currentSd->compiler->bind(elseLabel);
            visitBlockNode(node->elseBlock());
        }
        currentSd->compiler->bind(endLabel);
    }

    void JVisitor::visitBinaryOpNode(BinaryOpNode *node) {
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
                processLogicOperation(node);
                break;

            case tAOR:
            case tAAND:
            case tAXOR:
                processBitwiseOperation(node);
                break;

            case tEQ:
            case tNEQ:
            case tGT:
            case tGE:
            case tLT:
            case tLE:
                processComparisonOperation(node);
                break;

            default:
                throw new std::logic_error("unexpected operator");
        }
    }

    void JVisitor::visitStoreNode(StoreNode *node) {
        node->value()->visit(this);
        prepareTopType(node->var()->type());

        X86Var *oldVariable = nullptr;
        X86Var *variable = nullptr;
        X86Var *value;
        if ((node->op() == tINCRSET) || (node->op() == tDECRSET)) {
            loadVariable(node->var());
            oldVariable = jStack.top();
            jStack.pop();
            switch (node->var()->type()) {
                case VT_DOUBLE:
                    variable = new asmjit::X86XmmVar();
                    *variable = currentSd->compiler->newXmm();
                    currentSd->compiler->movsd(*(X86XmmVar *) variable, *(X86XmmVar *) oldVariable);
                    break;
                case VT_INT:
                case VT_STRING:
                    variable = new asmjit::X86GpVar();
                    *variable = currentSd->compiler->newInt64();
                    currentSd->compiler->mov(*(X86GpVar *) variable, *(X86GpVar *) oldVariable);
                    break;
                default:
                    assert(false);
            }
        }

        value = jStack.top();
        jStack.pop();

        switch (node->op()) {
            case tINCRSET:
                switch (node->var()->type()) {
                    case VT_DOUBLE:
                        currentSd->compiler->addsd(*(X86XmmVar *) variable, *(X86XmmVar *) value);
                        break;
                    case VT_INT:
                        currentSd->compiler->add(*(X86GpVar *) variable, *(X86GpVar *) value);
                        break;
                    default:
                        assert(false);
                }
                break;
            case tDECRSET:
                switch (node->var()->type()) {
                    case VT_DOUBLE:
                        currentSd->compiler->subsd(*(X86XmmVar *) variable, *(X86XmmVar *) value);
                        break;
                    case VT_INT:
                        currentSd->compiler->sub(*(X86GpVar *) variable, *(X86GpVar *) value);
                        break;
                    default:
                        assert(false);
                }
                break;
            case tASSIGN:
                switch (node->var()->type()) {
                    case VT_DOUBLE:
                        variable = new asmjit::X86XmmVar();
                        *variable = currentSd->compiler->newXmm();
                        currentSd->compiler->movsd(*(X86XmmVar *) variable, *(X86XmmVar *) value);
                        break;
                    case VT_INT:
                    case VT_STRING:
                        variable = new asmjit::X86GpVar();
                        *variable = currentSd->compiler->newInt64();
                        currentSd->compiler->mov(*(X86GpVar *) variable, *(X86GpVar *) value);
                        break;
                    default:
                        assert(false);
                }
                break;
            default:
                throw new std::logic_error("unexpected save action");
        }
        jStack.push(variable);
        currentSd->topType = node->var()->type();
        storeVariable(node->var());
    }

    void JVisitor::visitStringLiteralNode(StringLiteralNode *node) {
        AstVisitor::visitStringLiteralNode(node);
        auto it = constantMap.find(node->literal());
        int id;
        if (it != constantMap.end()) {
            id = it->second;
        } else {
            id = constantPoolId++;
            constantMap[node->literal()] = id;
            constantPool[id] = (char *) node->literal().c_str();
        }

        X86GpVar *variable = new X86GpVar();
        *variable = currentSd->compiler->newIntPtr();
        currentSd->compiler->mov(*variable, imm_ptr(*(constantPool + id)));
        jStack.push(variable);
        currentSd->topType = VT_STRING;
    }

    void JVisitor::visitWhileNode(WhileNode *node) {
        asmjit::Label startLabel = currentSd->compiler->newLabel();
        asmjit::Label endLabel = currentSd->compiler->newLabel();
        currentSd->compiler->bind(startLabel);
        //currentSd->compiler->comment("startLabel");
        node->whileExpr()->visit(this);
        X86GpVar *top = (X86GpVar *) jStack.top();
        jStack.pop();
        currentSd->compiler->cmp(*top, 0);
        currentSd->compiler->je(endLabel);
        //currentSd->compiler->comment("loop block");
        visitBlockNode(node->loopBlock());
        //currentSd->compiler->comment("end loop block");
        currentSd->compiler->jmp(startLabel);
        currentSd->compiler->bind(endLabel);
    }

    void JVisitor::visitIntLiteralNode(IntLiteralNode *node) {
        X86Mem constant = currentSd->compiler->newInt64Const(kConstScopeLocal, node->literal());
        X86GpVar *variable = new X86GpVar();
        *variable = currentSd->compiler->newInt64();
        currentSd->compiler->mov(*variable, constant);
        jStack.push(variable);
        currentSd->topType = VT_INT;
    }

    void JVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
        X86Mem constant = currentSd->compiler->newDoubleConst(kConstScopeLocal, node->literal());
        X86XmmVar *variable = new X86XmmVar();
        *variable = currentSd->compiler->newXmm();
        currentSd->compiler->movq(*variable, constant);
        jStack.push(variable);
        currentSd->topType = VT_DOUBLE;
    }

    void JVisitor::visitUnaryOpNode(UnaryOpNode *node) {
        node->operand()->visit(this);
        VarType type = currentSd->topType;
        X86Var *temp;
        switch (node->kind()) {
            case tNOT:
                prepareTopType(VT_INT);
                temp = new X86GpVar();
                *temp = currentSd->compiler->newInt64();
                currentSd->compiler->cmp(*(X86GpVar *) jStack.top(), 0);
                jStack.pop();
                currentSd->compiler->sete((*(X86GpVar *) temp).r8());
                currentSd->compiler->movzx(*(X86GpVar *) temp, (*(X86GpVar *) temp).r8());
                jStack.push(temp);
                break;
            case tADD:
                break;
            case tSUB:
                switch (type) {
                    case VT_DOUBLE: {
                        temp = new XmmVar();
                        *temp = currentSd->compiler->newXmm();
                        X86XmmVar *top = (X86XmmVar *) jStack.top();
                        X86XmmVar sign = currentSd->compiler->newXmm();
                        jStack.pop();
                        currentSd->compiler->movq(sign, ptr(doubleMin));
                        currentSd->compiler->movsd(*(X86XmmVar *) temp, *top);
                        currentSd->compiler->xorpd(*(X86XmmVar *) temp, sign);
                        currentSd->compiler->unuse(sign);
                        jStack.push(temp);
                    }
                        break;
                    case VT_INT: {
                        temp = new X86GpVar();
                        *temp = currentSd->compiler->newInt64();
                        X86GpVar *top = (X86GpVar *) jStack.top();
                        jStack.pop();
                        currentSd->compiler->mov(*(X86GpVar *) temp, *top);
                        currentSd->compiler->neg(*(X86GpVar *) temp);
                        jStack.push(temp);
                    }
                        break;
                    default:
                        throw new VmException("unexpected type for negative operator", node->position());
                }
                break;
            default:
                throw new VmException("unexpected unary operator", node->position());
        }
    }

    void JVisitor::visitNativeCallNode(NativeCallNode *node) {
        AstVisitor::visitNativeCallNode(node);
    }

    void JVisitor::visitBlockNode(BlockNode *node) {
        auto old_sd = currentSd;
        currentSd = make_shared<JScopeData>(old_sd);
        currentSd->currentNodeScope = ((AScopeData *) node->info())->currentNodeScope;
        scopeEvaluator(node->scope());
        for (uint32_t i = 0; i < node->nodes(); i++) {
            node->nodeAt(i)->visit(this);
        }
        old_sd->topType = currentSd->topType;
        currentSd = old_sd;
    }

    void JVisitor::visitFunctionNode(FunctionNode *node) {
        node->body()->visit(this);
    }

    void JVisitor::visitReturnNode(ReturnNode *node) {
        if (node->returnExpr() == nullptr) {
            if (currentSd->returnLabel == nullptr) {
                currentSd->compiler->ret();
            } else {
                currentSd->compiler->jmp(*currentSd->returnLabel);
            }
            return;
        }

        node->returnExpr()->visit(this);
        const VarType expectedType = currentSd->containedFunction->returnType();
        if (expectedType != currentSd->topType) {
            prepareTopType(expectedType);
            currentSd->topType = expectedType;
        }
        X86Var *retVal = jStack.top();
        jStack.pop();

        if (currentSd->returnVariable == nullptr) {
            if (retVal->isGp()) {
                currentSd->compiler->ret(*(X86GpVar *) retVal);
            } else {
                currentSd->compiler->ret(*(X86XmmVar *) retVal);
            }
        } else {
            if (retVal->isGp()) {
                currentSd->compiler->mov(*(X86GpVar *) currentSd->returnVariable, *(X86GpVar *) retVal);
            } else {
                currentSd->compiler->movq(*(X86XmmVar *) currentSd->returnVariable, *(X86XmmVar *) retVal);
            }
            currentSd->compiler->jmp(*currentSd->returnLabel);
        }
    }

    void JVisitor::makeInline(CallNode *node, AstFunction *bf, NodeScopeData *nsd, int64_t function_id) {
        //currentSd->compiler->comment(("Start INLINE of function " + bf->name()).c_str());

        auto old_sd = currentSd;
        currentSd = make_shared<JScopeData>(old_sd);
        currentSd->currentNodeScope = ((AScopeData *) bf->info())->currentNodeScope;

        for (uint32_t i = node->parametersNumber(); i >= 1; i--) {
            node->parameterAt(i - 1)->visit(this);
            prepareTopType(bf->parameterType(i - 1));
            X86Var *top = jStack.top();
            jStack.pop();
            X86Var *copyTop = nullptr;
            switch (bf->parameterType(i - 1)) {
                case VT_DOUBLE:
                    copyTop = new asmjit::X86XmmVar();
                    *copyTop = currentSd->compiler->newXmm();
                    currentSd->compiler->movq(*(X86XmmVar *) copyTop, *(X86XmmVar *) top);
                    break;
                case VT_INT:
                    copyTop = new asmjit::X86GpVar();
                    *copyTop = currentSd->compiler->newInt64();
                    currentSd->compiler->mov(*(X86GpVar *) copyTop, *(X86GpVar *) top);
                    break;
                case VT_STRING:
                    copyTop = new asmjit::X86GpVar();
                    *copyTop = currentSd->compiler->newIntPtr();
                    currentSd->compiler->mov(*(X86GpVar *) copyTop, *(X86GpVar *) top);
                    break;
                default:
                    break;
            }

            currentSd->registerVariable(bf->parameterName(i - 1), copyTop, bf->parameterType(i - 1), false, false);

        }
        X86Var *returnVar;
        switch (bf->returnType()) {
            case VT_DOUBLE:
                returnVar = new X86XmmVar();
                *returnVar = old_sd->compiler->newXmm();
                break;
            case VT_INT:
                returnVar = new X86GpVar();
                *returnVar = old_sd->compiler->newInt64();
                break;
            case VT_STRING:
                returnVar = new X86GpVar();
                *returnVar = old_sd->compiler->newIntPtr();
                break;
            default:
                returnVar = nullptr;
        }
        currentSd->returnVariable = returnVar;
        currentSd->returnLabel = new asmjit::Label();
        *(currentSd->returnLabel) = old_sd->compiler->newLabel();
        currentSd->containedFunction = bf;
        bf->node()->visit(this);


        HLNode *possibleJmpCursor = currentSd->compiler->getCursor();

        currentSd->compiler->bind(*currentSd->returnLabel);

        HLNode *labelCursor = currentSd->compiler->getCursor();
        if (possibleJmpCursor->isJmp()) {
            HLNode *node = static_cast<HLJump *>(possibleJmpCursor)->getTarget();
            // Stop on jump that is not followed.
            if (node == labelCursor) {
                currentSd->compiler->removeNode(possibleJmpCursor);
            }
        }

        old_sd->topType = bf->returnType();
        currentSd = old_sd;
        jStack.push(returnVar);
        //currentSd->compiler->comment(("End INLINE of function " + bf->name()).c_str());
    }

    void JVisitor::visitCallNode(CallNode *node) {
        int64_t function_id = -1;
        AstFunction *bf = currentSd->lookupFunctionByName(node->name(), function_id);
        if (bf == nullptr) {
            throw new std::logic_error("cannot found target call function");
        }
        if (bf->parametersNumber() != node->parametersNumber()) {
            throw new std::logic_error("incorrect count of arguments for function call");
        }
        NodeScopeData *nsd = functionsNodeScopeData[bf];
        bool isNativeCall = (bf->node()->body()->nodes() > 0) && (bf->node()->body()->nodeAt(0)->isNativeCallNode());
        if (!nsd->hasInternalCalls && !isNativeCall) {
            makeInline(node, bf, nsd, function_id);
            return;
        }

        HLFunc thisFunction(currentSd->compiler);
        FuncPrototype prototype;
        vector<uint32_t> argumentTypes;

        argumentTypes.reserve(bf->parametersNumber() + nsd->captured.size());

        std::vector<asmjit::X86Var *> closureVariables;
        std::vector<asmjit::X86Mem *> stackPointers;


        for (auto it = nsd->captured.begin(); it != nsd->captured.end(); ++it) {
            const uint32_t *targetTypePointer = (it->isPointer) ? &(argsTypesPointed[0]) : &(argsTypes[0]);
            argumentTypes.push_back(targetTypePointer[it->type]);
            AsmScopeVariable *variable = currentSd->lookupVariableInfo(it->name);

            if (variable->isPointer) {
                closureVariables.push_back(variable->var);
            } else {
                switch (variable->type) {
                    case VT_DOUBLE: {
                        X86GpVar *myPtr = new X86GpVar();
                        *myPtr = currentSd->compiler->newIntPtr();
                        //currentSd->compiler->comment("addDClss");
                        X86Mem *stacked = new X86Mem();
                        *stacked = currentSd->compiler->newStack(8, 8);
                        stackPointers.push_back(stacked);
                        currentSd->compiler->movsd(*stacked, *(X86XmmVar *) (variable->var));
                        currentSd->compiler->lea(*myPtr, *stacked);
                        closureVariables.push_back(myPtr);
                    }
                        break;
                    case VT_STRING:
                    case VT_INT: {
                        X86GpVar *myPtr = new X86GpVar();
                        *myPtr = currentSd->compiler->newIntPtr();
                        //currentSd->compiler->comment("addClsr");
                        X86Mem *stacked = new X86Mem();
                        *stacked = currentSd->compiler->newStack(8, 8);
                        stackPointers.push_back(stacked);
                        currentSd->compiler->mov(*stacked, *(X86GpVar *) (variable->var));
                        currentSd->compiler->lea(*myPtr, *stacked);
                        closureVariables.push_back(myPtr);
                    }
                        break;
                    default:
                        assert(false);
                }
            }
        }

        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            argumentTypes.push_back(argsTypes[bf->parameterType(i)]);
        }

        bool standardFunctionCall = argumentTypes.size() <= kFuncArgCount;

        if (standardFunctionCall) {
            prototype.setup(kCallConvHost, argsTypes[bf->returnType()], &argumentTypes[0],
                            argumentTypes.size());
        } else {
            switch (bf->returnType()) {
                case VT_VOID:
                    prototype = signatureLongCallV;
                    break;
                case VT_DOUBLE:
                    prototype = signatureLongCallD;
                    break;
                case VT_INT:
                    prototype = signatureLongCallI;
                    break;
                case VT_STRING:
                    prototype = signatureLongCallS;
                    break;
                default:
                    assert(false);
            }
        }

        int xmmCnt = 0;
        for (auto it = nsd->captured.begin(); it != nsd->captured.end(); ++it) {
            if ((it->type == VT_DOUBLE) && (!it->isPointer)) {
                ++xmmCnt;
            }
        }

        for (uint32_t i = node->parametersNumber(); i >= 1; i--) {
            node->parameterAt(i - 1)->visit(this);
            prepareTopType(bf->parameterType(i - 1));
            if (bf->parameterType(i - 1) == VT_DOUBLE) {
                ++xmmCnt;
            }
        }
        xmmCnt = std::min(xmmCnt, 8);

        X86CallNode *callNode;
        bool requireSetXmmCnt = false;


        if (standardFunctionCall) {
            setupArgumentsForTraditionalCall(bf, node, callNode, prototype, nsd, closureVariables, function_id, xmmCnt,
                                             requireSetXmmCnt, isNativeCall);
        } else {
            setupArgumentsForLongCall(bf, node, callNode, prototype, nsd, closureVariables, function_id, xmmCnt,
                                      requireSetXmmCnt, isNativeCall);
        }

        currentSd->topType = bf->returnType();
        X86Var *resultOfCall;
        X86Var *fallback;
        switch (currentSd->topType) {
            case VT_VOID:
                break;
            case VT_DOUBLE:
                resultOfCall = (X86Var *) new XmmVar();
                fallback = (X86Var *) new XmmVar();

                *resultOfCall = currentSd->compiler->newXmm();
                *fallback = currentSd->compiler->newXmm();
                callNode->setRet(0, *(X86XmmVar *) resultOfCall);
                callNode->setRet(1, *(X86XmmVar *) fallback);
                jStack.push(resultOfCall);
                break;
            case VT_INT:
            case VT_STRING:
                resultOfCall = (X86Var *) new GpVar();
                fallback = (X86Var *) new GpVar();
                *resultOfCall = currentSd->compiler->newInt64();
                *fallback = currentSd->compiler->newInt64();
                callNode->setRet(0, *(X86GpVar *) resultOfCall);
                callNode->setRet(1, *(X86GpVar *) fallback);
                jStack.push(resultOfCall);
                break;
            default:
                assert(false);
        }

        if (requireSetXmmCnt) {
            setXmmDataForLastCall(xmmCnt);
        }

        saveCapturedVariables(nsd, stackPointers);
    }

    void JVisitor::setupArgumentsForLongCall(AstFunction *bf,
                                             CallNode *node,
                                             X86CallNode *&callNode,
                                             FuncPrototype &prototype,
                                             const NodeScopeData *nsd,
                                             const std::vector<asmjit::X86Var *> &closureVariables,
                                             const int64_t &function_id,
                                             const int &xmmCnt,
                                             bool &requireSetXmmCnt,
                                             bool isNativeCall
    ) {
        const X86Mem stackAddr = currentSd->compiler->newStack((nsd->captured.size() + bf->parametersNumber()) * 8, 8);
        const X86GpVar stackPtr = currentSd->compiler->newIntPtr();
        currentSd->compiler->lea(stackPtr, stackAddr);

        uint32_t argumentId = 0;
        for (auto it = nsd->captured.begin(); it != nsd->captured.end(); ++it) {
            currentSd->compiler->mov(ptr(stackPtr, argumentId * 8, 8), *(X86GpVar *) (closureVariables[argumentId]));
            ++argumentId;
        }


        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            X86Var *top = jStack.top();
            jStack.pop();
            switch (bf->parameterType(i)) {
                case VT_DOUBLE:
                    currentSd->compiler->movsd(ptr(stackPtr, argumentId * 8, 8), *(X86XmmVar *) (top));
                    break;
                case VT_INT:
                case VT_STRING:
                    currentSd->compiler->mov(ptr(stackPtr, argumentId * 8, 8), *(X86GpVar *) (top));
                    break;
                default:
                    assert(false);
            }
            ++argumentId;
        }

        if (isNativeCall) {
            void *exportedFunction = dlsym(RTLD_DEFAULT,
                                           bf->node()->body()->nodeAt(0)->asNativeCallNode()->nativeName().c_str());

            if (exportedFunction == nullptr) {
                throw new VmException("cannot find native call ", node->position());
            }

            if (nativeFunctions[function_id] == 0) {
                nativeFunctions[function_id] = buildNativeProxy(
                        bf->node()->body()->nodeAt(0)->asNativeCallNode()->nativeSignature(), exportedFunction);
            }

            requireSetXmmCnt = xmmCnt > 0;
            callNode = currentSd->compiler->call(imm_ptr(nativeFunctions[function_id]), prototype);
        } else {
            callNode = currentSd->compiler->call(jitedFunctions[function_id], prototype);
        }

        callNode->setArg(0, stackPtr);
    }

    void JVisitor::setupArgumentsForTraditionalCall(AstFunction *bf,
                                                    CallNode *node,
                                                    X86CallNode *&callNode,
                                                    FuncPrototype &prototype,
                                                    const NodeScopeData *nsd,
                                                    const std::vector<asmjit::X86Var *> &closureVariables,
                                                    const int64_t &function_id,
                                                    const int &xmmCnt,
                                                    bool &requireSetXmmCnt,
                                                    bool isNativeCall
    ) {
        if ((bf->node()->body()->nodes() > 0) && (bf->node()->body()->nodeAt(0)->isNativeCallNode())) {

            void *exportedFunction = dlsym(RTLD_DEFAULT,
                                           bf->node()->body()->nodeAt(0)->asNativeCallNode()->nativeName().c_str());

            if (exportedFunction == nullptr) {
                throw new VmException("cannot find native call ", node->position());
            }
            requireSetXmmCnt = xmmCnt > 0;
            callNode = currentSd->compiler->call(imm_ptr(exportedFunction), prototype);
        } else {
            callNode = currentSd->compiler->call(jitedFunctions[function_id], prototype);
        }

        uint32_t argumentId = 0;
        for (auto it = nsd->captured.begin(); it != nsd->captured.end(); ++it) {
            callNode->setArg(argumentId, *closureVariables[argumentId]);
            ++argumentId;
        }

        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            X86Var *top = jStack.top();
            jStack.pop();
            callNode->setArg(argumentId++, *top);
        }
    }

    inline void JVisitor::setXmmDataForLastCall(const int &xmmCnt) {
        HLNode *currentCursor = currentSd->compiler->getCursor();
        HLNode *endCursor = currentCursor;
        HLNode::Type callType = currentCursor->kTypeCall;
        while (currentCursor->getType() != callType) {
            currentCursor = currentCursor->getPrev();
        }
        currentCursor = currentCursor->getPrev();
        currentSd->compiler->setCursor(currentCursor);
        currentSd->compiler->emit(kX86InstIdMov, rax, xmmCnt);
        currentSd->compiler->setCursor(endCursor);
    }

    inline void
    JVisitor::saveCapturedVariables(NodeScopeData *nsd, const std::vector<asmjit::X86Mem *> &stackPointers) {
        uint64_t stackIndex = 0;
        for (auto it = nsd->captured.begin(); it != nsd->captured.end(); ++it) {
            AsmScopeVariable *variable = currentSd->lookupVariableInfo(it->name);

            if (!variable->isPointer) {
                switch (variable->type) {
                    case VT_DOUBLE:
                        currentSd->compiler->movsd(*(X86XmmVar *) (variable->var), *stackPointers[stackIndex++]);
                        break;
                    case VT_INT:
                    case VT_STRING:
                        currentSd->compiler->mov(*(X86GpVar *) (variable->var), *stackPointers[stackIndex++]);
                        break;
                    default:
                        assert(false);
                }
            }
        }
    }

    void JVisitor::processArithmeticOperation(BinaryOpNode *node) {
        node->left()->visit(this);
        VarType leftType = currentSd->topType;
        node->right()->visit(this);
        VarType rightType = currentSd->topType;


        if ((leftType != VT_INT) && (leftType != VT_DOUBLE)) {
            throw new std::logic_error("unexpected type in arithmetic operation");
        }
        if ((rightType != VT_INT) && (rightType != VT_DOUBLE)) {
            throw new std::logic_error("unexpected type in arithmetic operation");
        }
        VarType resultType = ((leftType == VT_DOUBLE) || (rightType == VT_DOUBLE)) ? VT_DOUBLE : VT_INT;

        if (rightType != resultType) {
            currentSd->topType = rightType;
            prepareTopType(resultType);
        }
        X86Var *rightVar = jStack.top();
        jStack.pop();

        if (leftType != resultType) {
            currentSd->topType = leftType;
            prepareTopType(resultType);
        }
        X86Var *leftVar = jStack.top();
        jStack.pop();
        X86Var *result;
        if (resultType == VT_DOUBLE) {
            result = new X86XmmVar();
            *result = currentSd->compiler->newXmm();
            currentSd->compiler->movq(*(X86XmmVar *) result, *(X86XmmVar *) leftVar);
            switch (node->kind()) {
                case tADD:
                    currentSd->compiler->addsd(*(X86XmmVar *) result, *(X86XmmVar *) rightVar);
                    break;
                case tSUB:
                    currentSd->compiler->subsd(*(X86XmmVar *) result, *(X86XmmVar *) rightVar);
                    break;
                case tMUL:
                    currentSd->compiler->mulsd(*(X86XmmVar *) result, *(X86XmmVar *) rightVar);
                    break;
                case tDIV:
                    currentSd->compiler->divsd(*(X86XmmVar *) result, *(X86XmmVar *) rightVar);
                    break;
                default:
                    throw new std::logic_error("unexpected type in arithmetic operation");
            }
        } else {
            result = new X86GpVar();
            *result = currentSd->compiler->newInt64();
            currentSd->compiler->mov(*(X86GpVar *) result, *(X86GpVar *) leftVar);
            X86GpVar *resultHigh = new X86GpVar();
            *resultHigh = currentSd->compiler->newInt64();
            X86CallNode *call;
            switch (node->kind()) {
                case tADD:
                    currentSd->compiler->add(*(X86GpVar *) result, *(X86GpVar *) rightVar);
                    break;
                case tSUB:
                    currentSd->compiler->sub(*(X86GpVar *) result, *(X86GpVar *) rightVar);
                    break;
                case tMUL:
                    currentSd->compiler->cqo(*resultHigh, *(X86GpVar *) rightVar);
                    currentSd->compiler->imul(*resultHigh, *(X86GpVar *) result, *(X86GpVar *) rightVar);
                    break;
                case tDIV:
                    call = currentSd->compiler->call(imm_ptr(idiv_div_int), signatureIII);
                    call->setArg(0, *(X86GpVar *) leftVar);
                    call->setArg(1, *(X86GpVar *) rightVar);
                    call->setRet(0, *(X86GpVar *) result);
                    break;
                case tMOD:
                    call = currentSd->compiler->call(imm_ptr(idiv_mod_int), signatureIII);
                    call->setArg(0, *(X86GpVar *) leftVar);
                    call->setArg(1, *(X86GpVar *) rightVar);
                    call->setRet(0, *(X86GpVar *) result);
                    break;
                default:
                    throw new std::logic_error("unexpected type in arithmetic operation");
            }
        }
        jStack.push(result);


        currentSd->topType = resultType;
    }

    void JVisitor::processLogicOperation(BinaryOpNode *node) {
        node->left()->visit(this);
        VarType leftType = currentSd->topType;
        if (leftType != VT_INT) {
            throw new std::logic_error("int expected for logic operator");
        }
        asmjit::Label endLabel = currentSd->compiler->newLabel();

        X86GpVar *result = new X86GpVar();
        *result = currentSd->compiler->newInt8();
        X86GpVar *extendedResult = new X86GpVar();
        *extendedResult = currentSd->compiler->newInt64();

        X86Var *leftVar = jStack.top();
        jStack.pop();
        X86Var *rightVar;
        switch (node->kind()) {
            case tOR:
                currentSd->compiler->mov(*result, 1);
                currentSd->compiler->cmp(*(X86GpVar *) leftVar, 0);
                currentSd->compiler->jne(endLabel);
                node->right()->visit(this);
                if (currentSd->topType != VT_INT) {
                    throw new std::logic_error("int expected for logic operator");
                }
                rightVar = jStack.top();
                jStack.pop();
                currentSd->compiler->cmp(*(X86GpVar *) rightVar, 0);
                currentSd->compiler->jne(endLabel);
                currentSd->compiler->mov(*result, 0);
                currentSd->compiler->bind(endLabel);
                currentSd->compiler->movzx(*extendedResult, *result);
                break;
            case tAND:
                currentSd->compiler->mov(*result, 0);
                currentSd->compiler->cmp(*(X86GpVar *) leftVar, 0);
                currentSd->compiler->je(endLabel);
                node->right()->visit(this);
                if (currentSd->topType != VT_INT) {
                    throw new std::logic_error("int expected for logic operator");
                }
                rightVar = jStack.top();
                jStack.pop();
                currentSd->compiler->cmp(*(X86GpVar *) rightVar, 0);
                currentSd->compiler->je(endLabel);
                currentSd->compiler->mov(*result, 1);
                currentSd->compiler->bind(endLabel);
                currentSd->compiler->movzx(*extendedResult, *result);
                break;
            default:
                throw new std::logic_error("unexpected operator");
        }
        asmjit::X86Var *pushed = extendedResult;
        jStack.push(pushed);
    }

    inline void JVisitor::processBitwiseOperation(BinaryOpNode *node) {
        node->left()->visit(this);
        VarType leftType = currentSd->topType;
        if (leftType != VT_INT) {
            throw new std::logic_error("int expected for bitwise operator");
        }
        node->right()->visit(this);
        VarType rightType = currentSd->topType;
        if (rightType != VT_INT) {
            throw new std::logic_error("int expected for bitwise operator");
        }
        X86Var *rightVar = jStack.top();
        jStack.pop();
        X86Var *leftVar = jStack.top();
        jStack.pop();

        X86GpVar *result = new X86GpVar();
        *result = currentSd->compiler->newInt64();
        currentSd->compiler->mov(*result, *(X86GpVar *) leftVar);

        switch (node->kind()) {
            case tAAND:
                currentSd->compiler->and_(*result, *(X86GpVar *) rightVar);
                break;
            case tAXOR:
                currentSd->compiler->xor_(*result, *(X86GpVar *) rightVar);
                break;
            case tAOR:
                currentSd->compiler->or_(*result, *(X86GpVar *) rightVar);
                break;
            default:
                throw new std::logic_error("unexpected type in arithmetic operation");
        }
        jStack.push(result);
        currentSd->topType = VT_INT;
    }

    inline void JVisitor::processComparisonOperation(BinaryOpNode *node) {
        node->left()->visit(this);
        VarType leftType = currentSd->topType;
        node->right()->visit(this);
        VarType rightType = currentSd->topType;

        bool isIntComparison = true;
        X86Var *leftVal;
        switch (leftType) {
            case VT_DOUBLE:
                isIntComparison = false;
                switch (rightType) {
                    case VT_DOUBLE:
                        break;
                    case VT_INT:
                        prepareTopType(VT_DOUBLE);
                        break;
                    default:
                        throw new std::logic_error("unexpected type in comparison operation");
                }
                break;
            case VT_INT:
                switch (rightType) {
                    case VT_DOUBLE:
                        isIntComparison = false;
                        leftVal = jStack.top();
                        jStack.pop();
                        currentSd->topType = VT_INT;
                        prepareTopType(VT_DOUBLE);
                        jStack.push(leftVal);
                        break;
                    case VT_INT:
                        break;
                    default:
                        throw new std::logic_error("unexpected type in comparison operation");
                }
                break;
            default:
                throw new std::logic_error("unexpected type in comparison operation");
        }

        if (isIntComparison) {
            processIntegerComparisonOperation(node);
        } else {
            processDoubleComparisonOperation(node);
        }
    }

    void JVisitor::processIntegerComparisonOperation(BinaryOpNode *node) {
        X86GpVar *rightVar = (X86GpVar *) jStack.top();
        jStack.pop();
        X86GpVar *leftVar = (X86GpVar *) jStack.top();
        jStack.pop();
        X86GpVar *result = new X86GpVar();
        *result = currentSd->compiler->newInt64();
        currentSd->compiler->cmp(*leftVar, *rightVar);
        switch (node->kind()) {
            case tEQ:
                currentSd->compiler->sete(result->r8());
                break;
            case tNEQ:
                currentSd->compiler->setne(result->r8());
                break;
            case tLT:
                currentSd->compiler->setl(result->r8());
                break;
            case tLE:
                currentSd->compiler->setle(result->r8());
                break;
            case tGT:
                currentSd->compiler->setg(result->r8());
                break;
            case tGE:
                currentSd->compiler->setge(result->r8());
                break;
            default:
                throw new std::logic_error("unexpected operator");
        }

        currentSd->compiler->movzx(*(X86GpVar *) result, (*(X86GpVar *) result).r8());
        currentSd->topType = VT_INT;
        jStack.push(result);
    }

    void JVisitor::processDoubleComparisonOperation(BinaryOpNode *node) {
        X86GpVar *result = new X86GpVar();
        *result = currentSd->compiler->newInt64();

        X86XmmVar *rightVar = (X86XmmVar *) jStack.top();
        jStack.pop();
        X86XmmVar *leftVar = (X86XmmVar *) jStack.top();
        jStack.pop();

        void *targetCall;
        switch (node->kind()) {
            case tEQ:
                targetCall = (void *) ucomisd_e;
                break;
            case tNEQ:
                targetCall = (void *) ucomisd_ne;
                break;
            case tLT:
                targetCall = (void *) ucomisd_l;
                break;
            case tLE:
                targetCall = (void *) ucomisd_le;
                break;
            case tGT:
                targetCall = (void *) ucomisd_g;
                break;
            case tGE:
                targetCall = (void *) ucomisd_ge;
                break;
            default:
                throw new std::logic_error("unexpected operator");
        }
        //currentSd->compiler->comment("add extern double comparison");
        X86CallNode *call = currentSd->compiler->call(imm_ptr(targetCall), signatureIDD);


        call->setArg(0, *leftVar);
        call->setArg(1, *rightVar);
        call->setRet(0, *result);

        currentSd->topType = VT_INT;
        jStack.push(result);
    }

    inline void JVisitor::bindAll() {
        topCompiler->bind(doubleMin);
        topCompiler->dint64(-0x8000000000000000LL);
        topCompiler->bind(doubleZero);
        topCompiler->dint64(00LL);
        /*
         * 	.long	0
         * 	.long	-2147483648
         *
         */
    }


    FunctionAnalysData::FunctionAnalysData(AstFunction *function, shared_ptr<JScopeData> scope) : function(function),
                                                                                                  scope(scope) {}
}

mathvm::JTranslator::~JTranslator() {}

mathvm::Status *mathvm::JTranslator::translate(const string &program, mathvm::Code **code) {
    if (code == nullptr) {
        return Status::Error("Code is nullptr");
    }

    Parser parserInstance = Parser();
    Status *parseStatus = parserInstance.parseProgram(program);
    if (parseStatus->isError()) {
        return parseStatus;
    }
    delete parseStatus;

    JVariableAnnotator *annotator = new JVariableAnnotator();
    Status *resultAnnotator = annotator->runTranslate(*code, parserInstance.top());
    if (!resultAnnotator->isOk()) {
        delete annotator;
        return resultAnnotator;
    }

    JVisitor *visitor = new JVisitor();
    Status *result = visitor->runTranslate(*code, parserInstance.top());

    delete visitor;
    delete annotator;
    return result;
}
