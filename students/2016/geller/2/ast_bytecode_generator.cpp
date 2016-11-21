//
// Created by wimag on 20.11.16.
//

#include "ast_bytecode_generator.h"
using namespace mathvm;

Status *ast_bytecode_generator::execute() {
    auto root_function = new BytecodeFunction(root);
    code->addFunction(root_function);
    visitAstFunction(root);
    root_function->bytecode()->addInsn(BC_RETURN);
    return Status::Ok();
}

ast_bytecode_generator::ast_bytecode_generator(AstFunction *root, Code *code) : code(code) {
    assert(root->top_name == "<top>");
    this->root = root;
}

void ast_bytecode_generator::visitFunctionNode(FunctionNode *node) {
    node->name();
    if(node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()){
        assert(false); //TODO - native functions
    }else{
        node->body()->visit(this);
    }
}

void ast_bytecode_generator::visitBlockNode(BlockNode *node) {
    Scope::VarIterator varIterator(node->scope());
    while(varIterator.hasNext()){
        AstVar* var(varIterator.next());
        storage.register_variable(var);
    }

    Scope::FunctionIterator functionIterator(node->scope());
    while(functionIterator.hasNext()){
        AstFunction* func = functionIterator.next();
        if(code->functionByName(func->name()) == nullptr){
            code->addFunction(new BytecodeFunction(func));
        }
    }

    Scope::FunctionIterator functionVisitIterator(node->scope());
    while(functionVisitIterator.hasNext()){
        visitAstFunction(functionVisitIterator.next());
    }
    node->visitChildren(this);
}

ast_bytecode_generator::~ast_bytecode_generator() {}



void ast_bytecode_generator::visitBinaryOpNode(BinaryOpNode *node) {
    TokenKind kind = node->kind();
    if(kind == tOR || kind == tAND){
        visitLazyLogicNode(node);
        return;
    }

    if(kind == tAAND || kind == tAOR || kind == tAXOR || kind == tMOD){
        visitArithmeticNode(node);
        return;
    }

    if(kind == tEQ || kind == tNEQ || kind == tLT || kind == tGT || kind == tLE || kind == tGE){
        visitComparationNode(node);
        return;
    }


    node->right()->visit(this);
    node->left()->visit(this);
    VarType common = storage.top_2_consensus();
    auto bc = storage.get_bytecode();
    switch (node->kind()){
        case tADD:
            if(common == VT_INT){
                bc->addInsn(BC_IADD);
            }else{
                bc->addInsn(BC_DADD);
            }
            break;
        case tSUB:
            if(common == VT_INT){
                bc->addInsn(BC_ISUB);
            }else{
                bc->addInsn(BC_DSUB);
            }
            break;
        case tMUL:
            if(common == VT_INT){
                bc->addInsn(BC_IMUL);
            }else{
                bc->addInsn(BC_DMUL);
            }
            break;
        case tDIV:
            if(common == VT_INT){
                bc->addInsn(BC_IDIV);
            }else{
                bc->addInsn(BC_DDIV);
            }
            break;
        default:
            assert(false);
    }
    storage.pop_type();
}

void ast_bytecode_generator::visitLazyLogicNode(BinaryOpNode *node) {
    node->left()->visit(this);
    storage.cast_top_to(VT_INT);
    storage.pop_type();
    storage.top_insn(BC_ILOAD0);
    Bytecode* bc = storage.get_bytecode();
    Label first(bc);
    if(node->kind() == tAND){
        bc->addBranch(BC_IFICMPNE, first);
        bc->addInsn(BC_ILOAD0);
    }else{
        bc->addBranch(BC_IFICMPE, first);
        bc->addInsn(BC_ILOAD1);
    }
    Label second(bc);
    bc->addBranch(BC_JA, second);
    bc->bind(first);
    node->right()->visit(this);
    storage.cast_top_to(VT_INT);
    storage.pop_type();
    bc->bind(second);
    storage.push_type(VT_INT);
}

