#include "parser.h"
#include "bytecode_translator.h"
#include "bytecode_interpreter.h"

#include <dlfcn.h>

#include <stdexcept>
#include <string>
#include <utility>

using std::runtime_error;
using std::string;
using std::pair;

#define CREATE_INSTRUCTION(code, type, exp) (type == VT_INT ? code##I##exp : (type == VT_DOUBLE ? code##D##exp : (type == VT_STRING ? code##S##exp : BC_INVALID)))
#define CREATE_NUMBER_INSTRUCTION(code, type, exp) (type == VT_INT ? code##I##exp : (type == VT_DOUBLE ? code##D##exp : BC_INVALID))

using namespace mathvm;

Status* bytecode_translator::translate(const string &program, Code** code) {
    Parser parser;
    Status* s = parser.parseProgram(program);
    if(s && s->isError()) return s;
    this->code = new InterpreterCodeImpl();
    *code = this->code;
    current_scope = 0;
    tos_type = VT_INVALID;
    try {
        translate_function(parser.top());
    } catch (runtime_error& e) {
        return Status::Error(e.what());
    }
    return 0;
}

void bytecode_translator::translate_function(AstFunction* f) {
    BytecodeFunction* bcf = (BytecodeFunction*)code->functionByName(f->name());
    if(!bcf) {
        bcf = new BytecodeFunction(f);
        code->addFunction(bcf);
    }
    enter_scope(bcf);
    for(Signature::const_iterator i = bcf->signature().begin() + 1; i != bcf->signature().end(); ++i) {
        AstVar *v = f->scope()->lookupVariable(i->second);
        current_scope->add_var(v);
        store_var(v);
    }
    f->node()->visit(this);
    if(current_scope->parent == 0) bytecode()->addInsn(BC_STOP);
    exit_scope();
}

void bytecode_translator::convert_tos_type(VarType to) {
    if(tos_type == to) return;
    if(tos_type == VT_INT && to == VT_DOUBLE) bytecode()->addInsn(BC_I2D);
    else if(tos_type == VT_DOUBLE && to == VT_INT) bytecode()->addInsn(BC_D2I);
    else if(tos_type == VT_STRING && to == VT_INT) bytecode()->addInsn(BC_S2I);
    else throw runtime_error(string(typeToName(tos_type)) + " to " + typeToName(to) + " conversion is not supported");
}

void bytecode_translator::convert_tos_to_bool() {
    convert_tos_type(VT_INT);
    Label setFalse(bytecode());
    Label convEnd(bytecode());
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, setFalse);
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addBranch(BC_JA, convEnd);
    bytecode()->bind(setFalse);
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->bind(convEnd);
}

VarType bytecode_translator::make_num_type_cast(VarType ltype, VarType rtype, VarType to_type, bool soft) {
    if(!(ltype == VT_INT || ltype == VT_DOUBLE) || !(rtype == VT_INT || rtype == VT_DOUBLE)) throw runtime_error("Not a number");
    if(ltype == rtype && soft) return ltype;
    Instruction insn = to_type == VT_INT ? BC_D2I : BC_I2D;
    if(ltype != to_type) {
        bytecode()->addInsn(insn);
        tos_type = to_type;
    }
    if(rtype != to_type) {
        bytecode()->addInsn(BC_SWAP);
        bytecode()->addInsn(insn);
        bytecode()->addInsn(BC_SWAP);
    }
    return to_type;
}

void bytecode_translator::create_comparison_operation(TokenKind op) {
    Label setFalse(bytecode());
    Label setTrue(bytecode());
    Instruction ins;
    switch(op) {
        case tEQ:
            ins = BC_IFICMPE;
            break;
        case tNEQ:
            ins = BC_IFICMPNE;
            break;
        case tGT:
            ins = BC_IFICMPG;
            break;
        case tGE:
            ins = BC_IFICMPGE;
            break;
        case tLT:
            ins = BC_IFICMPL;
            break;
        case tLE:
            ins = BC_IFICMPLE;
            break;
        default:
            throw runtime_error("Unsupported operation");
    }
    bytecode()->addBranch(ins, setTrue);
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_JA, setFalse);
    bytecode()->bind(setTrue);
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->bind(setFalse);
    tos_type = VT_INT;
}

void bytecode_translator::create_math_operation(TokenKind op) {
    switch(op) {
        case tADD:
            bytecode()->addInsn(CREATE_NUMBER_INSTRUCTION(BC_, tos_type, ADD));
            break;
        case tSUB:
            bytecode()->addInsn(CREATE_NUMBER_INSTRUCTION(BC_, tos_type, SUB));
            break;
        case tMUL:
            bytecode()->addInsn(CREATE_NUMBER_INSTRUCTION(BC_, tos_type, MUL));
            break;
        case tDIV:
            bytecode()->addInsn(CREATE_NUMBER_INSTRUCTION(BC_, tos_type, DIV));
            break;
        case tMOD:
            if(tos_type != VT_INT) throw runtime_error("Unsupported type for math operation");
            bytecode()->addInsn(BC_IMOD);
            break;
        default:
            throw runtime_error("Unsupported operation");
    }
}

