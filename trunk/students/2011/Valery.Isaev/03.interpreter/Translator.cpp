#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "Translator.h"
#include "ShowVisitor.h"
#include "BytecodeFunction.h"

#define INTERNAL_ERROR \
    throwError(node, "Internal error at %s:%d", __FILE__, __LINE__);

void throwError(const mathvm::AstNode* node, const char* format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    vasprintf(&buf, format, args);
    mathvm::Status s(buf, node ? node->position() : mathvm::Status::INVALID_POSITION);
    free(buf);
    throw s;
}

struct FunInfo {
    std::set<const mathvm::AstVar*> freeVars;
    std::set<std::pair<FunInfo*, mathvm::Scope*> > callees;
    BytecodeFunction* bcfun;
};

std::map<const mathvm::CallNode*, FunInfo*> callNodes;
std::map<const mathvm::AstFunction*, FunInfo> infos;
std::set<const mathvm::AstVar*> globals;

class Label {
    mathvm::Label label;
    mathvm::Bytecode* code;
public:
    Label(mathvm::Bytecode* c): label(c), code(c) {}
    ~Label() {
        if (!label.isBound()) {
            code->bind(label);
        }
    }
    mathvm::Label& operator()() { return label; }
};

class FreeVarsVisitor: public mathvm::AstVisitor {
    FunInfo* info;
    mathvm::Scope* scope;
    mathvm::Scope* funScope;
    template<class T> void addFreeVar(const T* t) {
        if (!lookupVariable(t->var()) && !globals.count(t->var())) {
            info->freeVars.insert(t->var());
        }
    }
    bool lookupVariable(const mathvm::AstVar* var) {
        for (mathvm::Scope* s = scope; s; s = s->parent()) {
            if (s->lookupVariable(var->name()) == var) {
                return true;
            }
            if (s == funScope) {
                break;
            }
        }
        return false;
    }
    bool lookupVariable(mathvm::Scope* s, const mathvm::AstVar* v) {
        for (; s; s = s->parent()) {
            if (s->lookupVariable(v->name()) == v) {
                return true;
            }
        }
        return false;
    }
    bool global;
    std::map<std::string, FunInfo*>* functions;
    FreeVarsVisitor(mathvm::AstFunction* v, std::map<std::string, FunInfo*>* f)
        : global(false), functions(f) { init(v); }
    void init(mathvm::AstFunction* v) {
        info = &infos[v];
        info->bcfun = new BytecodeFunction(v);
        scope = v->node()->body()->scope();
        funScope = scope->parent();
        v->node()->body()->visit(this);
    }
public:
    FreeVarsVisitor(mathvm::AstFunction* v): global(true),
    functions(new std::map<std::string, FunInfo*>()) {
        init(v);
        for (bool repeat = true; repeat;) {
            repeat = false;
            for (std::map<const mathvm::AstFunction*, FunInfo>::iterator it
             = infos.begin(); it != infos.end(); ++it) {
                size_t size = it->second.freeVars.size();
                for (std::set<std::pair<FunInfo*, mathvm::Scope*> >::iterator
                 it1 = it->second.callees.begin(); it1 != it->second.callees.end(); ++it1) {
                    for (std::set<const mathvm::AstVar*>::iterator it2 =
                     it1->first->freeVars.begin(); it2 != it1->first->freeVars.end(); ++it2) {
                        scope = it1->second;
                        funScope = it->first->node()->body()->scope()->parent();
                        if (!lookupVariable(*it2)) {
                            it->second.freeVars.insert(*it2);
                        }
                    }
                }
                if (it->second.freeVars.size() != size) {
                    repeat = true;
                }
            }
        }
        for (std::map<const mathvm::AstFunction*, FunInfo>::iterator it
         = infos.begin(); it != infos.end(); ++it) {
            it->second.bcfun->setFreeVarsNumber(it->second.freeVars.size());
        }
    }
    ~FreeVarsVisitor() {
        if (global) delete functions;
    }
    void visitBinaryOpNode(mathvm::BinaryOpNode* node) { node->visitChildren(this); }
    void visitUnaryOpNode(mathvm::UnaryOpNode* node) { node->visitChildren(this); }
    void visitReturnNode(mathvm::ReturnNode* node) { node->visitChildren(this); }
    void visitWhileNode(mathvm::WhileNode* node) { node->visitChildren(this); }
    void visitPrintNode(mathvm::PrintNode* node) { node->visitChildren(this); }
    void visitLoadNode(mathvm::LoadNode* node) { addFreeVar(node); }
    void visitCallNode(mathvm::CallNode* node) {
        node->visitChildren(this);
        std::map<std::string, FunInfo*>::iterator it = (*functions).find(node->name());
        if (it == functions->end()) {
            throwError(node, "Undeclared function: %s", node->name().c_str());
        }
        info->callees.insert(std::make_pair(it->second, scope));
        callNodes[node] = it->second;
    }
    void visitIfNode(mathvm::IfNode* node) {
        node->ifExpr()->visit(this);
        uint32_t loc1 = info->bcfun->localsNumber();
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            uint32_t loc2 = info->bcfun->localsNumber();
            info->bcfun->setLocalsNumber(loc1);
            node->elseBlock()->visit(this);
            if (loc2 > info->bcfun->localsNumber()) {
                info->bcfun->setLocalsNumber(loc2);
            }
        }
    }
    void visitStoreNode(mathvm::StoreNode* node) {
        node->visitChildren(this);
        addFreeVar(node);
    }
    void visitForNode(mathvm::ForNode* node) {
        node->visitChildren(this);
        addFreeVar(node);
    }
    void visitBlockNode(mathvm::BlockNode* node) {
        mathvm::Scope* scope1 = scope;
        scope = node->scope();
        std::vector<std::map<std::string, FunInfo*>::iterator> f;
        for (mathvm::Scope::VarIterator it(node->scope()); it.hasNext();) {
            info->bcfun->setLocalsNumber(info->bcfun->localsNumber() + 1);
            mathvm::AstVar* var = it.next();
            if (global) {
                globals.insert(var);
            }
        }
        global = false;
        uint32_t loc1 = info->bcfun->localsNumber(), loc_max = 0;
        for (mathvm::Scope::FunctionIterator it(node->scope()); it.hasNext();) {
            mathvm::AstFunction* fun = it.next();
            std::pair<std::map<std::string, FunInfo*>::iterator, bool> p =
                functions->insert(make_pair(fun->name(), &infos[fun]));
            if (p.second) {
                f.push_back(p.first);
            }
        }
        for (mathvm::Scope::FunctionIterator it(node->scope()); it.hasNext();) {
            FreeVarsVisitor fv(it.next(), functions);
            for (std::set<const mathvm::AstVar*>::iterator it1 = fv.info->freeVars.begin();
             it1 != fv.info->freeVars.end(); ++it1) {
                if (!lookupVariable(node->scope(), *it1)) {
                    info->freeVars.insert(*it1);
                }
            }
            info->callees.insert(fv.info->callees.begin(), fv.info->callees.end());
        }
        for (uint32_t i = 0; i < node->nodes(); ++i) {
            info->bcfun->setLocalsNumber(0);
            node->nodeAt(i)->visit(this);
            if (info->bcfun->localsNumber() > loc_max) {
                loc_max = info->bcfun->localsNumber();
            }
        }
        info->bcfun->setLocalsNumber(loc1 + loc_max);
        for (uint32_t i = 0; i < f.size(); ++i) {
            functions->erase(f[i]);
        }
        scope = scope1;
    }
};