void ast_bytecode_generator::visitArithmeticNode(BinaryOpNode *node) {
    node->right()->visit(this);
    assert(storage.tos_type() == VT_INT);
    node->left()->visit(this);
    assert(storage.tos_type() == VT_INT);
    auto bc = storage.get_bytecode();
    switch (node->kind()){
        case tAOR:
            bc->addInsn(BC_IAOR);
            break;
        case tAXOR:
            bc->addInsn(BC_IAXOR);
            break;
        case tAAND:
            bc->addInsn(BC_IAAND);
            break;
        case tMOD:
            bc->addInsn(BC_IMOD);
        default:
            assert(false);
    }
    storage.pop_type(); // TODO - maybe don't need to pop here
}

void ast_bytecode_generator::visitComparationNode(BinaryOpNode *node) {
    node->right()->visit(this);
    node->left()->visit(this);
    VarType common = storage.top_2_consensus();
    Bytecode *bc = storage.get_bytecode();
    if(common == VT_INT){
        bc->addInsn(BC_ICMP);
    }else{
        bc->addInsn(BC_DCMP);
    }
    storage.pop_type();
    storage.pop_type();
    storage.push_type(VT_INT);
    Instruction inst;
    switch (node->kind()) {
        case tEQ:
            inst = BC_IFICMPE;
            break;
        case tNEQ:
            inst = BC_IFICMPNE;
            break;
        case tLT:
            inst = BC_IFICMPG;
            break;
        case tGT:
            inst = BC_IFICMPL;
            break;
        case tLE:
            inst = BC_IFICMPGE;
            break;
        case tGE:
            inst = BC_IFICMPLE;
            break;

        default:
            assert(false);
    }
    bc->addInsn(BC_ILOAD0);
    Label l1(bc);
    bc->addBranch(inst, l1);
    bc->addInsn(BC_ILOAD0);
    Label l2(bc);
    bc->addBranch(BC_JA, l2);
    bc->bind(l1);
    bc->addInsn(BC_ILOAD1);
    storage.push_type(VT_INT);
    bc->bind(l2);


}

void ast_bytecode_generator::visitUnaryOpNode(UnaryOpNode *node){
    node->operand()->visit(this);
    Bytecode *bc = storage.get_bytecode();
    if(storage.tos_type() == VT_INT){
        switch (node->kind()){
            case tADD:
                break;
            case tSUB:
                bc->addInsn(BC_INEG);
                break;
            case tNOT:
                bc->addInsn(BC_ILOAD0);
                bc->addInsn(BC_ICMP);
                bc->addInsn(BC_ILOAD1);
                bc->addInsn(BC_IAAND);
                bc->addInsn(BC_ILOAD1);
                bc->addInsn(BC_IAXOR);
                break;
            default:
                assert(false);
        }
    } else if(storage.tos_type() == VT_DOUBLE){
        switch (node->kind()){
            case tADD:
                break;
            case tSUB:
                bc->addInsn(BC_DNEG);
                break;
            default:
                assert(false);
        }
    }else{
        assert(false);
    }
}

void ast_bytecode_generator::visitStringLiteralNode(StringLiteralNode *node) {
    uint16_t s = code->makeStringConstant(node->literal());
    storage.get_bytecode()->addInsn(BC_SLOAD);
    storage.get_bytecode()->addInt16(s);
    storage.push_type(VT_STRING);

}

void ast_bytecode_generator::visitIntLiteralNode(IntLiteralNode *node) {
    storage.get_bytecode()->addInsn(BC_ILOAD);
    storage.get_bytecode()->addInt64(node->literal());
    storage.push_type(VT_INT);
}

void ast_bytecode_generator::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    storage.get_bytecode()->addInsn(BC_DLOAD);
    storage.get_bytecode()->addDouble(node->literal());
    storage.push_type(VT_DOUBLE);
}

void ast_bytecode_generator::visitLoadNode(LoadNode *node) {
    context_entry entry = storage.find_var(node->var()->name());
    load_entry(node->var()->type(), entry);
}

