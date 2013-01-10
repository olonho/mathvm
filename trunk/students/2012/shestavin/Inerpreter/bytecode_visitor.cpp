#include "bytecode_visitor.h"

#include "mathvm.h"

namespace mathvm {

    BytecodeVisitor::BytecodeVisitor(AstFunction* top, Code* code): top(top), code(code), currentScope(0){
}
    
void BytecodeVisitor::visit() {
    processFunctionsDeclarations(top->scope());
}
    
void BytecodeVisitor::processForvardFunctionsDeclarations(Scope* scope) {
    Scope::FunctionIterator functionIterator(scope);
    while (functionIterator.hasNext()) {
        AstFunction* function = functionIterator.next();
        BytecodeFunction* bytecodeFunction = new BytecodeFunction(function);
        code->addFunction(bytecodeFunction);
    }
}

void BytecodeVisitor::processFunctionsDeclarations(Scope* scope) {
    processForvardFunctionsDeclarations(scope);
    Scope::FunctionIterator functionIterator(scope);
    while (functionIterator.hasNext()) {
        AstFunction* function = functionIterator.next();
        if (function->node()->body()->nodeAt(0)->isNativeCallNode()) {
            function->node()->visit(this);
        } else {
            /*
            if (function->name() != "<top>") {
                vector<const AstVar*> prevLocals = locals;
                locals.clear();
                findLocals(function->node());
                
                for (vector<const AstVar*>::const_iterator i = locals.begin(); i != locals.end(); ++i) {
                    size_t varsMapSize = varsMap.size();
                    getVarID(*i);
                    if (varsMapSize < varsMap.size()) {
                        initVar(*i);
                    }
                }
                locals = prevLocals;
            }
             */

            BytecodeFunction* bytecodeFunction = dynamic_cast<BytecodeFunction*>(code->functionByName(function->node()->name()));
            functionsStack.push(std::make_pair(bytecodeFunction, function));
            function->node()->visit(this);
            functionsStack.pop();
        }
    }
}

Bytecode* BytecodeVisitor::getActualBytecode() {
    return functionsStack.top().first->bytecode();
}

uint16_t BytecodeVisitor::getVarID(const AstVar* var) {
        if (varsMap.find(var) == varsMap.end()) {
            uint16_t id = (uint16_t) varsMap.size();
            varsMap[var] = id;
        }
        return varsMap[var];
}

void BytecodeVisitor::processVariablesDeclarations(Scope* scope) {
    Scope::VarIterator varIterator(scope);
    while (varIterator.hasNext()) {
        AstVar* var = varIterator.next();
        getVarID(var);
        locals.push_back(var);
        blockLocals.push_back(var);
    }
}
    
void BytecodeVisitor::loadVar(const AstVar* var) {
    //if (varsMap.find(var) == varsMap.end()) {
    //    initVar(var);
    //}
    switch (var->type()) {
        case VT_INT: {
            getActualBytecode()->addInsn(BC_LOADIVAR);
            TOSType = VT_INT;
            break;
        }
        case VT_DOUBLE: {
            getActualBytecode()->addInsn(BC_LOADDVAR);
            TOSType = VT_DOUBLE;
            break;
        }
        case VT_STRING: {
            getActualBytecode()->addInsn(BC_LOADSVAR);
            TOSType = VT_STRING;
            break;
        }
        default: {
            std::cerr << "Type error!\n";
            return;
        }
    }
    getActualBytecode()->addUInt16(getVarID(var));
}
    
void BytecodeVisitor::initVar(const AstVar* var) {
    switch (var->type()) {
        case VT_INT: {
            getActualBytecode()->addInsn(BC_ILOAD0);
            getActualBytecode()->addInsn(BC_STOREIVAR);
            break;
        }
        case VT_DOUBLE: {
            getActualBytecode()->addInsn(BC_DLOAD0);
            getActualBytecode()->addInsn(BC_STOREDVAR);
            break;
        }
        case VT_STRING: {
            getActualBytecode()->addInsn(BC_SLOAD0);
            getActualBytecode()->addInsn(BC_STORESVAR);
            break;
        }
        default: {
            std::cerr << "Type error!\n";
            return;
        }
    }
    getActualBytecode()->addUInt16(getVarID(var));
}

void BytecodeVisitor::storeVar(const AstVar* var) {
    uint16_t varId = getVarID(var);
    VarType varType = var->type();
    if (varType == VT_INT) {
        getActualBytecode()->addInsn(BC_STOREIVAR);
    } else if (varType == VT_DOUBLE) {
        getActualBytecode()->addInsn(BC_STOREDVAR);
    } else if (varType == VT_STRING) {
        getActualBytecode()->addInsn(BC_STORESVAR);
    } else {
        std::cerr << "Function VOID param!\n";
    }
    getActualBytecode()->addUInt16(varId);
}
    
    
    
void BytecodeVisitor::pushVars(const vector<const AstVar*>& vars) {
    for (vector<const AstVar*>::const_iterator i = vars.begin(); i != vars.end(); ++i) {
        loadVar(*i);
    }
}
    
void BytecodeVisitor::initVars(const vector<const AstVar*>& vars) {
    for (vector<const AstVar*>::const_iterator i = vars.begin(); i != vars.end(); ++i) {
        initVar(*i);
    }
}
    
    
void BytecodeVisitor::popVars(const vector<const AstVar*>& vars, bool swap) {
    for (vector<const AstVar*>::const_reverse_iterator i = vars.rbegin(); i != vars.rend(); ++i) {
        if (swap) {
            getActualBytecode()->addInsn(BC_SWAP);
        }
        storeVar(*i);
    }
}

void BytecodeVisitor::visitBlockNode(BlockNode* node) {
    Scope* prevScope = currentScope;
    currentScope = node->scope();
    
    vector<const AstVar*> prevBlockLocals = blockLocals;
    blockLocals.clear();

    processVariablesDeclarations(node->scope());
    processFunctionsDeclarations(node->scope());
    
    pushVars(blockLocals);
    initVars(blockLocals);

    for(size_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
        if (TOSType != VT_VOID && !(node->nodeAt(i)->isReturnNode())) {
            getActualBytecode() -> addInsn(BC_POP);
        }
    }
        popVars(blockLocals, false);

    
    locals.erase(locals.end() - blockLocals.size(), locals.end());
    
    blockLocals = prevBlockLocals;
    currentScope = prevScope;
    TOSType = VT_VOID;

}

void BytecodeVisitor::castToIntOnly(VarType type1, VarType type2) {
    if ((type1 == VT_INT && type2 == VT_INT)) {
        return;
    } else {
        if (type1 == VT_STRING) {
            getActualBytecode()->addInsn(BC_S2I);
        }
        if (type1 == VT_DOUBLE) {
            getActualBytecode()->addInsn(BC_D2I);
        }
        if (type2 != VT_INT) {
            getActualBytecode()->addInsn(BC_SWAP);
            if (type2 == VT_STRING) {
                getActualBytecode()->addInsn(BC_S2I);
            }
            if (type2 == VT_DOUBLE) {
                getActualBytecode()->addInsn(BC_D2I);
            }
            getActualBytecode()->addInsn(BC_SWAP);
        }
    }
}

VarType BytecodeVisitor::castToSameType(VarType type1, VarType type2) {
    if ((type1 == VT_INT && type2 == VT_INT) || (type1 == VT_DOUBLE && type2 == VT_DOUBLE)) {
        return type1;
    }
    if ((type1 == VT_DOUBLE || type2 == VT_DOUBLE)) {
        VarType toCast = type1;
        if (type1 == VT_DOUBLE) {
            getActualBytecode()->addInsn(BC_SWAP);
            toCast = type2;
        }
        if (toCast == VT_STRING) {
            getActualBytecode()->addInsn(BC_S2I);
        } 
        getActualBytecode()->addInsn(BC_I2D);
        if (type1 == VT_DOUBLE) {
            getActualBytecode()->addInsn(BC_SWAP);
        }
        return VT_DOUBLE;
    } else {
        castToIntOnly(type1, type2);
        return VT_INT;
    }
}

void BytecodeVisitor::compareInts(Instruction instruction) {
    getActualBytecode()->addInsn(instruction);
    getActualBytecode()->addInt16(9);
    getActualBytecode()->addInsn(BC_POP);
    getActualBytecode()->addInsn(BC_POP);
    getActualBytecode()->addInsn(BC_ILOAD0);
    getActualBytecode()->addInsn(BC_JA);
    getActualBytecode()->addInt16(6);
    getActualBytecode()->addInsn(BC_POP);
    getActualBytecode()->addInsn(BC_POP);
    getActualBytecode() ->addInsn(BC_ILOAD1);
}

void BytecodeVisitor::compareDoubles(Instruction instruction) {
    getActualBytecode() -> addInsn(BC_DCMP);
    getActualBytecode() -> addInsn(BC_ILOAD0);
    compareInts(instruction);
}

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    node->right()->visit(this);
    VarType type1 = TOSType;
    node->left()->visit(this);
    VarType type2 = TOSType;
    if (!type1 || !type2) {
        std::cerr << "Type error!\n";
        return;
    }
    switch(node->kind()) {
        case tADD: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                getActualBytecode()->addInsn(BC_DADD);
                TOSType = VT_DOUBLE;
            } else if (opType == VT_INT) {
                getActualBytecode()->addInsn(BC_IADD);
                TOSType = VT_INT;
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            break;
        }
        case tSUB: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                getActualBytecode()->addInsn(BC_DSUB);
                TOSType = VT_DOUBLE;
            } else if (opType == VT_INT) {
                getActualBytecode()->addInsn(BC_ISUB);
                TOSType = VT_INT;
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            break;
        }
        case tMUL: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                getActualBytecode()->addInsn(BC_DMUL);
                TOSType = VT_DOUBLE;
            } else if (opType == VT_INT) {
                getActualBytecode()->addInsn(BC_IMUL);
                TOSType = VT_INT;
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            break;
        }
        case tDIV: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                getActualBytecode()->addInsn(BC_DDIV);
                TOSType = VT_DOUBLE;
            } else if (opType == VT_INT) {
                getActualBytecode()->addInsn(BC_IDIV);
                TOSType = VT_INT;
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            break;
        }
        case tMOD: {
            castToIntOnly(type2, type1);
            getActualBytecode()->addInsn(BC_IMOD);
            TOSType = VT_INT;
            break;
        }
        case tEQ: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                compareDoubles(BC_IFICMPE);
            } else if (opType == VT_INT) {
                compareInts(BC_IFICMPE);
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            TOSType = VT_INT;
            break;
        }
        case tNEQ: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                compareDoubles(BC_IFICMPNE);
            } else if (opType == VT_INT) {
                compareInts(BC_IFICMPNE);
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            TOSType = VT_INT;
            break;
        }
        case tGT: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                compareDoubles(BC_IFICMPG);
            } else if (opType == VT_INT) {
                compareInts(BC_IFICMPG);
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            TOSType = VT_INT;
            break;
        }
        case tGE: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                compareDoubles(BC_IFICMPGE);
            } else if (opType == VT_INT) {
                compareInts(BC_IFICMPGE);
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            TOSType = VT_INT;
            break;
        }
        case tLT: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                compareDoubles(BC_IFICMPL);
            } else if (opType == VT_INT) {
                compareInts(BC_IFICMPL);
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            TOSType = VT_INT;
            break;
        }
        case tLE: {
            VarType opType = castToSameType(type2, type1);
            if (opType == VT_DOUBLE) {
                compareDoubles(BC_IFICMPLE);
            } else if (opType == VT_INT) {
                compareInts(BC_IFICMPLE);
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            TOSType = VT_INT;
            break;
        }
        case tAND: {
            castToIntOnly(type2, type1);

            getActualBytecode()->addInsn(BC_ILOAD0);
            getActualBytecode()->addInsn(BC_IFICMPE);
            getActualBytecode()->addInt16(14);
        
            getActualBytecode()->addInsn(BC_SWAP);
            getActualBytecode()->addInsn(BC_POP);
            getActualBytecode()->addInsn(BC_IFICMPE);
            getActualBytecode()->addInt16(10);
        
            getActualBytecode()->addInsn(BC_POP);
            getActualBytecode()->addInsn(BC_POP);
            getActualBytecode()->addInsn(BC_ILOAD1);
            getActualBytecode()->addInsn(BC_JA);
            getActualBytecode()->addInt16(7);
        
            getActualBytecode()->addInsn(BC_POP);

            getActualBytecode()->addInsn(BC_POP);
            getActualBytecode()->addInsn(BC_POP);
            getActualBytecode()->addInsn(BC_ILOAD0);

            TOSType = VT_INT;
            break;
        }
        case tOR: {
            castToIntOnly(type2, type1);

            getActualBytecode()->addInsn(BC_ILOAD0);
            getActualBytecode()->addInsn(BC_IFICMPNE);
            getActualBytecode()->addInt16(13);
        
            getActualBytecode()->addInsn(BC_SWAP);
            getActualBytecode()->addInsn(BC_POP);
            getActualBytecode()->addInsn(BC_IFICMPNE);
            getActualBytecode()->addInt16(9);
        
            getActualBytecode()->addInsn(BC_SWAP);
            getActualBytecode()->addInsn(BC_POP);
            getActualBytecode()->addInsn(BC_JA);
            getActualBytecode()->addInt16(7);
        
            getActualBytecode()->addInsn(BC_POP);

            getActualBytecode()->addInsn(BC_POP);
            getActualBytecode()->addInsn(BC_POP);
            getActualBytecode()->addInsn(BC_ILOAD1);

            TOSType = VT_INT;
            break;
        }
        default: {
            std::cerr << "Binary operator error!\n";
            return;
        }
    }
}

