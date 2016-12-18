#include "JVariableAnnotator.h"
#include "StackItem.h"
#include "InterScope.h"
#include <cstring>
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../libs/asmjit/asmjit.h"
#include "VmException.h"
#include <set>
#include <unordered_map>


std::unordered_map<mathvm::AstFunction *, size_t> scopedDataNodes;
std::vector<mathvm::NodeScopeData *> functionIdToNodeScope;


void mathvm::JVariableAnnotator::visitForNode(mathvm::ForNode *node) {
    BinaryOpNode *range = node->inExpr()->asBinaryOpNode();
    range->left()->visit(this);
    range->right()->visit(this);
    loadVariable(node->var());
    storeVariable(node->var());
    visitBlockNode(node->body());
}

void mathvm::JVariableAnnotator::visitPrintNode(mathvm::PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
    }
}

void mathvm::JVariableAnnotator::visitLoadNode(mathvm::LoadNode *node) {
    loadVariable(node->var());
}

void mathvm::JVariableAnnotator::visitIfNode(mathvm::IfNode *node) {
    node->ifExpr()->visit(this);
    bool elseExists = node->elseBlock() != nullptr;
    visitBlockNode(node->thenBlock());
    if (elseExists) {
        visitBlockNode(node->elseBlock());
    }
}

void mathvm::JVariableAnnotator::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
    node->left()->visit(this);
    node->right()->visit(this);
}

void mathvm::JVariableAnnotator::visitDoubleLiteralNode(mathvm::DoubleLiteralNode *node) {
}

void mathvm::JVariableAnnotator::visitStoreNode(mathvm::StoreNode *node) {
    node->value()->visit(this);
    switch (node->op()) {
        case tINCRSET:
        case tDECRSET:
            loadVariable(node->var());
            break;
        case tASSIGN:
            break;
        default:
            throw new std::logic_error("unexpected save action");
    }
    storeVariable(node->var());
}

void mathvm::JVariableAnnotator::visitStringLiteralNode(mathvm::StringLiteralNode *node) {
}

void mathvm::JVariableAnnotator::visitWhileNode(mathvm::WhileNode *node) {
    node->whileExpr()->visit(this);
    visitBlockNode(node->loopBlock());
}

void mathvm::JVariableAnnotator::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
}

void mathvm::JVariableAnnotator::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
    node->operand()->visit(this);
}

void mathvm::JVariableAnnotator::visitNativeCallNode(mathvm::NativeCallNode *node) {
}

void mathvm::JVariableAnnotator::visitBlockNode(mathvm::BlockNode *node) {
    AScopeData *old_sd = currentSd;
    currentSd = (AScopeData *) node->info();
    if (currentSd == nullptr) {
        currentSd = new AScopeData(old_sd);
    }
    scopeEvaluator(node->scope());
    for (uint32_t i = 0; i < node->nodes(); i++) {
        node->nodeAt(i)->visit(this);
    }
    node->setInfo(currentSd);
    currentSd = old_sd;
}

void mathvm::JVariableAnnotator::visitFunctionNode(mathvm::FunctionNode *node) {
    node->body()->visit(this);
}

void mathvm::JVariableAnnotator::visitReturnNode(mathvm::ReturnNode *node) {
    if (node->returnExpr() != nullptr) {
        node->returnExpr()->visit(this);
    }
}

void mathvm::JVariableAnnotator::visitCallNode(mathvm::CallNode *node) {
    for (uint32_t i = node->parametersNumber(); i >= 1; i--) {
        node->parameterAt(i - 1)->visit(this);
    }
    AstFunction *targetFunction = currentSd->lookupFunctionByName(node->name(), true);

    const unsigned long targetFunctionId = scopedDataNodes[targetFunction];
    NodeScopeData *dataNode = functionIdToNodeScope[targetFunctionId];
    for (auto it = dataNode->captured.begin(); it != dataNode->captured.end(); ++it) {
        currentSd->registerVariableCapture(*it);
    }
    functionIdToNodeScope[currentSd->current_function_id]->hasInternalCalls = true;
}

inline void mathvm::JVariableAnnotator::loadVariable(const mathvm::AstVar *var) {
    currentSd->registerVariableLoad(ManagedVariable(var->name(), var->type()));
}

inline void mathvm::JVariableAnnotator::storeVariable(const mathvm::AstVar *var) {
    currentSd->registerVariableStore(ManagedVariable(var->name(), var->type()));
}

void mathvm::JVariableAnnotator::scopeEvaluator(mathvm::Scope *scope) {
    Scope::VarIterator it = Scope::VarIterator(scope);
    while (it.hasNext()) {
        AstVar *next = it.next();
        currentSd->addVariable(next);
    }
    Scope::FunctionIterator itFun = Scope::FunctionIterator(scope);
    while (itFun.hasNext()) {
        AstFunction *next = itFun.next();
        AstFunction *currentByteFunction = currentSd->lookupFunctionByName(next->name(), false);
        if (currentByteFunction == nullptr) {
            currentSd->addFunction(next);
        }
    }
    //only after index all names
    itFun = Scope::FunctionIterator(scope);
    while (itFun.hasNext()) {
        AstFunction *next = itFun.next();
        translateFunction(next);
    }
}


mathvm::Status *mathvm::JVariableAnnotator::runTranslate(mathvm::Code *code, mathvm::AstFunction *function) {
    try {
        currentSd = new AScopeData(function);
        currentSd->addFunction(function);
        currentSd->current_function_id = scopedDataNodes[function];

        translateFunction(function);

        delete currentSd;
    }
    catch (mathvm::VmException *ex) {
        return Status::Error(ex->what(), ex->getPosition());
    }
    catch (std::exception *ex) {
        return Status::Error(ex->what());
    }
    return Status::Ok();
}

void mathvm::JVariableAnnotator::translateFunction(mathvm::AstFunction *function) {
    AScopeData *old_sd = currentSd;
    currentSd = (AScopeData *) function->info();
    if (currentSd == nullptr) {
        currentSd = new AScopeData(function, old_sd);
    }

    currentSd->current_function_id = functionIdToNodeScope.size();
    scopedDataNodes[function] = functionIdToNodeScope.size();
    functionIdToNodeScope.push_back(currentSd->currentNodeScope);


    for (uint32_t currentParameter = 0; currentParameter < function->parametersNumber(); ++currentParameter) {
        AstVar *variable = function->scope()->lookupVariable(function->parameterName(currentParameter), false);
        currentSd->addVariable(variable);
        storeVariable(variable);
    }


    size_t delta = 0;
    do {
        const size_t beforeSize = total_captured_count;
        function->node()->visit(this);
        delta = total_captured_count - beforeSize;
    } while (delta > 0);


    currentSd->saveFunctionInfo();

    function->setInfo(currentSd);
    currentSd = old_sd;
}