void ast_bytecode_generator::load_entry(VarType type, context_entry entry) {
    bool topmost = storage.in_tompost_context(entry);
    Bytecode *bc = storage.get_bytecode();
    switch (type){
        case VT_INT:
            if(topmost){
                bc->addInsn(BC_LOADIVAR);
            }else{
                bc->addInsn(BC_LOADCTXIVAR);
            }
            storage.push_type(VT_INT);
            break;
        case VT_DOUBLE:
            if(topmost){
                bc->addInsn(BC_LOADDVAR);
            }else{
                bc->addInsn(BC_LOADCTXDVAR);
            }
            storage.push_type(VT_DOUBLE);
            break;
        case VT_STRING:
            if(topmost){
                bc->addInsn(BC_LOADSVAR);
            }else{
                bc->addInsn(BC_LOADCTXSVAR);
            }
            storage.push_type(VT_STRING);
            break;
        default:
            assert(false);
    }
    if(!topmost){
        bc->addInt16(entry.context_id);
    }
    bc->addInt16(entry.var_id);
}

void ast_bytecode_generator::visitStoreNode(StoreNode *node) {
    node->value()->visit(this);
    context_entry entry = storage.find_var(node->var()->name());
    bool topmost = storage.in_tompost_context(entry);
    Bytecode *bc = storage.get_bytecode();
    if(node->var()->type() == VT_STRING){
        if(topmost){
            bc->addInsn(BC_STORESVAR);
        }else{
            bc->addInsn(BC_STORECTXSVAR);
            bc->addInt16(entry.context_id);
        }
        bc->addInt16(entry.var_id);
    }else{
        if(node->op() != tASSIGN){
            load_entry(node->var()->type(), entry);
            VarType common = storage.top_2_consensus();
            if(node->op() == tINCRSET){
                if(common == VT_INT){
                    bc->addInsn(BC_IADD);
                }else{
                    bc->addInsn(BC_DADD);
                }
                storage.pop_type();
            }else if(node->op() == tDECRSET){
                if(common == VT_INT){
                    bc->addInsn(BC_ISUB);
                }else{
                    bc->addInsn(BC_DSUB);
                }
                storage.pop_type();
            }
        }
        storage.cast_top_to(node->var()->type());
        store_entry(node->var()->type(), entry);
    }
    storage.pop_type();
}

void ast_bytecode_generator::store_entry(VarType type, context_entry entry) {
    bool topmost = storage.in_tompost_context(entry);
    Bytecode *bc = storage.get_bytecode();
    switch (type){
        case VT_INT:
            if(topmost){
                bc->addInsn(BC_STOREIVAR);
            }else{
                bc->addInsn(BC_STORECTXIVAR);
            }
            break;
        case VT_DOUBLE:
            if(topmost){
                bc->addInsn(BC_STOREDVAR);
            }else{
                bc->addInsn(BC_STORECTXDVAR);
            }
            break;
        case VT_STRING:
            if(topmost){
                bc->addInsn(BC_STORESVAR);
            }else{
                bc->addInsn(BC_STORECTXSVAR);
            }
            break;
        default:
            assert(false);
    }
    if(!topmost){
        bc->addInt16(entry.context_id);
    }
    bc->addInt16(entry.var_id);
}

void ast_bytecode_generator::visitForNode(ForNode *node) {
    assert(node->var()->type() == VT_INT);
    assert(node->inExpr()->isBinaryOpNode());
    assert(node->inExpr()->asBinaryOpNode()->kind() == tRANGE);
    context_entry entry = storage.find_var(node->var()->name());
    node->inExpr()->asBinaryOpNode()->left()->visit(this);
    store_entry(storage.tos_type(), entry);
    storage.pop_type();
    Bytecode *bc = storage.get_bytecode();
    Label l1(bc);
    Label l2(bc);
    bc->bind(l1);
    node->inExpr()->asBinaryOpNode()->right()->visit(this);
    load_entry(VT_INT, entry);
    bc->addBranch(BC_IFICMPG, l2);
    node->body()->visit(this);
    load_entry(VT_INT, entry);
    bc->addInsn(BC_ILOAD1);
    bc->addInsn(BC_IADD);
    store_entry(VT_INT, entry);
    bc->addBranch(BC_JA, l1);
    bc->bind(l2);
    bc->addInsn(BC_POP);
    storage.pop_type();
}

