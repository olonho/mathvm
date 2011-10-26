#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "Translator.h"
#include "ShowVisitor.h"
#include "BytecodeFunction.h"

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
    uint32_t loc;
    BytecodeFunction* fun;
    std::set<std::string> exclude;
    template<class T> void addFreeVar(const T* t) {
        if (exclude.find(t->var()->name()) == exclude.end()) {
            fun->addFreeVar(t);
        }
    }
public:
    FreeVarsVisitor(BytecodeFunction* _fun, mathvm::AstFunction* v): loc(0), fun(_fun) {
        v->node()->visit(this);
        fun->setLocalsNumber(loc);
    }
    void visitBinaryOpNode(mathvm::BinaryOpNode* node) { node->visitChildren(this); }
    void visitUnaryOpNode(mathvm::UnaryOpNode* node) { node->visitChildren(this); }
    void visitReturnNode(mathvm::ReturnNode* node) { node->visitChildren(this); }
    void visitWhileNode(mathvm::WhileNode* node) { node->visitChildren(this); }
    void visitPrintNode(mathvm::PrintNode* node) { node->visitChildren(this); }
    void visitCallNode(mathvm::CallNode* node) { node->visitChildren(this); }
    void visitLoadNode(mathvm::LoadNode* node) { addFreeVar(node); }
    void visitIfNode(mathvm::IfNode* node) {
        node->ifExpr()->visit(this);
        uint32_t loc1 = loc;
        node->thenBlock()->visit(this);
        if (node->elseBlock()) {
            uint32_t loc2 = loc;
            loc = loc1;
            node->elseBlock()->visit(this);
            if (loc2 > loc) loc = loc2;
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
    void visitFunctionNode(mathvm::FunctionNode* node) {
        std::vector<std::set<std::string>::iterator> v;
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            std::pair<std::set<std::string>::iterator, bool> p =
                exclude.insert(node->parameterName(i));
            if (p.second) {
                v.push_back(p.first);
            }
        }
        node->visitChildren(this);
        for (uint32_t i = 0; i < v.size(); ++i) {
            exclude.erase(v[i]);
        }
    }
    void visitBlockNode(mathvm::BlockNode* node) {
        std::vector<std::set<std::string>::iterator> v;
        for (mathvm::Scope::VarIterator it(node->scope()); it.hasNext();) {
            ++loc;
            std::pair<std::set<std::string>::iterator, bool> p =
                exclude.insert(it.next()->name());
            if (p.second) {
                v.push_back(p.first);
            }
        }
        uint32_t loc1 = loc, loc_max = 0;
        for (mathvm::Scope::FunctionIterator it(node->scope()); it.hasNext();) {
            it.next()->node()->visit(this);
        }
        for (uint32_t i = 0; i < node->nodes(); ++i) {
            loc = 0;
            node->nodeAt(i)->visit(this);
            if (loc > loc_max) {
                loc_max = loc;
            }
        }
        loc = loc1 + loc_max;
        for (uint32_t i = 0; i < v.size(); ++i) {
            exclude.erase(v[i]);
        }
    }
};

#define INTERNAL_ERROR \
    throwError(node, "Internal error at %s:%d", __FILE__, __LINE__);

void throwError(const mathvm::AstNode* node, const char* format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    vasprintf(&buf, format, args);
    mathvm::Status s(buf, node->position());
    free(buf);
    throw s;
}

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

Translator::Translator(mathvm::Code* p): prog(p), currentVar(0),
    overflow(false), currentType(mathvm::VT_INVALID), resultType(mathvm::VT_VOID) { }

void Translator::put(const void* buf_, unsigned int size) {
    const uint8_t* buf = (const uint8_t*)buf_;
    while (size--) {
        code->add(*buf++);
    }
}

template<class T>
void Translator::putVar(mathvm::Instruction ins, const T* node) {
    putVar(ins, node->var()->name(), node);
}