std::string showExpr(mathvm::AstNode* node) {
    std::stringstream str;
    ShowVisitor v(str);
    v.show(node);
    return str.str();
}

void typeMismatch(const char* e, mathvm::AstNode* expr, mathvm::VarType a) {
    throwError(expr, "Expected expression of type %s, but %s has type %s",
        e, showExpr(expr).c_str(), typeToName(a));
}

void Translator::checkTypeInt(mathvm::AstNode* expr) {
    expr->visit(this);
    if (currentType != mathvm::VT_INT) {
        typeMismatch("int", expr, currentType);
    }
    currentType = mathvm::VT_INVALID;
}

Translator::Translator(Interpreter* p): prog(p), currentVar(0),
    overflow(false), currentType(mathvm::VT_INVALID), resultType(mathvm::VT_VOID) { }

void Translator::put(const void* buf_, unsigned int size) {
    const uint8_t* buf = (const uint8_t*)buf_;
    while (size--) {
        code->add(*buf++);
    }
}

void Translator::putVar(mathvm::Instruction ins, const mathvm::AstVar* var, mathvm::AstNode* node) {
    const std::vector<VarInt>& v = vars[var->name()];
    if (v.empty()) INTERNAL_ERROR
    if (v.size() == 1 && globals.count(var)) {
        switch (ins) {
            case mathvm::BC_LOADIVAR: code->add(mathvm::BC_LOADDVAR); break;
            case mathvm::BC_STOREIVAR: code->add(mathvm::BC_STOREDVAR); break;
            default: INTERNAL_ERROR
        }
    } else {
        code->add(ins);
    }
    put(&v.back(), sizeof(VarInt));
}

