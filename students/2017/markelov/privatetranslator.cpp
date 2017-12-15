#include "translator.h"
#include "interpret.h"
#include "parser.h"

using namespace std;

namespace mathvm {

class PrivateTranslator: public AstVisitor {
    ScopedCode * code;
    std::vector<std::tuple<VarType, CodeScope*>> fstack;
    uint16_t scopeLast = 0;

    std::tuple<VarType, CodeScope*> CALL_VISIT(AstNode * node, CodeScope * scope) {
        fstack.push_back(std::make_tuple(VT_INVALID, scope));
        if (node != nullptr)
            node->visit(this);
        else
            get<0>(fstack.back()) = VT_VOID;
        std::tuple<VarType, CodeScope*> ret = fstack.back();
        assert(std::get<0>(ret) != VT_INVALID);
        fstack.pop_back();
        return ret;
    }

    void VISIT_RETURN(VarType rettype) {
        get<0>(fstack.back()) = (rettype);
    }

    void VISIT_CHANGE_CURRENT_SCOPE(CodeScope * newscope) {
        get<1>(fstack.back()) = newscope;
    }

    Bytecode * current_bytecode() {
        return current_function()->bytecode();
    }

    BytecodeFunction * current_function() {
        return current_scope()->f;
    }

    CodeScope* current_scope() {
        if (fstack.empty())
            return nullptr;
        return get<1>(fstack.back());
    }

    CodeScope * createScope(Scope * newscope) {
        return new CodeScope(code, current_scope(), newscope, scopeLast++);
    }
    CodeScope * createFunctionScope(AstFunction * astf) {
        return new CodeScope(code, current_scope(), astf->scope(), scopeLast++,
                astf);
    }

public:
    PrivateTranslator(ScopedCode * code, AstFunction * top) :
            code(code) {
        CodeScope * scope = addAstFunction(top);
        scope->isGlobal = 1;
        CALL_VISIT(top->node(), scope);
    }

//=============

    CodeScope * addAstFunction(AstFunction * top) {
        CodeScope * scope = createFunctionScope(top);
        code->addFunction(scope->f);

        return scope;
    }

    void visitFunctionNode(FunctionNode* node) {
        AstFunction * top = current_scope()->astf;
        if (top->top_name != top->name()) {
            for (uint32_t i = 0; i < top->parametersNumber(); i++) {
                const string& pname = top->parameterName(i);
                VarType ptype = top->parameterType(i);

                std::tuple<uint16_t, uint16_t> varid =
                        current_scope()->outerVarId(pname);
                switch (ptype) {
                case VT_INT:
                    current_bytecode()->addInsn(BC_STORECTXIVAR);
                    break;
                case VT_STRING:
                    current_bytecode()->addInsn(BC_STORECTXSVAR);
                    break;
                case VT_DOUBLE:
                    current_bytecode()->addInsn(BC_STORECTXDVAR);
                    break;
                default:
                    break;
                }
                current_bytecode()->addInt16(std::get<0>(varid));
                current_bytecode()->addInt16(std::get<1>(varid));
            }
        }

        CALL_VISIT(node->body(), current_scope());
        VISIT_RETURN(VT_VOID);
    }

    void visitBlockNode(BlockNode* node) {

        CodeScope * newScope = createScope(node->scope());
        VISIT_CHANGE_CURRENT_SCOPE(newScope);

        Scope::FunctionIterator it(newScope->legacy_scope);
        std::vector<std::tuple<FunctionNode*, CodeScope *> > fscopes;
        while (it.hasNext()) {
            AstFunction* f = it.next();

            fscopes.push_back(std::make_tuple(f->node(), addAstFunction(f)));
        }

        for (auto fscope : fscopes) {
            CALL_VISIT(std::get<0>(fscope), std::get<1>(fscope));
        }

        newScope->sbci = current_bytecode()->length();
        for (uint32_t i = 0; i < node->nodes(); i++) {
            VarType ret = std::get<0>(CALL_VISIT(node->nodeAt(i), newScope));
            if (ret != VT_VOID) {
                /* remove unessary value from stack */
                int npop = (ret == VT_STRING ? 1 : 4);
                for (int j = 0; j < npop; j++) {
                    current_bytecode()->addInsn(BC_POP);
                }
            }
        }
        newScope->ebi = current_bytecode()->length();

        VISIT_RETURN(VT_VOID);
    }