void Translator::putVar(mathvm::Instruction ins, const std::string& name, const mathvm::AstNode* node) {
    code->add(ins);
    const std::vector<VarInt>& v = vars[name];
    if (v.empty()) {
        throwError(node, "Undeclared variable: %s", name.c_str());
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
    putVar(mathvm::BC_LOADIVAR, node);
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
    putVar(mathvm::BC_STOREIVAR, node);
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
    putVar(mathvm::BC_STOREIVAR, node);
    Label start(code), end(code);
    code->bind(start());
    checkTypeInt(in->right());
    putVar(mathvm::BC_LOADIVAR, node);
    code->addBranch(mathvm::BC_IFICMPG, end());
    node->body()->visit(this);
    currentType = mathvm::VT_INVALID;
    putVar(mathvm::BC_LOADIVAR, node);
    code->add(mathvm::BC_ILOAD1);
    code->add(mathvm::BC_IADD);
    putVar(mathvm::BC_STOREIVAR, node);
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

Translator::VarInt Translator::addVar(mathvm::AstNode* node, const std::string& name) {
    VarInt r = currentVar++;
    if (overflow) {
        throwError(node, "I'm sorry, Dave. I'm afraid you exceeded the limit of"
            " variables which is %d.", (unsigned long long)1 + VarInt(-1));
    }
    if (!currentVar) {
        overflow = true;
    }
    vars[name].push_back(r);
    return r;
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
        mathvm::AstFunction* fun = it.next();
        BytecodeFunction* bfun = new BytecodeFunction(fun);
        FreeVarsVisitor(bfun, fun);
        prog->addFunction(bfun);
        Translator trans(prog);
        trans.code = bfun->bytecode();
        trans.resultType = fun->returnType();
        if (trans.resultType != mathvm::VT_VOID) {
            ++trans.currentVar;
        }
        for (BytecodeFunction::iterator it = bfun->vars().begin(); it != bfun->vars().end(); ++it) {
            trans.addVar(node, it->var);
        }
        for (uint32_t i = 0; i < fun->parametersNumber(); ++i) {
            trans.addVar(node, fun->parameterName(i));
        }
        fun->node()->body()->visit(&trans);
    }
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        mathvm::CallNode* callNode = dynamic_cast<mathvm::CallNode*>(node->nodeAt(i));
        node->nodeAt(i)->visit(this);
        if (callNode && prog->functionByName(callNode->name())->returnType() != mathvm::VT_VOID) {
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
    BytecodeFunction* fun = static_cast<BytecodeFunction*>(prog->functionByName(node->name()));
    if (!fun) {
        throwError(node, "Undeclared function: %s", node->name().c_str());
    }
    if (fun->parametersNumber() != node->parametersNumber()) {
        throwError(node, "Function %s expects %d arguments but got %d",
            node->name().c_str(), fun->parametersNumber(), node->parametersNumber());
    }
    if (fun->returnType() != mathvm::VT_VOID) {
        code->add(mathvm::BC_ILOAD0);
    }
    std::vector<BytecodeFunction::iterator> v;
    for (BytecodeFunction::iterator it = fun->vars().begin(); it != fun->vars().end(); ++it) {
        putVar(mathvm::BC_LOADIVAR, it->var, it->node);
        v.push_back(it);
    }
    for (uint32_t i = 0; i < fun->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        if (currentType != fun->parameterType(i)) {
            if (currentType == mathvm::VT_INT && fun->parameterType(i) == mathvm::VT_DOUBLE) {
                code->add(mathvm::BC_I2D);
            } else {
                throwError(node, "Expected expression of type %s "
                    "for argument %d to function %s but %s has type %s",
                    typeToName(fun->parameterType(i)), i, node->name().c_str(),
                    showExpr(node->parameterAt(i)).c_str(), typeToName(currentType));
            }
        }
    }
    code->add(mathvm::BC_CALL);
    code->addTyped(fun->id());
    currentType = fun->returnType();
    for (int32_t i = v.size() - 1; i >= 0; --i) {
        putVar(mathvm::BC_STOREIVAR, v[i]->var, v[i]->node);
    }
}

mathvm::Status Translator::translate(mathvm::AstFunction* fun) {
    try {
        BytecodeFunction* main = new BytecodeFunction(fun);
        FreeVarsVisitor(main, fun);
        prog->addFunction(main);
        code = main->bytecode();
        fun->node()->body()->visit(this);
        code->add(mathvm::BC_STOP);
    } catch (const mathvm::Status& e) {
        return e;
    }
    return mathvm::Status();
}