void Translator::triple(mathvm::Instruction i) {
    Label label1(code);
    { Label label(code);
      code->addBranch(i, label());
      code->add(mathvm::BC_ILOAD0);
      code->addBranch(mathvm::BC_JA, label1());
    }
    code->add(mathvm::BC_ILOAD1);
}

void Translator::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
    using namespace mathvm;
    
    if (node->kind() == tOR || node->kind() == tAND) {
        checkTypeInt(node->left());
        code->add(BC_ILOAD0);
        ::Label label1(code);
        { ::Label label(code);
          code->addBranch(node->kind() == tOR ? BC_IFICMPE : BC_IFICMPNE, label());
          code->add(node->kind() == tOR ? BC_ILOAD1 : BC_ILOAD0);
          code->addBranch(BC_JA, label1());
        }
        node->right()->visit(this);
        if (currentType != VT_INT) {
            typeMismatch("int", node->right(), currentType);
        }
        return;
    }
    node->right()->visit(this);
    VarType right = currentType;
    currentType = VT_INVALID;
    node->left()->visit(this);
    VarType left = currentType;
    switch (node->kind()) {
        case tEQ: case tNEQ:
        case tGT: case tGE: case tLT: case tLE:
        case tADD: case tSUB: case tMUL: case tDIV:
            if (left != VT_INT && left != VT_DOUBLE) {
                typeMismatch("int or double", node->left(), left);
            }
            if (right != VT_INT && right != VT_DOUBLE) {
                typeMismatch("int or double", node->right(), right);
            }
            if (left != right) {
                if (left == VT_INT) {
                    currentType = VT_DOUBLE;
                    code->add(BC_I2D);
                } else {
                    code->add(BC_SWAP);
                    code->add(BC_I2D);
                    if (node->kind() != tADD && node->kind() != tMUL) {
                        code->add(BC_SWAP);
                    }
                }
            }
            break;
        default: throwError(node, "Unexpected token %s", tokenOp(node->kind()));
    }
    if (currentType == VT_DOUBLE) {
        switch (node->kind()) {
            case tEQ: case tNEQ: case tGT: case tGE: case tLT: case tLE:
                code->add(BC_DCMP); currentType = VT_INT;
            default:;
        }
        switch (node->kind()) {
            case tNEQ: code->add(BC_ILOAD0); triple(BC_IFICMPNE); break;
            case tEQ: code->add(BC_ILOAD0); triple(BC_IFICMPE); break;
            case tGT: code->add(BC_ILOADM1); triple(BC_IFICMPE); break;
            case tGE: code->add(BC_ILOAD1); triple(BC_IFICMPNE); break;
            case tLT: code->add(BC_ILOAD1); triple(BC_IFICMPE); break;
            case tLE: code->add(BC_ILOADM1); triple(BC_IFICMPNE); break;
            case tADD: code->add(BC_DADD); break;
            case tSUB: code->add(BC_DSUB); break;
            case tMUL: code->add(BC_DMUL); break;
            case tDIV: code->add(BC_DDIV); break;
            default:;
        }
    } else
    switch (node->kind()) {
        case tEQ: triple(BC_IFICMPE); break;
        case tNEQ: triple(BC_IFICMPNE); break;
        case tGT: triple(BC_IFICMPG); break;
        case tGE: triple(BC_IFICMPGE); break;
        case tLT: triple(BC_IFICMPL); break;
        case tLE: triple(BC_IFICMPLE); break;
        case tADD: code->add(BC_IADD); break;
        case tSUB: code->add(BC_ISUB); break;
        case tMUL: code->add(BC_IMUL); break;
        case tDIV: code->add(BC_IDIV); break;
        default:;
    }
}

