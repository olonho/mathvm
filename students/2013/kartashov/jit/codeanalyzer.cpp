#include "codeanalyzer.h"
#include <algorithm>

using namespace mathvm;

void CodeAnalyzer::analyze(AstFunction *top) {
    extUsageChanged = true;
    recursiveBinaryOp = false;
    inStoreNode = false;
    visitedFuncs.clear();
    funcDescrs.clear();
    funcIds.clear();
    varUsage.clear();

    Scope *scope = new Scope(0);
    scope->declareFunction(top->node());
    currentFuncID = 0;

    //we'll make full ast traverse in enterScope
    enterScope(scope);
    exitScope(scope);

    while(extUsageChanged) {
        visitedFuncs.clear();
        extUsageChanged = false;
        findExtUsages(funcDescrs[0]);
    }

    for(std::vector<FunctionDescription>::iterator it = funcDescrs.begin(); it != funcDescrs.end(); ++it) {
        it->generateMetadata();
    }
}

//------------------------------------------------------------

void CodeAnalyzer::visitFunctionNode(FunctionNode *node) {
    uint16_t extFuncID = currentFuncID;
    currentFuncID = funcIds[node->name()];
    for(uint32_t i = 0; i < node->parametersNumber(); ++i) varUsage.pushData(node->parameterName(i), currentFuncID);
    node->body()->visit(this);
    for(uint32_t i = 0; i < node->parametersNumber(); ++i) varUsage.popData(node->parameterName(i));
    currentFuncID = extFuncID;
}

void CodeAnalyzer::visitBlockNode(BlockNode *node) {
    enterScope(node->scope());
    node->visitChildren(this);
    exitScope(node->scope());
}

void CodeAnalyzer::visitCallNode(CallNode *node) {
    node->visitChildren(this);
    funcDescrs[currentFuncID].calls.push_back(funcIds[node->name()]);
}

void CodeAnalyzer::visitForNode(ForNode *node) {
    node->visitChildren(this);
    useVar(node->var());
}

void CodeAnalyzer::visitStoreNode(StoreNode *node) {
    inStoreNode = node->op() == tASSIGN;
    node->visitChildren(this);
    inStoreNode = false;
    useVar(node->var());
}

void CodeAnalyzer::visitLoadNode(LoadNode *node) {
    node->visitChildren(this);
    useVar(node->var());
}

//---------------------------------------------------------------------------------------------

void CodeAnalyzer::enterScope(Scope *scope) {
    Scope::VarIterator vit(scope);
    while(vit.hasNext()) {
        AstVar *var = vit.next();
        varUsage.pushData(var->name(), currentFuncID);
        funcDescrs[currentFuncID].locals.push_back(var->name());
        funcDescrs[currentFuncID].localTypes.push_back(var->type());
    }

    Scope::FunctionIterator fit(scope);
    while(fit.hasNext()) {
        AstFunction *fn = fit.next();
        FunctionDescription fdescr(funcDescrs.size(), fn->name());
        for(uint32_t i = 0; i < fn->parametersNumber(); ++i) {
            fdescr.params.push_back(fn->parameterName(i));
        }
        funcDescrs.push_back(fdescr);
        funcIds[fn->name()] = fdescr.id;
    }

    fit = Scope::FunctionIterator(scope);
    while(fit.hasNext()) fit.next()->node()->visit(this);
}

void CodeAnalyzer::exitScope(Scope *scope) {
    Scope::VarIterator vit(scope);
    while(vit.hasNext()) varUsage.popData(vit.next()->name());
}

//---------------------------------------------------------------------------------------------

bool CodeAnalyzer::hasVar(const VarUsageList &list, const std::string &name, uint16_t userId) const {
    return hasVar(list, VarUsage(name, userId));
}

bool CodeAnalyzer::hasVar(const std::vector<std::string> &list, const std::string &name) const {
    return std::find(list.begin(), list.end(), name) != list.end();
}

bool CodeAnalyzer::hasVar(const VarUsageList &list, const VarUsage &var) const {
    return std::find(list.begin(), list.end(), var) != list.end();
}

bool CodeAnalyzer::isLocalVar(const FunctionDescription &fd, const VarUsage &var) const {
    return (hasVar(fd.locals, var.var) && fd.id == var.funcId) || (hasVar(fd.params, var.var) && fd.id == var.funcId);
}

bool CodeAnalyzer::isLocalVar(const FunctionDescription &fd, const string &var) const {
    return hasVar(fd.locals, var) || hasVar(fd.params, var);
}

void CodeAnalyzer::useVar(const AstVar *v) {
    const std::string &var = v->name();
    uint16_t sourceId = varUsage.topData(var);
    FunctionDescription &userFunc = funcDescrs[currentFuncID];
    if(currentFuncID == sourceId) return;
    if(hasVar(userFunc.useVars, var, sourceId)) return;
    userFunc.useVars.push_back(VarUsage(var, sourceId));

    if(isLocalVar(userFunc, var)) throw std::logic_error("Found binding of local var " + var + " as external");
}

//---------------------------------------------------------------------------------------------

