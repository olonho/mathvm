#include "visitors.h"
#include "validating_visitor.h"
#include "memory.h"
#include "my_util.h"
#include "bytecode_generator.h"

namespace mathvm {
const std::string BytecodeGenerator::tmpVarName = "$tmp";

BytecodeGenerator::BytecodeGenerator() {
    initTables();
}

Status* BytecodeGenerator::generateCode(AstFunction* top, Code* code) {
    status = Status::Ok();
    this->code = code;

    // declate tmp var as top function local variable
    top->node()->body()->scope()->declareVariable(tmpVarName, VT_INT);

    startScope(top->owner());

    return status;
}


void BytecodeGenerator::initTables() {
    memset(arithmeticCommands, 0, sizeof(arithmeticCommands));
    arithmeticCommands[tAOR][VT_INT] = BC_IAOR;
    arithmeticCommands[tAAND][VT_INT] = BC_IAAND;
    arithmeticCommands[tAXOR][VT_INT] = BC_IAXOR;
    arithmeticCommands[tADD][VT_INT] = BC_IADD;
    arithmeticCommands[tADD][VT_DOUBLE] = BC_DADD;
    arithmeticCommands[tSUB][VT_INT] = BC_ISUB;
    arithmeticCommands[tSUB][VT_DOUBLE] = BC_DSUB;
    arithmeticCommands[tMUL][VT_INT] = BC_IMUL;
    arithmeticCommands[tMUL][VT_DOUBLE] = BC_DMUL;
    arithmeticCommands[tDIV][VT_INT] = BC_IDIV;
    arithmeticCommands[tDIV][VT_DOUBLE] = BC_DDIV;
    arithmeticCommands[tMOD][VT_INT] = BC_IMOD;

    memset(cmpCommands, 0, sizeof(cmpCommands));
    cmpCommands[tEQ] = BC_IFICMPE;
    cmpCommands[tNEQ] = BC_IFICMPNE;
    cmpCommands[tGT] = BC_IFICMPG;
    cmpCommands[tGE] = BC_IFICMPGE;
    cmpCommands[tLT] = BC_IFICMPL;
    cmpCommands[tLE] = BC_IFICMPLE;

    memset(convertCommands, 0, sizeof(convertCommands));
    convertCommands[VT_INT][VT_DOUBLE] = BC_I2D;
    convertCommands[VT_DOUBLE][VT_INT] = BC_D2I;

    memset(storeCommands, 0, sizeof(storeCommands));
    storeCommands[0][VT_INT] = BC_STOREIVAR;
    storeCommands[0][VT_DOUBLE] = BC_STOREDVAR;
    storeCommands[0][VT_STRING] = BC_STORESVAR;
    storeCommands[1][VT_INT] = BC_STORECTXIVAR;
    storeCommands[1][VT_DOUBLE] = BC_STORECTXDVAR;
    storeCommands[1][VT_STRING] = BC_STORECTXSVAR;

    memset(loadCommands, 0, sizeof(loadCommands));
    loadCommands[0][VT_INT] = BC_LOADIVAR;
    loadCommands[0][VT_DOUBLE] = BC_LOADDVAR;
    loadCommands[0][VT_STRING] = BC_LOADSVAR;
    loadCommands[1][VT_INT] = BC_LOADCTXIVAR;
    loadCommands[1][VT_DOUBLE] = BC_LOADCTXDVAR;
    loadCommands[1][VT_STRING] = BC_LOADCTXSVAR;
}

void BytecodeGenerator::generateDuplicateIntTOS() {
    AstVar* tmpVar = scope->lookupVariable(tmpVarName);
    
    storeVar(tmpVar);
    loadVar(tmpVar);
    loadVar(tmpVar);
}


void BytecodeGenerator::visitBooleanBinOpNode(BinaryOpNode* node) {
    Instruction jmpCmd = node->kind() == tOR ? BC_IFICMPNE : BC_IFICMPE;
    Label end(bytecode);

    node->left()->visit(this);
    generateDuplicateIntTOS();
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(jmpCmd, end); 
    bytecode->addInsn(BC_POP);
    node->right()->visit(this);
    bytecode->bind(end);
}

void BytecodeGenerator::visitArithmeticBinOpNode(BinaryOpNode* node) {
    VarType type = getType(node);

    node->right()->visit(this);
    convertIfNecessary(getType(node->right()), type);
    node->left()->visit(this);
    convertIfNecessary(getType(node->left()), type);

    Instruction cmd = arithmeticCommands[node->kind()][type];
    assert(cmd != BC_INVALID);
    bytecode->addInsn(cmd);
}

void BytecodeGenerator::visitCmpBinOpNode(BinaryOpNode* node) {
    VarType type = getWidestType(getType(node->left()), getType(node->right())); 
    
    if(type == VT_DOUBLE) {
        bytecode->addInsn(BC_ILOAD0);
    }

    node->right()->visit(this);
    convertIfNecessary(getType(node->right()), type);
    node->left()->visit(this);
    convertIfNecessary(getType(node->left()), type);

    Label go(bytecode), end(bytecode);
    
    if(type == VT_DOUBLE) {
        bytecode->addInsn(BC_DCMP);
    }
    
    Instruction jmpCmd = cmpCommands[node->kind()];
    assert(jmpCmd != BC_INVALID);
     
    bytecode->addBranch(jmpCmd, go); 
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_JA, end);
    bytecode->bind(go);
    bytecode->addInsn(BC_ILOAD1);
    bytecode->bind(end);
}