void Translator::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    node->operand()->visit(this);
    switch (node->kind()) {
        case mathvm::tSUB: switch (currentType) {
            case mathvm::VT_INT: code->add(mathvm::BC_INEG); break;
            case mathvm::VT_DOUBLE: code->add(mathvm::BC_DNEG); break;
            default: typeMismatch("int or double", node->operand(), currentType);
        } break;
        case mathvm::tNOT: {
            if (currentType != mathvm::VT_INT) {
                typeMismatch("int", node->operand(), currentType);
            }
            code->add(mathvm::BC_ILOAD0);
            triple(mathvm::BC_IFICMPE);
            break;
        }
        default: INTERNAL_ERROR
    }
}

void Translator::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
    code->add(mathvm::BC_SLOAD);
    code->addInt16(prog->makeStringConstant(node->literal()));
    currentType = mathvm::VT_STRING;
}

void Translator::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
    if (node->literal() == -1) {
        code->add(mathvm::BC_DLOADM1);
    } else
    if (node->literal() == 0) {
        code->add(mathvm::BC_DLOAD0);
    } else
    if (node->literal() == 1) {
        code->add(mathvm::BC_DLOAD1);
    } else {
        code->add(mathvm::BC_DLOAD);
        if (sizeof(double) == 8) {
            double l = node->literal();
            put(&l, sizeof(l));
        } else
        if (sizeof(long double) == 8) {
            long double l = node->literal();
            put(&l, sizeof(l));
        } else
        if (sizeof(float) == 8) {
            float l = node->literal();
            put(&l, sizeof(l));
        } else throwError(node, "I'm sorry, Dave. "
            "I'm afraid I can't compile a code with floating point numbers, "
            "cause your system doesn't have 8-byte numbers.");
    }
    currentType = mathvm::VT_DOUBLE;
}

void Translator::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
    int64_t l = node->literal();
    switch (l) {
        case -1: code->add(mathvm::BC_ILOADM1); break;
        case 0: code->add(mathvm::BC_ILOAD0); break;
        case 1: code->add(mathvm::BC_ILOAD1); break;
        default: code->add(mathvm::BC_ILOAD); put(&l, sizeof(l));
    }
    currentType = mathvm::VT_INT;
}

void Translator::visitLoadNode(mathvm::LoadNode* node) {
    putVar(mathvm::BC_LOADIVAR, node->var(), node);
    currentType = node->var()->type();
}

void Translator::visitStoreNode(mathvm::StoreNode* node) {
    mathvm::AstNode* right = node->value();
    mathvm::LoadNode l(0, node->var());
    mathvm::BinaryOpNode tnode(0, node->op() == mathvm::tINCRSET
        ? mathvm::tADD : mathvm::tSUB, &l, right);
    switch (node->op()) {
        case mathvm::tASSIGN: break;
        case mathvm::tINCRSET:
        case mathvm::tDECRSET: right = &tnode; break;
        default: INTERNAL_ERROR
    }
    right->visit(this);
    if (node->var()->type() != currentType) {
        typeMismatch(typeToName(node->var()->type()), node->value(), currentType);
    }
    putVar(mathvm::BC_STOREIVAR, node->var(), node);
    currentType = mathvm::VT_INVALID;
}

