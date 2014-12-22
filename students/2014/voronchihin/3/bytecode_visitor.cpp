#include <functional>
#include "bytecode_visitor.h"
#include "mathvm.h"
#include <iostream>

using namespace mathvm;

void ByteCodeVisitor::visitForNode(mathvm::ForNode *node) {
    Label expr(bytecode());
    Label outLabel(bytecode());

    BinaryOpNode *binaryOpNode = node->inExpr()->asBinaryOpNode();
    binaryOpNode->left()->visit(this);
    store(node->var());
    bytecode()->bind(expr);
    load(node->var());
    binaryOpNode->right()->visit(this);
    branch(BC_IFICMPL, outLabel);
    popStack();
    popStack();
    node->body()->visit(this);
    load(node->var());
    insn(BC_ILOAD1);
    insn(BC_IADD);
    store(node->var());
    branch(BC_JA, expr);
    bytecode()->bind(outLabel);
}

void ByteCodeVisitor::visitPrintNode(mathvm::PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        printTop();
    }
}

void ByteCodeVisitor::visitLoadNode(mathvm::LoadNode *node) {
    load(node->var());
}

void ByteCodeVisitor::visitIfNode(mathvm::IfNode *node) {
    node->ifExpr()->visit(this);
    Label outLabel(bytecode());
    insn(BC_ILOAD0);
    if (node->elseBlock()) {
        Label elseLabel = Label(bytecode());
        branch(BC_IFICMPE, elseLabel);
        node->thenBlock()->visit(this);
        branch(BC_JA, outLabel);
        bytecode()->bind(elseLabel);
        node->elseBlock()->visit(this);
    } else {
        branch(BC_IFICMPE, outLabel);
        node->thenBlock()->visit(this);
    }
    bytecode()->bind(outLabel);
}

void ByteCodeVisitor::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
    bytecode()->add(BC_ILOAD);
    bytecode()->addTyped(node->literal());
    pushStack(VT_INT);
}

void ByteCodeVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode *node) {
    bytecode()->add(BC_DLOAD);
    bytecode()->addTyped(node->literal());
    pushStack(VT_DOUBLE);
}

void ByteCodeVisitor::visitStringLiteralNode(mathvm::StringLiteralNode *node) {
    bytecode()->add(BC_SLOAD);
    bytecode()->addTyped(interpreterCode->makeStringConstant(node->literal()));
    pushStack(VT_STRING);
}

void ByteCodeVisitor::visitWhileNode(mathvm::WhileNode *node) {
    Label exprBegin(bytecode());
    bytecode()->bind(exprBegin);
    node->whileExpr()->visit(this);
    Label outLabel(bytecode());
    insn(BC_ILOAD0);
    branch(BC_IFICMPE, outLabel);
    node->loopBlock()->visit(this);
    branch(BC_JA, exprBegin);
    bytecode()->bind(outLabel);
}

void ByteCodeVisitor::visitBlockNode(mathvm::BlockNode *node) {
    currScope = node->scope();
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        AstNode *n = node->nodeAt(i);
        n->visit(this);
    }
    Scope::FunctionIterator functionIterator(currScope);
    while (functionIterator.hasNext()) {
        AstFunction *astFunction = functionIterator.next();
        visitFunctionNode(astFunction->node());
    }
    currScope = currScope->parent();
}

void ByteCodeVisitor::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
    node->right()->visit(this);
    node->left()->visit(this);
    binaryOp(node->kind());
}

void ByteCodeVisitor::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
    node->visitChildren(this);
    unaryOp(node->kind());
}

void ByteCodeVisitor::visitNativeCallNode(mathvm::NativeCallNode *node) {
    AstVisitor::visitNativeCallNode(node);
}

void ByteCodeVisitor::visitFunctionNode(mathvm::FunctionNode *node) {
    BytecodeFunction *dump = currentBytecodeFunction;
    if (currScope) {
        AstFunction *function = currScope->lookupFunction(node->name());
        if (!function) {
            error("undeclared function %s", node->name().c_str());
        }
        AstFunctionInfo *astFunctionInfo = (AstFunctionInfo *) function->info();
        currentBytecodeFunction = astFunctionInfo->function;
    } else {
        currentBytecodeFunction = dynamic_cast<BytecodeFunction *>(interpreterCode->functionById((uint16_t) 0));
    }
    std::vector<VarType> newStack;
    currStack = &newStack;

    Scope::VarIterator varIterator(node->body()->scope()->parent());
    while (varIterator.hasNext()) {
        AstVar *var = varIterator.next();
        pushStack(var->type());//ensure that variables on stack is ok
        store(var);
    }

    visitBlockNode(node->body());

    currentBytecodeFunction = dump;
    currScope = currScope->parent();//jump parameters scope
}