void BytecodeGenerator::visitRangeBinOpNode(BinaryOpNode* node) {
    node->right()->visit(this);
    node->left()->visit(this);
}

void BytecodeGenerator::convertIfNecessary(VarType have, VarType expect) {
    if(have != expect) {
        assert(convertCommands[have][expect] != BC_INVALID);
        bytecode->addInsn(convertCommands[have][expect]);
    }
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode* node) {
    node->visitChildren(this);

    if(node->kind() == tSUB) {
        VarType type = getType(node->operand());
        if(type == VT_INT) {
            bytecode->addInsn(BC_INEG);
        }
        else {
            bytecode->addInsn(BC_DNEG);
        }
        return;
    }

    if(node->kind() == tNOT) {
        Label go(bytecode), end(bytecode);

        /*      expr
         *      load 0
         *      if expr == 0 jmp _go_
         *      load 0
         *      jmp _end_
         * go:  load 1
         * end:
         */

        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_IFICMPE, go);
        bytecode->addInsn(BC_ILOAD0);
        bytecode->addBranch(BC_JA, end);
        bytecode->bind(go);
        bytecode->addInsn(BC_ILOAD1);
        bytecode->bind(end);
    }
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode* node) {
    uint16_t id = code->makeStringConstant(node->literal());
    bytecode->addInsn(BC_SLOAD);
    bytecode->addUInt16(id);
}
void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    bytecode->addInsn(BC_DLOAD);
    bytecode->addDouble(node->literal());
}
void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode* node) {
    bytecode->addInsn(BC_ILOAD);
    bytecode->addInt64(node->literal()); 
}

void BytecodeGenerator::loadVar(AstVar const * var) {
    VarLocation &loc = vars[var]; 

    if(loc.funId == currentFunction->id()) {
        bytecode->addInsn(loadCommands[0][var->type()]);
        bytecode->addUInt16(loc.idx);
    }
    else {
        bytecode->addInsn(loadCommands[1][var->type()]);
        bytecode->addUInt16(loc.funId);
        bytecode->addUInt16(loc.idx);
    }
}

void BytecodeGenerator::visitLoadNode(LoadNode* node) {
    loadVar(node->var());
}

void BytecodeGenerator::visitChangeStoreNode(StoreNode* node) {
    loadVar(node->var());
    
    TokenKind binOp = node->op() ==  tINCRSET ? tADD : tSUB;
    Instruction cmd = arithmeticCommands[binOp][node->var()->type()];
    bytecode->addInsn(cmd);
}

void BytecodeGenerator::storeVar(AstVar const * var) {
    VarLocation &loc = vars[var];
    
    if(loc.funId == currentFunction->id()) {
        bytecode->addInsn(storeCommands[0][var->type()]);
        bytecode->addUInt16(loc.idx);
    }
    else {
        bytecode->addInsn(storeCommands[1][var->type()]);
        bytecode->addUInt16(loc.funId);
        bytecode->addUInt16(loc.idx);
    }
}

void BytecodeGenerator::visitStoreNode(StoreNode* node) {
    node->visitChildren(this);
    convertIfNecessary(getType(node->value()), node->var()->type());

    if(node->op() != tASSIGN) {
        visitChangeStoreNode(node);
    }

    storeVar(node->var());
}