void Translator::visitForNode(mathvm::ForNode* node) {
    if (node->var()->type() != mathvm::VT_INT) {
        throwError(node, "Variable %s should have type int",
            node->var()->name().c_str());
    }
    mathvm::BinaryOpNode* in =
        dynamic_cast<mathvm::BinaryOpNode*>(node->inExpr());
    if (!in || in->kind() != mathvm::tRANGE) {
        node->inExpr()->visit(this);
        typeMismatch("range", node->inExpr(), currentType);
    }
    checkTypeInt(in->left());
    putVar(mathvm::BC_STOREIVAR, node->var(), node);
    Label start(code), end(code);
    code->bind(start());
    checkTypeInt(in->right());
    putVar(mathvm::BC_LOADIVAR, node->var(), node);
    code->addBranch(mathvm::BC_IFICMPG, end());
    node->body()->visit(this);
    currentType = mathvm::VT_INVALID;
    putVar(mathvm::BC_LOADIVAR, node->var(), node);
    code->add(mathvm::BC_ILOAD1);
    code->add(mathvm::BC_IADD);
    putVar(mathvm::BC_STOREIVAR, node->var(), node);
    code->addBranch(mathvm::BC_JA, start());
}

void Translator::visitWhileNode(mathvm::WhileNode* node) {
    Label start(code), end(code);
    code->bind(start());
    mathvm::IntLiteralNode* in =
        dynamic_cast<mathvm::IntLiteralNode*>(node->whileExpr());
    if (in) {
        if (in->literal() == 0) {
            return;
        }
    } else {
        checkTypeInt(node->whileExpr());
        code->add(mathvm::BC_ILOAD0);
        code->addBranch(mathvm::BC_IFICMPE, end());
    }
    node->loopBlock()->visit(this);
    currentType = mathvm::VT_INVALID;
    code->addBranch(mathvm::BC_JA, start());
}

void Translator::visitIfNode(mathvm::IfNode* node) {
    checkTypeInt(node->ifExpr());
    code->add(mathvm::BC_ILOAD0);
    Label label(code);
    code->addBranch(mathvm::BC_IFICMPE, label());
    node->thenBlock()->visit(this);
    currentType = mathvm::VT_INVALID;
    if (node->elseBlock()) {
        Label label1(code);
        code->addBranch(mathvm::BC_JA, label1());
        code->bind(label());
        node->elseBlock()->visit(this);
        currentType = mathvm::VT_INVALID;
    }
}

void Translator::addVar(mathvm::AstNode* node, const std::string& name) {
    if (overflow) {
        throwError(node, "I'm sorry, Dave. I'm afraid you exceeded the limit of"
            " variables which is %d.", (unsigned long long)1 + VarInt(-1));
    }
    vars[name].push_back(currentVar++);
    if (!currentVar) {
        overflow = true;
    }
}

void Translator::delVar(const std::string& name) {
    vars[name].pop_back();
    overflow = false;
    --currentVar;
}

void Translator::visitBlockNode(mathvm::BlockNode* node) {
    for (mathvm::Scope::VarIterator it(node->scope()); it.hasNext();) {
        addVar(node, it.next()->name());
    }
    for (mathvm::Scope::FunctionIterator it(node->scope()); it.hasNext();) {
        prog->addFunction(infos[it.next()].bcfun);
    }
    for (mathvm::Scope::FunctionIterator it(node->scope()); it.hasNext();) {
        mathvm::AstFunction* fun = it.next();
        FunInfo* info = &infos[fun];
        Translator trans(prog);
        trans.code = info->bcfun->bytecode();
        trans.resultType = fun->returnType();
        trans.currentVar = 0;
        if (trans.resultType != mathvm::VT_VOID) {
            ++trans.currentVar;
        }
        for (std::set<const mathvm::AstVar*>::iterator it = globals.begin(); it != globals.end(); ++it) {
            trans.vars[(*it)->name()].push_back(vars[(*it)->name()].front());
        }
        for (std::set<const mathvm::AstVar*>::iterator it = info->freeVars.begin(); it != info->freeVars.end(); ++it) {
            trans.addVar(node, (*it)->name());
        }
        for (uint32_t i = 0; i < fun->parametersNumber(); ++i) {
            trans.addVar(node, fun->parameterName(i));
        }
        fun->node()->body()->visit(&trans);
    }
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        mathvm::CallNode* callNode = dynamic_cast<mathvm::CallNode*>(node->nodeAt(i));
        node->nodeAt(i)->visit(this);
        if (callNode && callNodes[callNode]->bcfun->returnType() != mathvm::VT_VOID) {
            code->add(mathvm::BC_POP);
        }
    }
    for (mathvm::Scope::VarIterator it(node->scope()); it.hasNext();) {
        delVar(it.next()->name());
    }
}