void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->operand()->visit(this);
    switch(node->kind()) {
        case tNOT: {
            if (TOSType == VT_INT) {
                getActualBytecode()->addInsn(BC_ILOAD0);
                getActualBytecode()->addInsn(BC_IFICMPE);
                getActualBytecode()->addInt16(8);
                getActualBytecode()->addInsn(BC_SWAP);
                getActualBytecode()->addInsn(BC_POP);
                getActualBytecode()->addInsn(BC_JA);
                getActualBytecode()->addInt16(5);
                getActualBytecode()->addInsn(BC_POP);
                getActualBytecode()->addInsn(BC_POP);
                getActualBytecode()->addInsn(BC_ILOAD1);
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            break;
        } case tSUB: {
            if (TOSType == VT_DOUBLE) {
                getActualBytecode()->addInsn(BC_DNEG);
            } else if (TOSType == VT_INT) {
                getActualBytecode()->addInsn(BC_INEG);
            } else {
                std::cerr << "Type error!\n";
                return;
            }
            break;
        }
        default: {
            std::cerr << "Unary operator error!\n";
            return;
        }
    }
}

void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    getActualBytecode()->addInsn(BC_SLOAD);
    getActualBytecode()->addInt16(code->makeStringConstant(node->literal()));
    TOSType = VT_STRING;
}