    void makeNot(Bytecode * code) {
        code->addInsn(BC_ILOAD0);
        Label tmp, tmp2;
        code->addBranch(BC_IFICMPE, tmp);
        code->addInsn(BC_ILOAD0);
        code->addBranch(BC_JA, tmp2);
        tmp.bind(code->length(), code);
        code->addInsn(BC_ILOAD1);
        tmp2.bind(code->length(), code);
    }

    void docast(VarType from, VarType to) {
        if (from == VT_INT && to == VT_DOUBLE)
            current_bytecode()->addInsn(BC_I2D);
        if (from == VT_DOUBLE && to == VT_INT)
            current_bytecode()->addInsn(BC_D2I);
        if (from == VT_STRING && to == VT_INT)
            current_bytecode()->addInsn(BC_S2I);
        if (from == VT_STRING && to == VT_DOUBLE) {
            current_bytecode()->addInsn(BC_S2I);
            current_bytecode()->addInsn(BC_I2D);
        }
    }

    VarType castTargetMax(VarType a, VarType b) {
        if (a == VT_DOUBLE || b == VT_DOUBLE)
            return VT_DOUBLE;
        if (a == VT_INT || b == VT_INT)
            return VT_INT;
        return VT_STRING;
    }

    void visitBinaryOpNode(BinaryOpNode* node) {
        VarType tr = std::get<0>(CALL_VISIT(node->right(), current_scope()));
        VarType tl = std::get<0>(CALL_VISIT(node->left(), current_scope()));
        Bytecode * bcode = current_bytecode();

        VarType t = VT_INVALID;
        t = castTargetMax(tl, tr);

        if (tl != t) {
            docast(tl, t);
            tl = t;
        }
        if (tr != t) {
            switch (tl) {
            case VT_STRING:
                bcode->addInsn(BC_STORESVAR1);
                docast(tr, t);
                bcode->addInsn(BC_LOADSVAR1);
                break;
            case VT_INT:
                bcode->addInsn(BC_STOREIVAR1);
                docast(tr, t);
                bcode->addInsn(BC_LOADIVAR1);
                break;
            case VT_DOUBLE:
                bcode->addInsn(BC_STOREDVAR1);
                docast(tr, t);
                bcode->addInsn(BC_LOADDVAR1);
                break;
            default:
                break;
            }

        }

        switch (node->kind()) {
        case tAOR:
            // TODO SCE
            bcode->addInsn(BC_IAOR);
            t = VT_INT;
            break;
        case tAAND:
            // TODO SCE
            bcode->addInsn(BC_IAAND);
            t = VT_INT;
            break;
        case tAXOR:
            bcode->addInsn(BC_IAXOR);
            t = VT_INT;
            break;
        case tADD:
            if (t == VT_INT)
                bcode->addInsn(BC_IADD);
            else
                bcode->addInsn(BC_DADD);
            break;
        case tSUB:
            if (t == VT_INT)
                bcode->addInsn(BC_ISUB);
            else
                bcode->addInsn(BC_DSUB);
            break;
        case tDIV:
            if (t == VT_INT)
                bcode->addInsn(BC_IDIV);
            else
                bcode->addInsn(BC_DDIV);
            break;
        case tMUL:
            if (t == VT_INT)
                bcode->addInsn(BC_IMUL);
            else
                bcode->addInsn(BC_DMUL);
            break;
        case tMOD:
            bcode->addInsn(BC_IMOD);
            t = VT_INT;
            break;
        case tEQ:
            if (tl == VT_INT)
                bcode->addInsn(BC_ICMP);
            else
                bcode->addInsn(BC_DCMP);
            makeNot(bcode);
            t = VT_INT;
            break;
        case tNEQ:
            if (tl == VT_INT)
                bcode->addInsn(BC_ICMP);
            else
                bcode->addInsn(BC_DCMP);
            t = VT_INT;
            break;
        case tGT:
            if (tl == VT_INT)
                bcode->addInsn(BC_ICMP);
            else
                bcode->addInsn(BC_DCMP);
            bcode->addInsn(BC_ILOAD1);
            bcode->addInsn(BC_ICMP);
            makeNot(bcode);
            t = VT_INT;
            break;
        case tGE:
            if (tl == VT_INT)
                bcode->addInsn(BC_ICMP);
            else
                bcode->addInsn(BC_DCMP);
            bcode->addInsn(BC_ILOADM1);
            bcode->addInsn(BC_ICMP);
            t = VT_INT;
            break;
        case tLT:
            if (tl == VT_INT)
                bcode->addInsn(BC_ICMP);
            else
                bcode->addInsn(BC_DCMP);
            bcode->addInsn(BC_ILOADM1);
            bcode->addInsn(BC_ICMP);
            makeNot(bcode);
            t = VT_INT;
            break;
        case tLE:
            if (tl == VT_INT)
                bcode->addInsn(BC_ICMP);
            else
                bcode->addInsn(BC_DCMP);
            bcode->addInsn(BC_ILOAD1);
            bcode->addInsn(BC_ICMP);
            t = VT_INT;
            break;
        case tAND:
            makeNot(bcode);
            makeNot(bcode);
            bcode->addInsn(BC_STOREIVAR0);
            makeNot(bcode);
            makeNot(bcode);
            bcode->addInsn(BC_LOADIVAR0);
            bcode->addInsn(BC_IAAND);
            break;
        case tOR:
            makeNot(bcode);
            makeNot(bcode);
            bcode->addInsn(BC_STOREIVAR0);
            makeNot(bcode);
            makeNot(bcode);
            bcode->addInsn(BC_LOADIVAR0);
            bcode->addInsn(BC_IAOR);
            break;
        default:
            break;
        }
        VISIT_RETURN(t);
    }
    void __loadVar(const AstVar * var) {
        Bytecode * bcode = current_bytecode();
        switch (var->type()) {
        case VT_INT:
            bcode->addInsn(BC_LOADCTXIVAR);
            break;
        case VT_STRING:
            bcode->addInsn(BC_LOADCTXSVAR);
            break;
        case VT_DOUBLE:
            bcode->addInsn(BC_LOADCTXDVAR);
            break;
        default:
            break;
        }
        std::tuple<uint16_t, uint16_t> id = current_scope()->outerVarId(
                var->name());
        bcode->addInt16(std::get<0>(id));
        bcode->addInt16(std::get<1>(id));
    }
    void visitLoadNode(LoadNode* node) {
        const AstVar * var = node->var();
        __loadVar(var);
        VISIT_RETURN(var->type());
    }
    void visitStoreNode(StoreNode* node) {
        Bytecode * bcode = current_bytecode();
        const AstVar * var = node->var();
        CALL_VISIT(node->value(), current_scope());

        switch (var->type()) {
        case VT_INT:
            if (node->op() == tINCRSET) {
                __loadVar(var);
                bcode->addInsn(BC_IADD);
            } else if (node->op() == tDECRSET) {
                __loadVar(var);
                bcode->addInsn(BC_ISUB);
            }
            bcode->addInsn(BC_STORECTXIVAR);
            break;
        case VT_STRING:
            bcode->addInsn(BC_STORECTXSVAR);
            break;
        case VT_DOUBLE:
            if (node->op() == tINCRSET) {
                __loadVar(var);
                bcode->addInsn(BC_DADD);
            } else if (node->op() == tDECRSET) {
                __loadVar(var);
                bcode->addInsn(BC_DSUB);
            }
            bcode->addInsn(BC_STORECTXDVAR);
            break;
        default:
            break;
        }
        std::tuple<uint16_t, uint16_t> id = current_scope()->outerVarId(
                var->name());
        bcode->addInt16(std::get<0>(id));
        bcode->addInt16(std::get<1>(id));

        VISIT_RETURN(VT_VOID);
    }