void bytecode_translator::create_logic_operation(TokenKind op) {
    switch(op) {
        case tAAND:
            bytecode()->addInsn(BC_IAAND);
            break;
        case tAOR:
            bytecode()->addInsn(BC_IAOR);
            break;
        case tAXOR:
            bytecode()->addInsn(BC_IAXOR);
            break;
        default:
            throw runtime_error("Unsupported operation");
    }
}

void bytecode_translator::process_var(const AstVar* var, bool loading) {
    pair<uint16_t, TScope*> scopeVar = current_scope->get_var(var);
    if(scopeVar.second->function->id() == current_scope->function->id()) {
        #define MAKE_INSN_SEL(cond, b1, b2, t, e) (cond ? CREATE_INSTRUCTION(b1, t, e) : CREATE_INSTRUCTION(b2, t, e))
        switch(scopeVar.first) {
            case 0:
                bytecode()->addInsn(loading ? CREATE_INSTRUCTION(BC_LOAD, var->type(), VAR0) : CREATE_INSTRUCTION(BC_STORE, var->type(), VAR0));
                break;
            case 1:
                bytecode()->addInsn(loading ? CREATE_INSTRUCTION(BC_LOAD, var->type(), VAR1) : CREATE_INSTRUCTION(BC_STORE, var->type(), VAR1));
                break;
            case 2:
                bytecode()->addInsn(loading ? CREATE_INSTRUCTION(BC_LOAD, var->type(), VAR2) : CREATE_INSTRUCTION(BC_STORE, var->type(), VAR2));
                break;
            case 3:
                bytecode()->addInsn(loading ? CREATE_INSTRUCTION(BC_LOAD, var->type(), VAR3) : CREATE_INSTRUCTION(BC_STORE, var->type(), VAR3));
                break;
            default:
                bytecode()->addInsn(loading ? CREATE_INSTRUCTION(BC_LOAD, var->type(), VAR) : CREATE_INSTRUCTION(BC_STORE, var->type(), VAR));
                bytecode()->addUInt16(scopeVar.first);
        }
    } else {
        bytecode()->addInsn(loading ? CREATE_INSTRUCTION(BC_LOADCTX, var->type(), VAR) : CREATE_INSTRUCTION(BC_STORECTX, var->type(), VAR));
        bytecode()->addUInt16(scopeVar.second->function->id());
        bytecode()->addUInt16(scopeVar.first);
    }
}

void bytecode_translator::visitBinaryOpNode(BinaryOpNode* node) {
    node->right()->visit(this);
    VarType rtype = tos_type;
    node->left()->visit(this);
    VarType ltype = tos_type;
    switch(node->kind()) {
        case tOR:
            convert_tos_to_bool();
            bytecode()->addInsn(BC_SWAP);
            convert_tos_to_bool();
            bytecode()->addInsn(BC_SWAP);
            bytecode()->addInsn(BC_IADD);
            convert_tos_to_bool();
            break;
        case tAND:
            convert_tos_to_bool();
            bytecode()->addInsn(BC_SWAP);
            convert_tos_to_bool();
            bytecode()->addInsn(BC_SWAP);
            bytecode()->addInsn(BC_IMUL);
            break;
        case tEQ:
        case tNEQ:
        case tGE:
        case tGT:
        case tLE:
        case tLT:
            make_num_type_cast(ltype, rtype, VT_INT, false);
            create_comparison_operation(node->kind());
            break;
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            make_num_type_cast(ltype, rtype, VT_DOUBLE, true);
            create_math_operation(node->kind());
            break;
        case tAOR:
        case tAXOR:
        case tAAND:
            make_num_type_cast(ltype, rtype, VT_INT, false);
            create_logic_operation(node->kind());
            break;
        default: throw runtime_error("Unsupported operation");
    }
}

void bytecode_translator::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);
    switch(node->kind()) {
        case tNOT:
            convert_tos_to_bool();
            bytecode()->addInsn(BC_ILOAD1);
            bytecode()->addInsn(BC_ISUB);
            break;
        case tSUB:
            if(!(tos_type == VT_INT || tos_type == VT_DOUBLE)) throw runtime_error("Not a number");
            bytecode()->addInsn(CREATE_NUMBER_INSTRUCTION(BC_, tos_type, NEG));
            break;
        default:
            throw string("Unsupported unary operation: ") + tokenOp(node->kind());
    }
}

void bytecode_translator::visitStringLiteralNode(StringLiteralNode* node) {
    bytecode()->addInsn(BC_SLOAD);
    bytecode()->addUInt16(code->makeStringConstant(node->literal()));
    tos_type = VT_STRING;
}

void bytecode_translator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    bytecode()->addInsn(BC_DLOAD);
    bytecode()->addDouble(node->literal());
    tos_type = VT_DOUBLE;
}

void bytecode_translator::visitIntLiteralNode(IntLiteralNode* node) {
    bytecode()->addInsn(BC_ILOAD);
    bytecode()->addInt64(node->literal());
    tos_type = VT_INT;
}