// find external vars for function 'descr' that used in child calls
void CodeAnalyzer::findExtUsages(FunctionDescription &descr) {
    visitedFuncs.insert(descr.id);

    for(VarUsageList::iterator it = descr.useVars.begin(); it != descr.useVars.end(); ++it) {
        if(!hasVar(descr.closureVars, *it)) {
            descr.closureVars.push_back(*it);
            extUsageChanged = true;
        }
    }

    for(std::vector<uint16_t>::iterator it = descr.calls.begin(); it != descr.calls.end(); ++it) {
        FunctionDescription &fd = funcDescrs[*it];
        if(visitedFuncs.find(*it) == visitedFuncs.end()) findExtUsages(fd);

        for(VarUsageList::iterator ueit = fd.closureVars.begin(); ueit != fd.closureVars.end(); ++ueit) {
            if(!hasVar(descr.closureVars, *ueit) && !isLocalVar(descr, *ueit)) {
                descr.closureVars.push_back(*ueit);
                extUsageChanged = true;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------

bool CodeAnalyzer::canReuseVars(BinaryOpNode *node) const {
    if(!inStoreNode) return false;
    if(node->kind() != tADD && node->kind() != tMUL) return false;
    if(node->left()->isIntLiteralNode() && node->right()->isLoadNode()) return true;
    else if(node->right()->isIntLiteralNode() && node->left()->isLoadNode()) return true;
    return false;
}

void CodeAnalyzer::visitBinaryOpNode(BinaryOpNode *node) {
    if(recursiveBinaryOp) {
        node->visitChildren(this);
    } else if(node->left()->isBinaryOpNode() || node->left()->isUnaryOpNode()
            || node->right()->isBinaryOpNode() || node->right()->isUnaryOpNode()) {
        recursiveBinaryOp = true;
        node->visitChildren(this);
        recursiveBinaryOp = false;
    } else {
        //one level binary op
        if(canReuseVars(node)) {
            funcDescrs[currentFuncID].canReuseMathVars = true;
            funcDescrs[currentFuncID].nodesWithReuse.insert(node);
        }
        node->visitChildren(this);
    }
}

void CodeAnalyzer::visitNativeCallNode(NativeCallNode *node) {
}

void CodeAnalyzer::visitUnaryOpNode(UnaryOpNode *node) {
    node->visitChildren(this);
}

void CodeAnalyzer::visitIfNode(IfNode *node) {
    node->visitChildren(this);
}

void CodeAnalyzer::visitWhileNode(WhileNode *node) {
    node->visitChildren(this);
}

void CodeAnalyzer::visitReturnNode(ReturnNode *node) {
    node->visitChildren(this);
}

void CodeAnalyzer::visitIntLiteralNode(IntLiteralNode *node) {
    node->visitChildren(this);
}

void CodeAnalyzer::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    node->visitChildren(this);
}

void CodeAnalyzer::visitStringLiteralNode(StringLiteralNode *node) {
    node->visitChildren(this);
}

void CodeAnalyzer::visitPrintNode(PrintNode *node) {
    FunctionDescription &fd = funcDescrs[currentFuncID];
    if(!fd.hasIntPrintNode) {
        for(uint32_t i = 0; i < node->operands(); ++i) {
            AstNode *nnode = node->operandAt(i);
            if(nnode->isLoadNode()) {
                switch(nnode->asLoadNode()->var()->type()) {
                case VT_INT: fd.hasIntPrintNode = true; break;
                default: break;
                }
            } else if(nnode->isStringLiteralNode() || nnode->isIntLiteralNode()) {
                fd.hasIntPrintNode = true;
            }
        }
    }
    node->visitChildren(this);
}

//==================================================================================

void FunctionDescription::generateMetadata() {
     uint32_t varCounter = 0;

     for(std::vector<std::string>::const_iterator it = params.begin(); it != params.end(); ++it, ++varCounter) {
         varAddresses[VarUsage(*it, id)] = varCounter;
     }

     for(VarUsageList::const_iterator it = closureVars.begin(); it != closureVars.end(); ++it, ++varCounter) {
         varAddresses[VarUsage(it->var, it->funcId)] = varCounter;
     }

     for(std::vector<std::string>::const_iterator it = locals.begin(); it != locals.end(); ++it, ++varCounter) {
         varAddresses[VarUsage(*it, id)] = varCounter;
     }

     varsToStore = varCounter;
}

uint32_t FunctionDescription::getAddress(const std::string &var) const {
    VarAddressMap::const_iterator it = varAddresses.find(VarUsage(var, id));
    if(it == varAddresses.end()) {
        for(VarUsageList::const_iterator uit = useVars.begin(); uit != useVars.end(); ++uit) {
            if(uit->var == var) {
                it = varAddresses.find(VarUsage(uit->var, uit->funcId));
                if(it != varAddresses.end()) break;
            }
        }
        for(VarUsageList::const_iterator uit = closureVars.begin(); uit != closureVars.end(); ++uit) {
            if(uit->var == var) {
                it = varAddresses.find(VarUsage(uit->var, uit->funcId));
                if(it != varAddresses.end()) break;
            }
        }
    }
    if(it == varAddresses.end()) throw std::logic_error("Unable to get address of variable: " + var);
    return it->second;
}

uint32_t FunctionDescription::getAddress(const std::string &var, uint16_t ownerId) const {
    VarAddressMap::const_iterator it = varAddresses.find(VarUsage(var, ownerId));
    if (it == varAddresses.end()) throw std::logic_error("Unable to get address of variable: " + var);
    return it->second;
}