void ast_bytecode_generator::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    Bytecode* bc = storage.get_bytecode();
    assert(storage.tos_type() == VT_INT);
    Label l1(bc);
    bc->addInsn(BC_ILOAD0);
    storage.push_type(VT_INT);
    bc->addBranch(BC_IFICMPE, l1);
    node->thenBlock()->visit(this);
    if(node->elseBlock() == nullptr){
        bc->bind(l1);
        return;
    }
    Label l2(bc);
    bc->addBranch(BC_JA, l2);
    bc->bind(l1);
    node->elseBlock()->visit(this);
    bc->bind(l2);
}

void ast_bytecode_generator::visitReturnNode(ReturnNode *node) {
    if(node->returnExpr() != nullptr){
        node->returnExpr()->visit(this);
        storage.cast_top_to(storage.topmost_function()->returnType());
    }
    storage.get_bytecode()->addInsn(BC_RETURN);
}

void ast_bytecode_generator::visitPrintNode(PrintNode *node) {
    Bytecode *bc = storage.get_bytecode();
    for(uint32_t i = 0; i < node->operands(); i++){
        node->operandAt(i)->visit(this);
        switch (storage.tos_type()){
            case VT_INT:
                bc->addInsn(BC_IPRINT);
                break;
            case VT_DOUBLE:
                bc->addInsn(BC_DPRINT);
                break;
            case VT_STRING:
                bc->addInsn(BC_SPRINT);
                break;
            default:
                assert(false);
        }
        storage.pop_type();
    }
}

void ast_bytecode_generator::visitWhileNode(WhileNode *node) {
    Bytecode* bc = storage.get_bytecode();
    Label l1(bc->currentLabel());
    node->whileExpr()->visit(this);
    assert(storage.tos_type() == VT_INT);
    Label l2(bc);
    bc->addInsn(BC_ILOAD0);
    storage.push_type(VT_INT);
    bc->addBranch(BC_IFICMPE, l2);
    node->loopBlock()->visit(this);
    bc->addBranch(BC_JA, l1);
    bc->bind(l2);
}

void ast_bytecode_generator::visitCallNode(CallNode *node) {
    BytecodeFunction* function = (BytecodeFunction *) code->functionByName(node->name());
    assert(function);
    for(uint32_t i = 0; i < node->parametersNumber(); i++){
        node->parameterAt(i)->visit(this);
        storage.cast_top_to(function->parameterType(i));
    }
    storage.get_bytecode()->addInsn(BC_CALL);
    storage.get_bytecode()->addInt16(function->id());
    if(function->returnType() != VT_VOID){
        storage.push_type(function->returnType());
    }
}

void ast_bytecode_generator::visitAstFunction(AstFunction *function) {
    BytecodeFunction* bc_function = (BytecodeFunction *) code->functionByName(function->name());
    storage.enter_function(bc_function);
    for(uint32_t i = function->parametersNumber(); i > 0; i--){
        context_entry entry = storage.register_variable(function->parameterName(i-1));
        switch(function->parameterType(i-1)){
            case VT_INT:
                storage.get_bytecode()->addInsn(BC_STOREIVAR);
                break;
            case VT_DOUBLE:
                storage.get_bytecode()->addInsn(BC_STOREDVAR);
                break;
            case VT_STRING:
                storage.get_bytecode()->addInsn(BC_STORESVAR);
                break;
            default:
                assert(false);
        }
        storage.get_bytecode()->addInt16(entry.var_id);
    }
    function->node()->visit(this);
    storage.exit_function();
}