void BytecodeGenerator::visitForNode(ForNode* node) {
    Label start(bytecode), end(bytecode);

    node->inExpr()->visit(this);
    
    /*
     *        end
     *        start
     *        store i
     *        duplicate
     *        load i
     *        if i > end jmp _end_
     * start: body
     *        duplicate
     *        load i
     *        if i == end jmp _end_
     *        load i
     *        load 1
     *        add
     *        store i
     *        jmp _start_
     * end:   pop
     */

    storeVar(node->var());
    generateDuplicateIntTOS();
    loadVar(node->var());
    bytecode->addBranch(BC_IFICMPG, end);
    bytecode->bind(start);
    node->body()->visit(this);
    generateDuplicateIntTOS();
    loadVar(node->var());
    bytecode->addBranch(BC_IFICMPE, end);
    loadVar(node->var());
    bytecode->addInsn(BC_ILOAD1);
    bytecode->addInsn(BC_IADD);
    storeVar(node->var());
    bytecode->addBranch(BC_JA, start);
    bytecode->bind(end);
    bytecode->addInsn(BC_POP);
}

void BytecodeGenerator::visitWhileNode(WhileNode* node) {
    Label start(bytecode), end(bytecode);

    /*
     * start: expr
     *        load 0
     *        if expr == 0 jmp _end_
     *        body
     *        jmp _start_
     * end:
     */

    bytecode->bind(start);
    node->whileExpr()->visit(this);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, end);
    node->loopBlock()->visit(this);
    bytecode->addBranch(BC_JA, start);
    bytecode->bind(end);
}

void BytecodeGenerator::visitIfNode(IfNode* node) {
    Label go(bytecode), end(bytecode);

    /*      expr
     *      load 0
     *      if expr == 0 jmp _go_
     *      thenBlock
     *      jmp _end_
     * go:  elseBlock
     * end:
     */

    node->ifExpr()->visit(this);
    
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, go);

    node->thenBlock()->visit(this);
    if(node->elseBlock()) {
        bytecode->addBranch(BC_JA, end);
        bytecode->bind(go);
        node->elseBlock()->visit(this);
        bytecode->bind(end);
    }
    else {
        bytecode->bind(go);
    }
}

int BytecodeGenerator::valsLeftOnStack(AstNode* node) {
    if(node->isCallNode()) {
        string const & funName = node->asCallNode()->name();
        AstFunction* astFunction = scope->lookupFunction(funName);

        return astFunction->returnType() == VT_VOID ? 0 : 1;
    }

    if(node->isBinaryOpNode()) {
        BinaryOpNode* binNode = node->asBinaryOpNode();
        return binNode->kind() == tRANGE ? 2 : 1;
    } 

    if(node->isStringLiteralNode() ||
       node->isDoubleLiteralNode() ||
       node->isIntLiteralNode()    ||
       node->isLoadNode()          ||
       node->isUnaryOpNode()) {
        return 1;
    }
    else {
        return 0;
    }
}

void BytecodeGenerator::visitBlockNode(BlockNode* node) {
    Scope* old = startScope(node->scope());
    
    for (uint32_t i = 0; i < node->nodes(); i++) {
        AstNode* cur = node->nodeAt(i);

        cur->visit(this);

        int unusedValsCnt = valsLeftOnStack(cur);
        for(int i = 0; i < unusedValsCnt; ++i) {
            bytecode->addInsn(BC_POP);
        }
    }

    endScope(node->scope(), old);
}

void BytecodeGenerator::visitFunctionNode(FunctionNode* node) {
    // declare arguments
    for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
        AstVar* var = scope->lookupVariable(node->parameterName(i));
        declareVar(var);
    } 

    node->body()->visit(this);

    //undeclare arguments
    for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
        AstVar* var = scope->lookupVariable(node->parameterName(i));
        undeclareVar(var);
    }
}

void BytecodeGenerator::visitReturnNode(ReturnNode* node) {
    node->visitChildren(this);
    
    if(node->returnExpr()) {
        convertIfNecessary(getType(node->returnExpr()), currentFunction->returnType());
    }

    bytecode->addInsn(BC_RETURN);
}

void BytecodeGenerator::visitCallNode(CallNode* node) {
    AstFunction* astFunction = scope->lookupFunction(node->name());

    for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        convertIfNecessary(getType(node->parameterAt(i)), astFunction->parameterType(i));
    }

    if(isFunctionNative(astFunction)) {
        astFunction->node()->body()->nodeAt(0)->visit(this); 
    }
    else {
        BytecodeFunction* function = (BytecodeFunction*) astFunction->info(); 
        bytecode->addInsn(BC_CALL);
        bytecode->addUInt16(function->id());
    }
}