    void visitPrintNode(PrintNode* node) {
        uint32_t csz = node->operands();
        std::vector<VarType> operands_types;
        for (int64_t i = csz - 1; i >= 0; i--) {
            operands_types.push_back(
                    std::get<0>(
                            CALL_VISIT(node->operandAt(i), current_scope())));
        }
        std::reverse(operands_types.begin(), operands_types.end());

        Bytecode * bcode = current_bytecode();
        for (VarType operand : operands_types) {
            switch (operand) {
            case VT_STRING:
                bcode->addInsn(BC_SPRINT);
                break;
            case VT_INT:
                bcode->addInsn(BC_IPRINT);
                break;
            case VT_DOUBLE:
                bcode->addInsn(BC_DPRINT);
                break;
            default:
                break;
            }
        }

        VISIT_RETURN(VT_VOID);
    }
    void visitStringLiteralNode(StringLiteralNode* node) {
        Bytecode * bcode = current_bytecode();
        bcode->addInsn(BC_SLOAD);
        uint16_t str_constant = code->makeStringConstant(node->literal());
        bcode->addInt16(str_constant);

        VISIT_RETURN(VT_STRING);
    }

    void visitIntLiteralNode(IntLiteralNode* node) {
        Bytecode * code = current_bytecode();
        code->addInsn(BC_ILOAD);
        code->addInt64(node->literal());

        VISIT_RETURN(VT_INT);
    }
    void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        Bytecode * code = current_bytecode();
        code->addInsn(BC_DLOAD);
        code->addDouble(node->literal());