void bytecode_translator::visitLoadNode(LoadNode* node) {
    load_var(node->var());
}

void bytecode_translator::visitStoreNode(StoreNode* node) {
    node->value()->visit(this);

    switch(node->op()) {
        case tASSIGN:
            break;
        case tINCRSET:
            load_var(node->var());
            bytecode()->addInsn(CREATE_NUMBER_INSTRUCTION(BC_, node->var()->type(), ADD));
            break;
        case tDECRSET:
            load_var(node->var());
            bytecode()->addInsn(CREATE_NUMBER_INSTRUCTION(BC_, node->var()->type(), SUB));
            break;
        default:
            throw runtime_error(string("Unsupported operation in StoreNode: ") + tokenOp(node->op()));
    }
    store_var(node->var());
}

void bytecode_translator::visitForNode(ForNode* node) {
    Label forStart(bytecode());
    Label forEnd(bytecode());
    if(node->var()->type() != VT_INT) throw runtime_error("Non integer variable in for expression");
    if(!node->inExpr()->isBinaryOpNode()) throw runtime_error("Non-range type in for expression");
    BinaryOpNode *inExpr = node->inExpr()->asBinaryOpNode();
    if(inExpr->kind() != tRANGE) throw runtime_error("Non-range type in for expression");
    inExpr->left()->visit(this);
    if(tos_type != VT_INT) throw runtime_error("Non integer range in for expression");
    store_var(node->var());
    bytecode()->bind(forStart);
    inExpr->right()->visit(this);
    if(tos_type != VT_INT) throw runtime_error("Non integer range in for expression");
    load_var(node->var());
    bytecode()->addBranch(BC_IFICMPG, forEnd);
    node->body()->visit(this);
    load_var(node->var());
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addInsn(BC_IADD);
    store_var(node->var());
    bytecode()->addBranch(BC_JA, forStart);
    bytecode()->bind(forEnd);
}

void bytecode_translator::visitWhileNode(WhileNode* node) {
    Label whileStart(bytecode());
    Label whileEnd(bytecode());
    bytecode()->bind(whileStart);
    node->whileExpr()->visit(this);
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, whileEnd);
    node->loopBlock()->visit(this);
    bytecode()->addBranch(BC_JA, whileStart);
    bytecode()->bind(whileEnd);
}

void bytecode_translator::visitIfNode(IfNode* node) {
    Label ifElse(bytecode());
    Label ifEnd(bytecode());
    node->ifExpr()->visit(this);
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_IFICMPE, ifElse);
    node->thenBlock()->visit(this);
    bytecode()->addBranch(BC_JA, ifEnd);
    bytecode()->bind(ifElse);
    if(node->elseBlock()) node->elseBlock()->visit(this);
    bytecode()->bind(ifEnd);
}

void bytecode_translator::visitBlockNode(BlockNode* node) {
    current_scope->function->setLocalsNumber(current_scope->function->localsNumber() + node->scope()->variablesCount());
    Scope::VarIterator vars(node->scope());
    while(vars.hasNext())
        current_scope->add_var(vars.next());
    Scope::FunctionIterator funcs(node->scope());
    while(funcs.hasNext())
        code->addFunction(new BytecodeFunction(funcs.next()));
    funcs = Scope::FunctionIterator(node->scope());
    while(funcs.hasNext())
        translate_function(funcs.next());
    for(uint32_t i = 0; i < node->nodes(); ++i)
        node->nodeAt(i)->visit(this);
}

void bytecode_translator::visitFunctionNode(FunctionNode* node) {
    if(node->body()->nodeAt(0)->isNativeCallNode()) node->body()->nodeAt(0)->visit(this);
    else node->body()->visit(this);
}

void bytecode_translator::visitReturnNode(ReturnNode* node) {
    if(node->returnExpr()) {
        node->returnExpr()->visit(this);
        convert_tos_type(current_scope->function->returnType());
    }
    bytecode()->addInsn(BC_RETURN);
}

void bytecode_translator::visitCallNode(CallNode* node) {
    TranslatedFunction* f = code->functionByName(node->name());
    if(!f) throw runtime_error("Function " + node->name() + " is not defined");
    for(int i = node->parametersNumber() - 1; i >= 0; --i) {
        node->parameterAt(i)->visit(this);
        convert_tos_type(f->parameterType(i));
    }
    bytecode()->addInsn(BC_CALL);
    bytecode()->addUInt16(f->id());
    if(f->returnType() != VT_VOID) tos_type = f->returnType();
}

void bytecode_translator::visitNativeCallNode(NativeCallNode* node) {
    void* addr = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if(!addr) throw runtime_error("Native function " + node->nativeName() + " not found");
    uint16_t id = code->makeNativeFunction(node->nativeName(), node->nativeSignature(), addr);
    bytecode()->addInsn(BC_CALLNATIVE);
    bytecode()->addUInt16(id);
    bytecode()->addInsn(BC_RETURN);
}

void bytecode_translator::visitPrintNode(PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        bytecode()->addInsn(CREATE_INSTRUCTION(BC_, tos_type, PRINT));
    }
}