void BytecodeGenerator::visitNativeCallNode(NativeCallNode* node) {
    uint16_t id = code->makeNativeFunction(node->nativeName(), node->nativeSignature(), 0);
    bytecode->addInsn(BC_CALLNATIVE);
    bytecode->addUInt16(id);
}

void BytecodeGenerator::visitPrintNode(PrintNode* node) {
    if(node->operands()  == 0) {
        return;
    }
    
    // translate operands in reverse order
    uint32_t i = node->operands();
    do {
        --i;
        node->operandAt(i)->visit(this);
    } while(i != 0);
    
    
    Instruction commands[5] = {BC_INVALID};
    commands[VT_INT] = BC_IPRINT;
    commands[VT_DOUBLE] = BC_DPRINT;
    commands[VT_STRING] = BC_SPRINT;

    for(i = 0; i < node->operands(); ++i) {
        VarType type = getType(node->operandAt(i));
        Instruction insn = commands[type];

        assert(insn != BC_INVALID);
        bytecode->addInsn(insn);
    }
}

Scope* BytecodeGenerator::startScope(Scope* scope) {
    Scope* old = this->scope;
    this->scope = scope;

    Scope::VarIterator varIt(scope);
    while(varIt.hasNext()) {
        declareVar(varIt.next());
    }
    
    Scope::FunctionIterator funIt(scope);
    while(funIt.hasNext()) {
        AstFunction* astFun = funIt.next();
        if(!isFunctionNative(astFun)) {
            BytecodeFunction* bytecodeFun = new BytecodeFunction(astFun);
            code->addFunction(bytecodeFun);
            astFun->setInfo(bytecodeFun);
        }
        else {
            NativeCallNode* node = astFun->node()->body()->nodeAt(0)->asNativeCallNode();
            void* nativeCode = locateNativeFunction(node->nativeName());
            if(nativeCode == 0) {
                fail("Can't load native function " + node->nativeName(), node->position());
            }

            code->makeNativeFunction(node->nativeName(), node->nativeSignature(), nativeCode);
        }
    }

    funIt = Scope::FunctionIterator(scope);
    while(funIt.hasNext()) {
        AstFunction* astFun = funIt.next();
        if(!isFunctionNative(astFun)) {
            visitAstFunction(astFun);
        }
    }
    
    return old;
}

void BytecodeGenerator::endScope(Scope* scope, Scope* old) {
    Scope::VarIterator varIt(scope);
    while(varIt.hasNext()) {
        undeclareVar(varIt.next());
    }

    this->scope = old;
}

void BytecodeGenerator::visitAstFunction(AstFunction* function) {
    BytecodeFunction* oldFunction = currentFunction;
    Bytecode* oldBytecode = bytecode;
    uint16_t oldLocals = locals;
    Scope* oldScope = scope;

    BytecodeFunction* newFunction = (BytecodeFunction*) function->info();
    
    currentFunction = newFunction;
    bytecode = newFunction->bytecode();
    locals = 0; 
    scope = function->scope(); 

    function->node()->visit(this);

    currentFunction->setLocalsNumber(currentFunction->localsNumber() - 
            currentFunction->parametersNumber());
    
    currentFunction = oldFunction;
    bytecode = oldBytecode;
    locals = oldLocals;
    scope = oldScope;
}

void BytecodeGenerator::declareVar(AstVar* var) {
    if(locals == (1<<16) - 1) {
        fail("Too many local variable defined in function " + 
                currentFunction->name(), 0);
        return;
    }

    vars[var] = VarLocation(currentFunction->id(), locals);
    locals++;
    
    if(locals > currentFunction->localsNumber()) {
        currentFunction->setLocalsNumber(locals);
    }
}

void BytecodeGenerator::undeclareVar(AstVar* var) {
    vars.erase(var); 
    locals--;
}

void BytecodeGenerator::fail(string const & msg, uint32_t position) {
    if(status->isOk()) {
        delete status;
        status = Status::Error(msg.c_str(), position);
    }
}

VarType BytecodeGenerator::getType(AstNode* node) {
    NodeData* data = (NodeData*) node->info();
    return data->type;
}

}