void ByteCodeVisitor::visitReturnNode(mathvm::ReturnNode *node) {
    node->visitChildren(this);
    VarType varType = currentBytecodeFunction->returnType();
    if (varType != VT_VOID) {
        cast(topStack(), varType);
    }
    insn(BC_RETURN);
}

void ByteCodeVisitor::visitStoreNode(mathvm::StoreNode *node) {
    const AstVar *var = node->var();
    node->value()->visit(this);
    store(var, node->op());
}

void ByteCodeVisitor::visitCallNode(mathvm::CallNode *node) {
    unsigned long stackSize = currStack->size();
    AstFunction *astFunction = currScope->lookupFunction(node->name(), true);

    AstFunctionInfo *functionInfo = (AstFunctionInfo *) astFunction->info();
    Scope *parameterScope = astFunction->scope();

    if (node->parametersNumber() != parameterScope->variablesCount()) {
        error("parameters number mistmach in calling %s", node->name().c_str());
    }
    vector<VarType> declaredParameters;
    declaredParameters.reserve(parameterScope->variablesCount());
    Scope::VarIterator varIterator(parameterScope);
    while (varIterator.hasNext()) {
        declaredParameters.push_back(varIterator.next()->type());
    }

    vector<VarType>::reverse_iterator it = declaredParameters.rbegin();

    for (uint32_t i = node->parametersNumber(); i > 0; --i, ++it) {
        AstNode *n = node->parameterAt(i - 1);
        n->visit(this);
        cast(topStack(), (*it));
    }

    insn(BC_CALL);
    typed(functionInfo->function->id());
    if (astFunction->returnType() != VT_VOID) {
        pushStack(astFunction->returnType());
    }
}


void ByteCodeVisitor::initVars(Scope *scope) {
    if (!scope) return;
    Scope::VarIterator varIt(scope);
    while (varIt.hasNext()) {
        AstVar *var = varIt.next();
        if (!var->info()) {
            var->setInfo(new AstVarInfo(currentBytecodeFunction->id(), currentBytecodeFunction->localsNumber()));
            currentBytecodeFunction->setLocalsNumber(currentBytecodeFunction->localsNumber() + 1);
            if (currentBytecodeFunction->localsNumber() >= UINT16_MAX) {
                error("vars overflow in function <%s,id>", currentBytecodeFunction->name().c_str(), currentBytecodeFunction->id());
            }
        } else {
            AstVarInfo *varInfo = (AstVarInfo *) var->info();
            TranslatedFunction *function = interpreterCode->functionById(varInfo->contextId);
            error("variable already declared var <%s,%d,%d> for function %s",
                    var->name().c_str(), varInfo->contextId, varInfo->id, function->name().c_str());
        }
    }

}

void ByteCodeVisitor::initChildFunctions(Scope *scope) {
    Scope::FunctionIterator functionIterator(scope);
    while (functionIterator.hasNext()) {
        initFunctionsAndParameters(functionIterator.next());
    }
}


void ByteCodeVisitor::initLocals(BlockNode *blockNode) {
    if (blockNode == 0) return;
    initVars(blockNode->scope());
    for (uint32_t i = 0; i < blockNode->nodes(); ++i) {
        AstNode *n = blockNode->nodeAt(i);
        if (n->isBlockNode()) {
            initLocals(blockNode);
        } else if (n->isIfNode()) {
            IfNode *ifNode = n->asIfNode();
            initLocals(ifNode->thenBlock());
            initLocals(ifNode->elseBlock());
        } else if (n->isWhileNode()) {
            WhileNode *whileNode = n->asWhileNode();
            initLocals(whileNode->loopBlock());
        } else if (n->isForNode()) {
            ForNode *forNode = n->asForNode();
            initLocals(forNode->body());
        }
    }
    initChildFunctions(blockNode->scope());
}

void ByteCodeVisitor::initFunctionsAndParameters(AstFunction *function) {
    BytecodeFunction *dumpByteCodeFunction = currentBytecodeFunction;

    BytecodeFunction *bytecodeFunction = new BytecodeFunction(function);
    currentBytecodeFunction = bytecodeFunction;
    interpreterCode->addFunction(bytecodeFunction);
    function->setInfo(new AstFunctionInfo(currentBytecodeFunction));
    initVars(function->scope());
    initLocals(function->node()->body());

    currentBytecodeFunction = dumpByteCodeFunction;
}