void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    getActualBytecode()->addInsn(BC_DLOAD);
    getActualBytecode()->addDouble(node->literal());
    TOSType = VT_DOUBLE;
}

void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    getActualBytecode()->addInsn(BC_ILOAD);
    getActualBytecode()->addInt64(node->literal());
    TOSType = VT_INT;
}
    

void BytecodeVisitor::visitLoadNode(LoadNode *node) {
    loadVar(node->var());
}

void BytecodeVisitor::visitStoreNode(StoreNode *node) {
    uint16_t varId = getVarID(node->var());
    VarType varType = node->var()->type();
    node->value()->visit(this);

    if (varType == VT_INT) {
        if (TOSType == VT_DOUBLE) {
            getActualBytecode()->addInsn(BC_D2I);
        } else if (TOSType == VT_STRING) {
            getActualBytecode()->addInsn(BC_S2I);
        } else if (TOSType != VT_INT) {
            std::cerr << "Type error!\n";
            return;
        }
    } else if (varType == VT_DOUBLE) {
        if (TOSType == VT_INT) {
            getActualBytecode()->addInsn(BC_I2D);
        } else if (TOSType == VT_STRING) {
            getActualBytecode()->addInsn(BC_S2I);
            getActualBytecode()->addInsn(BC_I2D);
        } else if (TOSType != VT_DOUBLE) {
            std::cerr << "Type error!\n";
            return;
        }
    }
    switch (node->op()) {
        case tASSIGN: {
            if (varType == VT_STRING) {
                if (TOSType != VT_STRING) {
                    std::cerr << "Type error!\n";
                    return;
                } else {
                    getActualBytecode()->addInsn(BC_STORESVAR);
                    getActualBytecode()->addUInt16(varId);
                }
            } else if (varType == VT_INT) {
                getActualBytecode()->addInsn(BC_STOREIVAR);
                getActualBytecode()->addUInt16(varId);
            } else if (varType == VT_DOUBLE) {
                getActualBytecode()->addInsn(BC_STOREDVAR);
                getActualBytecode()->addUInt16(varId);
            } else {
                std::cerr << "Assign error!\n";
            }
            break;
        }
        case tINCRSET: {
            if (varType == VT_INT) {
                getActualBytecode()->addInsn(BC_LOADIVAR);
                getActualBytecode()->addUInt16(varId);
                getActualBytecode()->addInsn(BC_IADD);
                getActualBytecode()->addInsn(BC_STOREIVAR);
                getActualBytecode()->addUInt16(varId);
            } else if (varType == VT_DOUBLE){
                getActualBytecode()->addInsn(BC_LOADDVAR);
                getActualBytecode()->addUInt16(varId);
                getActualBytecode()->addInsn(BC_DADD);
                getActualBytecode()->addInsn(BC_STOREDVAR);
                getActualBytecode()->addUInt16(varId);
            } else {
                std::cerr << "Incrset error!\n";
            }
            break;
        }
        case tDECRSET: {
            if (varType == VT_INT) {
                getActualBytecode()->addInsn(BC_LOADIVAR);
                getActualBytecode()->addUInt16(varId);
                getActualBytecode()->addInsn(BC_ISUB);
                getActualBytecode()->addInsn(BC_STOREIVAR);
                getActualBytecode()->addUInt16(varId);
            } else if (varType == VT_DOUBLE){
                getActualBytecode()->addInsn(BC_LOADDVAR);
                getActualBytecode()->addUInt16(varId);
                getActualBytecode()->addInsn(BC_DSUB);
                getActualBytecode()->addInsn(BC_STOREDVAR);
                getActualBytecode()->addUInt16(varId);
            } else {
                std::cerr << "Incrset error!\n";
            }
            break;
        }
        default: {
            std::cerr << "Store error!\n";
            return;
        }
    }
    TOSType = VT_VOID;
}