        VISIT_RETURN(VT_DOUBLE);
    }

    void visitUnaryOpNode(UnaryOpNode* node) {
        Bytecode * code = current_bytecode();

        VarType t = VT_INVALID;
        t = std::get<0>(CALL_VISIT(node->operand(), current_scope()));
        switch (node->kind()) {
        case tSUB:
            if (t == VT_INT) {
                code->addInsn(BC_INEG);
            } else if (t == VT_DOUBLE) {
                code->addInsn(BC_DNEG);
            }
            break;
        case tNOT:
            makeNot(code);
            break;
        default:
            break;
        }

        VISIT_RETURN(t);
    }

    void visitIfNode(IfNode* node) {
        Bytecode * code = current_bytecode();
        CALL_VISIT(node->ifExpr(), current_scope());

        Label tono, fromendyes;

        code->addInsn(BC_ILOAD0);
        code->addBranch(BC_IFICMPE, tono);
        CALL_VISIT(node->thenBlock(), current_scope());
        code->addBranch(BC_JA, fromendyes);

        if (node->elseBlock()) {
            CodeScope * no = get<1>(
                    CALL_VISIT(node->elseBlock(), current_scope()));
            tono.bind(no->sbci, code);
            fromendyes.bind(no->ebi, code);
        } else {
            tono.bind(code->length(), code);
            fromendyes.bind(code->length(), code);
        }

        VISIT_RETURN(VT_VOID);
    }

    void visitWhileNode(WhileNode* node) {
        Bytecode * code = current_bytecode();
        Label before_cond, tono;
        before_cond.bind(code->length(), code);
        CALL_VISIT(node->whileExpr(), current_scope());
        code->addInsn(BC_ILOAD0);
        code->addBranch(BC_IFICMPE, tono);
        CALL_VISIT(node->loopBlock(), current_scope());
        code->addBranch(BC_JA, before_cond);
        tono.bind(code->length(), code);

        VISIT_RETURN(VT_VOID);
    }

    void visitForNode(ForNode* node) {
        std::tuple<uint16_t, uint16_t> varid = current_scope()->outerVarId(
                node->var()->name());

        BinaryOpNode * expr = dynamic_cast<BinaryOpNode *>(node->inExpr());
        assert(expr);
        CALL_VISIT(expr->left(), current_scope());

        Bytecode * code = current_bytecode();
        code->addInsn(BC_STORECTXIVAR);
        code->addInt16(std::get<0>(varid));
        code->addInt16(std::get<1>(varid));

        Label before_cond, tono;

        before_cond.bind(code->length(), code);
        CALL_VISIT(expr->right(), current_scope());
        code->addInsn(BC_ILOAD1);
        code->addInsn(BC_IADD);

        code->addInsn(BC_LOADCTXIVAR);
        code->addInt16(std::get<0>(varid));
        code->addInt16(std::get<1>(varid));
        code->addBranch(BC_IFICMPE, tono);

        CALL_VISIT(node->body(), current_scope());

        code->addInsn(BC_ILOAD1);
        code->addInsn(BC_LOADCTXIVAR);
        code->addInt16(std::get<0>(varid));
        code->addInt16(std::get<1>(varid));
        code->addInsn(BC_IADD);

        code->addInsn(BC_STORECTXIVAR);
        code->addInt16(std::get<0>(varid));
        code->addInt16(std::get<1>(varid));
        code->addBranch(BC_JA, before_cond);
        tono.bind(code->length(), code);

        VISIT_RETURN(VT_VOID);
    }