void Translator::visitFunctionNode(mathvm::FunctionNode* node) { INTERNAL_ERROR }

void Translator::visitPrintNode(mathvm::PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        switch (currentType) {
            case mathvm::VT_INT: code->add(mathvm::BC_IPRINT); break;
            case mathvm::VT_DOUBLE: code->add(mathvm::BC_DPRINT); break;
            case mathvm::VT_STRING: code->add(mathvm::BC_SPRINT); break;
            case mathvm::VT_VOID: typeMismatch("int or double or string",
                node->operandAt(i), mathvm::VT_VOID); break;
            default: INTERNAL_ERROR
        }
        currentType = mathvm::VT_INVALID;
    }
    currentType = mathvm::VT_VOID;
}

void Translator::visitReturnNode(mathvm::ReturnNode* node) {
    if (node->returnExpr()) {
        if (resultType == mathvm::VT_VOID) {
            throwError(node, "Returning a value from a void function");
        }
        node->returnExpr()->visit(this);
        if (resultType != currentType) {
            if (currentType == mathvm::VT_INT && resultType == mathvm::VT_DOUBLE) {
                code->add(mathvm::BC_I2D);
            } else {
                typeMismatch(typeToName(resultType), node->returnExpr(), currentType);
            }
        }
    } else {
        if (resultType != mathvm::VT_VOID) {
            throwError(node, "Returning no value from a non-void function");
        }
    }
    currentType = mathvm::VT_INVALID;
    if (resultType != mathvm::VT_VOID) {
        code->add(mathvm::BC_STOREIVAR0);
    }
    code->add(mathvm::BC_RETURN);
}

void Translator::visitCallNode(mathvm::CallNode* node) {
    FunInfo* info = callNodes[node];
    if (info->bcfun->parametersNumber() != node->parametersNumber()) {
        throwError(node, "Function %s expects %d arguments but got %d",
            node->name().c_str(), info->bcfun->parametersNumber(), node->parametersNumber());
    }
    if (info->bcfun->returnType() != mathvm::VT_VOID) {
        code->add(mathvm::BC_ILOAD0);
    }
    for (std::set<const mathvm::AstVar*>::iterator it = info->freeVars.begin(); it != info->freeVars.end(); ++it) {
        putVar(mathvm::BC_LOADIVAR, *it);
    }
    for (uint32_t i = 0; i < info->bcfun->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        if (currentType != info->bcfun->parameterType(i)) {
            if (currentType == mathvm::VT_INT && info->bcfun->parameterType(i) == mathvm::VT_DOUBLE) {
                code->add(mathvm::BC_I2D);
            } else {
                throwError(node, "Expected expression of type %s "
                    "for argument %d to function %s but %s has type %s",
                    typeToName(info->bcfun->parameterType(i)), i, node->name().c_str(),
                    showExpr(node->parameterAt(i)).c_str(), typeToName(currentType));
            }
        }
    }
    code->add(mathvm::BC_CALL);
    code->addTyped(info->bcfun->id());
    currentType = info->bcfun->returnType();
    for (std::set<const mathvm::AstVar*>::reverse_iterator it = info->freeVars.rbegin();
     it != info->freeVars.rend(); ++it) {
        putVar(mathvm::BC_STOREIVAR, *it);
    }
}

mathvm::Status Translator::translate(mathvm::AstFunction* fun) {
    try {
        FreeVarsVisitor v(fun);
        BytecodeFunction* main = infos[fun].bcfun;
        prog->addFunction(main);
        code = main->bytecode();
        fun->node()->body()->visit(this);
        code->add(mathvm::BC_STOP);
        
        uint16_t i = 0;
        for (std::set<const mathvm::AstVar*>::iterator it = globals.begin(); it != globals.end(); ++it) {
            prog->globalVarsMap()[(*it)->name()] = i++;
        }
    } catch (const mathvm::Status& e) {
        return e;
    }
    return mathvm::Status();
}