void BytecodeVisitor::visitForNode(ForNode *node) {
    uint16_t indexVarId = getVarID(node->var());

    BinaryOpNode *range = (BinaryOpNode*) node->inExpr();
    range->right()->visit(this);
    if (TOSType != VT_INT) {
        std::cerr << "For index type error!\n";
    }
    range->left()->visit(this);
    if (TOSType != VT_INT) {
        std::cerr << "For index type error!\n";
    }
    getActualBytecode()->addInsn(BC_IFICMPG);
    uint32_t beginAddr = getActualBytecode()->length();
    uint32_t offsetAddr = getActualBytecode()->length();
    getActualBytecode()->addInt16(0);
    uint32_t jumpFromAddr = getActualBytecode()->length();
    
    getActualBytecode()->addInsn(BC_STOREIVAR);
    getActualBytecode()->addUInt16(indexVarId);
    getActualBytecode()->addInsn(BC_LOADIVAR);
    getActualBytecode()->addUInt16(indexVarId);

    node->body()->visit(this);

    getActualBytecode()->addInsn(BC_ILOAD1);
    getActualBytecode()->addInsn(BC_IADD);
    
    getActualBytecode()->addInsn(BC_JA);
    getActualBytecode()->addInt16(beginAddr - getActualBytecode()->length());

    uint32_t endAddr = getActualBytecode()->length();
    getActualBytecode()->setInt16(offsetAddr, endAddr - jumpFromAddr + 3);

    getActualBytecode()->addInsn(BC_POP);
    getActualBytecode()->addInsn(BC_POP);
    TOSType = VT_VOID;
}