    void visitReturnNode(ReturnNode* node) {
        VarType rr = std::get<0>(
                CALL_VISIT(node->returnExpr(), current_scope()));
        VarType sign = current_scope()->f->signature()[0].first;

        Bytecode * code = current_bytecode();

        if (rr != sign) {
            docast(rr, sign);
        }
        code->addInsn(BC_RETURN);

        VISIT_RETURN(VT_VOID);
    }
    void visitCallNode(CallNode * node) {

        TranslatedFunction * f = code->functionByName(node->name());
        for (int32_t i = node->parametersNumber() - 1; i >= 0; i--) {
            VarType rr = std::get<0>(
                    CALL_VISIT(node->parameterAt(i), current_scope()));
            VarType sign = f->signature()[1 + i].first;
            (void) rr;
            (void) sign;
            if (rr != sign)
                docast(rr, sign);
        }

        assert(f != nullptr);
        current_bytecode()->addInsn(BC_CALL);
        current_bytecode()->addInt16(f->id());

        VISIT_RETURN(f->returnType());
    }

    void visitNativeCallNode(NativeCallNode * node) {
        void *f = dlsym(NULL, node->nativeName().c_str());
        if (f == NULL)
            assert(false);


        AstFunction * top = current_scope()->astf;
        for (int32_t i = top->parametersNumber() - 1; i >= 0; i--) {
            const string& pname = top->parameterName(i);
            VarType ptype = top->parameterType(i);

            std::tuple<uint16_t, uint16_t> varid = current_scope()->outerVarId(
                    pname);
            switch (ptype) {
            case VT_INT:
                current_bytecode()->addInsn(BC_LOADCTXIVAR);
                break;
            case VT_STRING:
                current_bytecode()->addInsn(BC_LOADCTXSVAR);
                break;
            case VT_DOUBLE:
                current_bytecode()->addInsn(BC_LOADCTXDVAR);
                break;
            default:
                break;
            }
            current_bytecode()->addInt16(std::get<0>(varid));
            current_bytecode()->addInt16(std::get<1>(varid));
        }

        uint16_t id = code->makeNativeFunction(node->nativeName(),
                node->nativeSignature(), f);

        current_bytecode()->addInsn(BC_CALLNATIVE);
        current_bytecode()->addUInt16(id);

        VISIT_RETURN(VT_VOID);
    }

    ScopedCode * getCode() {
        return code;
    }
};

struct privateParts {
    Parser * parser;
    PrivateTranslator * vis;
    ~privateParts() {
        delete parser;
        delete vis;
    }
};

PublicTranslator::~PublicTranslator() {
    if (priv) {
        privateParts * tr = (privateParts *) priv;
        delete tr;
    }
}

PublicTranslator::PublicTranslator() :
        priv(0) {

}

Status* PublicTranslator::translate(const string& program, Code** code) {
    (*code) = new PassedCode();
    Parser * parser = new Parser();
    Status * parse_stat = parser->parseProgram(program);
    if (parse_stat->isError()) {
        return parse_stat;
    }
    delete parse_stat;

    PrivateTranslator * vis = new PrivateTranslator((ScopedCode*) (*code), parser->top());

    priv = new privateParts();
    ((privateParts *) priv)->parser = parser;
    ((privateParts *) priv)->vis = vis;

    return Status::Ok();
}

Translator* Translator::create(const string& impl) {
    return new PublicTranslator();
}

}