void BytecodeVisitor::visitWhileNode(WhileNode* node) { 
    uint32_t beginAddr = getActualBytecode()->length();
    node -> whileExpr()->visit(this);
    getActualBytecode()->addInsn(BC_ILOAD0);
    getActualBytecode()->addInsn(BC_IFICMPE);
    uint32_t offsetAddr = getActualBytecode()->length();
    getActualBytecode()->addInt16(0);
    uint32_t jumpFromAddr = getActualBytecode()->length();

    getActualBytecode()->addInsn(BC_POP);
    getActualBytecode()->addInsn(BC_POP);

    node->loopBlock()->visit(this);
    getActualBytecode()->addInsn(BC_JA);
    getActualBytecode()->addInt16(beginAddr - getActualBytecode()->length() + 1);

    uint32_t endAddr = getActualBytecode()->length();
    getActualBytecode()->setInt16(offsetAddr, endAddr - jumpFromAddr + 3);

    getActualBytecode()->addInsn(BC_POP);
    getActualBytecode()->addInsn(BC_POP);
    TOSType = VT_VOID;
}

void BytecodeVisitor::visitIfNode(IfNode* node) {
    node->ifExpr()->visit(this);
    getActualBytecode()->addInsn(BC_ILOAD0);
    getActualBytecode()->addInsn(BC_IFICMPNE);
    uint32_t thenOffsetAddr = getActualBytecode()->current();
    getActualBytecode()->addInt16(0);
    getActualBytecode()->addInsn(BC_JA);
    uint32_t elseOffsetAddr = getActualBytecode()->current();
    getActualBytecode()->addInt16(0);
    getActualBytecode()->setInt16(thenOffsetAddr, getActualBytecode()->current() - thenOffsetAddr + 1);
    getActualBytecode()->addInsn(BC_POP);
    getActualBytecode()->addInsn(BC_POP);
    node->thenBlock()->visit(this);
    getActualBytecode()->addInsn(BC_JA);
    uint32_t exitOffsetAddr = getActualBytecode()->current();
    getActualBytecode()->addInt16(0);
    getActualBytecode()->setInt16(elseOffsetAddr, getActualBytecode()->current() - elseOffsetAddr + 1);
    getActualBytecode()->addInsn(BC_POP);
    getActualBytecode()->addInsn(BC_POP);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }
    getActualBytecode()->setInt16(exitOffsetAddr, getActualBytecode()->current() - exitOffsetAddr + 1);
    TOSType = VT_VOID;
}
    
void BytecodeVisitor::findLocals(const FunctionNode* func) {
    Scope* scope = func->body()->scope();
    Scope::VarIterator varIterator(scope);
    while (varIterator.hasNext()) {
        AstVar* var = varIterator.next();
        //getVarID(var);
        locals.push_back(var);
    }
}

void BytecodeVisitor::visitFunctionNode(FunctionNode *node) {
    VarType prevReturnType = returnType;
    returnType = node -> returnType();
    vector<const AstVar*> prevLocals = locals;
    vector<const AstVar*> prevBlockLocals = blockLocals;
    locals.clear();
    blockLocals.clear();
    
    if (node->parametersNumber() > 0) {
        for (signed int i = node->parametersNumber() - 1; i >= 0; --i) {
            AstVar* variable = node->body()->scope()->lookupVariable(node->parameterName(i));
            storeVar(variable);
        }

    }
    
    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
    } else {
        node->body()->visit(this);
    }
    TOSType = node->returnType();
    returnType = prevReturnType;
    locals = prevLocals;
    blockLocals = prevBlockLocals;
}

void BytecodeVisitor::visitReturnNode(ReturnNode* node) {
    if (node->returnExpr() != 0) {
        node->returnExpr()->visit(this);
        if (TOSType == VT_INT && returnType == VT_DOUBLE) {
            getActualBytecode()->addInsn(BC_I2D);
        } else if (TOSType == VT_DOUBLE && returnType == VT_INT) {
            getActualBytecode()->addInsn(BC_D2I);
        } else if (TOSType == VT_STRING && returnType == VT_DOUBLE) {
            getActualBytecode()->addInsn(BC_S2I);
            getActualBytecode()->addInsn(BC_I2D);
        } else if (TOSType == VT_STRING && returnType == VT_INT) {
            getActualBytecode()->addInsn(BC_S2I);
        } else if (TOSType != returnType){
            std::cerr << "Function return type error!\n";
        }
    }
    
    if (functionsStack.top().second->returnType() != VT_VOID) {
        popVars(locals, true);
    } else {
        popVars(locals, false);
    }
    getActualBytecode()->addInsn(BC_RETURN);
    TOSType = VT_VOID;
}

void BytecodeVisitor::visitCallNode( CallNode* node ) {
    const Signature& signature = code->functionByName(node->name())->signature();
    
    AstFunction* functionToCall = currentScope->lookupFunction(node->name());
    vector<const AstVar*> params;
    if (node->parametersNumber() > 0) {
        for (signed int i = node->parametersNumber() - 1; i >= 0; --i) {
            AstVar* variable = functionToCall->scope()->lookupVariable(signature[i + 1].second);
            params.push_back(variable);
        }
    }
    
    pushVars(params);
    
    for (unsigned int i = 0; i < node->parametersNumber(); i++) {
        node->parameterAt(i)->visit(this);
        
        if (TOSType == VT_INT && signature[i + 1].first == VT_DOUBLE) {
            getActualBytecode()->addInsn(BC_I2D);
        } else if (TOSType == VT_DOUBLE && signature[i + 1].first == VT_INT) {
            getActualBytecode()->addInsn(BC_D2I);
        } else if (TOSType == VT_STRING && signature[i + 1].first == VT_DOUBLE) {
            getActualBytecode()->addInsn(BC_S2I);
            getActualBytecode()->addInsn(BC_I2D);
        } else if (TOSType == VT_STRING && signature[i + 1].first == VT_INT) {
            getActualBytecode()->addInsn(BC_S2I);
        } else if (TOSType != signature[i + 1].first){
            std::cerr << "Function call type error!\n";
        }
    }
    
    
    getActualBytecode()->addInsn(BC_CALL);
    getActualBytecode()->addInt16(code->functionByName(node->name())->id());

    if (signature[0].first != VT_VOID) {
        popVars(params, true);
    } else {
        popVars(params, false);
    }
    TOSType = signature[0].first;
}

void BytecodeVisitor::visitNativeCallNode(NativeCallNode* node) {

}

void BytecodeVisitor::visitPrintNode(PrintNode* node) {
    for (size_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        switch (TOSType) {
            case VT_INT: {
                getActualBytecode()->addInsn(BC_IPRINT);
                break;
            }
            case VT_DOUBLE: {
                getActualBytecode()->addInsn(BC_DPRINT);
                break;
            }
            case VT_STRING: {
                getActualBytecode()->addInsn(BC_SPRINT);
                break;
            }
            default: {
                std::cerr << "Can't print properly!\n";
            }
        }
    }
    TOSType = VT_VOID;
}

}