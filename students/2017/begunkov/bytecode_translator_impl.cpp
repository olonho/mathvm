#include <queue>
#include "code_impl.hpp"
#include "parser.h"

using namespace mathvm;
using namespace std;

template <typename T>
static T poptop(stack<T>& container)
{
    assert(!container.empty());

    T res = container.top();
    container.pop();
    return res;
}

template <typename T>
class mapInc
{
    map<T, uint16_t> mp;
    uint16_t lastNum = 0;

public:
    uint16_t operator[](T idx) {
        auto it = mp.find(idx);
        if (it == mp.end())
            return mp[idx] = lastNum++;
        return it->second;
    }
};

namespace {
class BCVisitor: public AstVisitor
{
public:
    Bytecode *bc = nullptr;
    unique_ptr<InterpreterCodeImpl> code;
    bool discard = true;

    stack<VarType> typestack;
    queue<AstFunction*> foos;
    mapInc<Scope*> scopes;
    mapInc<const AstVar*> vars;

    BCVisitor(): code(new InterpreterCodeImpl) {}

    void process(AstFunction *top)
    {
        addFoo(top);
        while (!foos.empty()) {
            auto foo = foos.front();
            foos.pop();
            TranslatedFunction *trFoo = code->functionByName(foo->name());
            bc = ((BytecodeFunction*)trFoo)->bytecode();

            for (uint32_t i = 0; i < foo->parametersNumber(); ++i) {
                Instruction instr;

                switch (foo->parameterType(i)) {
                    case VT_INT: instr = BC_STORECTXIVAR; break;
                    case VT_DOUBLE: instr = BC_STORECTXDVAR; break;
                    case VT_STRING: instr = BC_STORECTXSVAR; break;
                    default: assert(false);
                }
                auto var = foo->scope()->lookupVariable(foo->parameterName(i));

                bc->addInsn(instr);
                bc->addTyped(scopes[foo->scope()]);
                bc->addTyped(vars[var]);
            }

            typestack.push(foo->returnType());
            foo->node()->visit(this);
        }
    }

    void coercion()
    {
        auto l = poptop(typestack);
        auto r = poptop(typestack);

        if (l != r) {
            if (r == VT_INT) {
                bc->addInsn(BC_SWAP);
                bc->addInsn(BC_I2D);
                bc->addInsn(BC_SWAP);
            } else
                bc->addInsn(BC_I2D);

            l = VT_DOUBLE;
        }

        typestack.push(l);
        typestack.push(l);
    }

    void normalize()
    {
        Label lend(bc), lel(bc);

        bc->addInsn(BC_ILOAD0);
        bc->addBranch(BC_IFICMPE, lel);
        bc->addInsn(BC_ILOAD1);
        bc->addBranch(BC_JA, lend);

        bc->bind(lel);
        bc->addInsn(BC_ILOAD0);
        bc->bind(lend);
    }

    void compare(BinaryOpNode *node)
    {
        Label lend(bc), ltrue(bc);

        coercion();
        bc->addInsn(poptop(typestack) == VT_INT ? BC_ICMP : BC_DCMP);
        bc->addInsn(BC_ILOAD0);
        bc->addInsn(BC_SWAP);

        Instruction i = BC_INVALID;
        switch (node->kind()) {
        case tEQ: i = BC_IFICMPE; break;
        case tNEQ: i = BC_IFICMPNE; break;
        case tLT: i = BC_IFICMPL; break;
        case tGT: i = BC_IFICMPG; break;
        case tLE: i = BC_IFICMPLE; break;
        case tGE: i = BC_IFICMPGE; break;
        default:;
        }
        bc->addBranch(i, ltrue);
        bc->addInsn(BC_ILOAD0);
        bc->addBranch(BC_JA, lend);
        bc->bind(ltrue);
        bc->addInsn(BC_ILOAD1);
        bc->bind(lend);

        typestack.top() = VT_INT;
    }

    void arithmetic(BinaryOpNode *node)
    {
        Instruction i = BC_INVALID;

        coercion();
        if (poptop(typestack) == VT_INT) {
            switch (node->kind()) {
            case tADD: i = BC_IADD; break;
            case tSUB: i = BC_ISUB; break;
            case tMUL: i = BC_IMUL; break;
            case tDIV: i = BC_IDIV; break;
            case tMOD: i = BC_IMOD; break;
            case tAAND: i = BC_IAAND; break;
            case tAOR: i = BC_IAOR; break;
            case tAXOR: i = BC_IAXOR; break;
            default:;
            }
        } else {
            switch (node->kind()) {
            case tADD: i = BC_DADD; break;
            case tSUB: i = BC_DSUB; break;
            case tMUL: i = BC_DMUL; break;
            case tDIV: i = BC_DDIV; break;
            default:;
            }
        }

        assert(i != BC_INVALID);
        bc->addInsn(i);
    }

    void visitBinaryOpNode(BinaryOpNode *node) override
    {
        node->right()->visit(this);
        node->left()->visit(this);

        discard = true;

        switch (node->kind()) {
            case tRANGE:
                return;
            case tEQ:
            case tNEQ:
            case tLT:
            case tLE:
            case tGT:
            case tGE:
                return compare(node);
            case tOR:
            case tAND: {
                typestack.pop();
                normalize();
                bc->addInsn(BC_SWAP);
                normalize();

                switch (node->kind()) {
                    case tOR: bc->addInsn(BC_IAOR); break;
                    case tAND: bc->addInsn(BC_IAAND); break;
                    default:;
                }
                break;
            }
            default:
                return arithmetic(node);
        }
    }

    void visitUnaryOpNode(UnaryOpNode *node) override
    {
        node->visitChildren(this);
        discard = true;

        switch (node->kind()) {
            case tNOT: {
                Label end(bc), el(bc);

                bc->addInsn(BC_ILOAD0);
                bc->addBranch(BC_IFICMPE, el);
                bc->addInsn(BC_ILOAD0);
                bc->addBranch(BC_JA, end);

                bc->bind(el);
                bc->addInsn(BC_ILOAD1);
                bc->bind(end);

                typestack.pop();
                typestack.push(VT_INT);
                break;
            } case tSUB: {
                bc->addInsn(typestack.top() == VT_INT ? BC_INEG : BC_DNEG);
                break;
            }
            default:
                assert(false);
        }
    }


    void visitStringLiteralNode(StringLiteralNode *node) override
    {
        bc->addInsn(BC_SLOAD);
        uint16_t id = code->makeStringConstant(node->literal());
        bc->addTyped(id);
        typestack.push(VT_STRING);

        discard = true;
    }


    void visitDoubleLiteralNode(DoubleLiteralNode *node) override
    {
        bc->addInsn(BC_DLOAD);
        bc->addTyped(node->literal());
        typestack.push(VT_DOUBLE);

        discard = true;
    }


    void visitIntLiteralNode(IntLiteralNode *node) override
    {
        bc->addInsn(BC_ILOAD);
        bc->addTyped(node->literal());
        typestack.push(VT_INT);

        discard = true;
    }


    void visitLoadNode(LoadNode *node) override
    {
        typestack.push(node->var()->type());

        switch (typestack.top()) {
            case VT_INT: bc->addInsn(BC_LOADCTXIVAR); break;
            case VT_DOUBLE: bc->addInsn(BC_LOADCTXDVAR); break;
            case VT_STRING: bc->addInsn(BC_LOADCTXSVAR); break;
            default:;
        }

        auto pVar = node->var();
        auto scope = scopes[pVar->owner()];
        auto var = vars[pVar];
        bc->addTyped(scope);
        bc->addTyped(var);

        discard = false;
    }

    void cast(VarType type)
    {
        auto old = poptop(typestack);

        if (old != type)
            bc->addInsn(old == VT_INT ? BC_I2D : BC_D2I);
        typestack.push(type);
    }

    void visitStoreNode(StoreNode *node) override
    {
        node->value()->visit(this);
        cast(node->var()->type());

        VarType sType = typestack.top();
        uint16_t scopeID = scopes[node->var()->owner()];
        uint16_t varID = vars[node->var()];

        if (node->op() != tASSIGN) {
            bc->addInsn(BC_LOADCTXDVAR);
            bc->addTyped(scopeID);
            bc->addTyped(varID);

            Instruction insn = (node->op() == tINCRSET)
                ? (sType == VT_INT ? BC_IADD : BC_DADD)
                : (sType == VT_INT ? BC_ISUB : BC_DSUB);
            bc->addInsn(insn);
        }

        Instruction insn =  (sType == VT_INT) ? BC_STORECTXIVAR :
                            (sType == VT_DOUBLE) ? BC_STORECTXDVAR :
                                                BC_STORECTXSVAR;

        bc->addInsn(insn);
        bc->addTyped(scopeID);
        bc->addTyped(varID);
        typestack.pop();
        discard = false;
    }

    void doublingVar() {
        bc->addInsn(BC_STOREIVAR0);
        bc->addInsn(BC_LOADIVAR0);
        bc->addInsn(BC_LOADIVAR0);
    }


    void visitForNode(ForNode *node) override
    {
        Label lbody(bc), lcheck(bc), lend(bc);
        uint16_t scope = scopes[node->var()->owner()];
        uint16_t var = vars[node->var()];

        node->inExpr()->visit(this);
        bc->addInsn(BC_STORECTXIVAR);
        bc->addTyped(scope);
        bc->addTyped(var);

        bc->bind(lcheck);
        doublingVar();
        bc->addInsn(BC_LOADCTXIVAR);
        bc->addTyped(scope);
        bc->addTyped(var);
        bc->addBranch(BC_IFICMPG, lend);

        // body
        bc->bind(lbody);
        node->body()->visit(this);

        // iter
        bc->addInsn(BC_LOADCTXIVAR);
        bc->addTyped(scope);
        bc->addTyped(var);
        bc->addInsn(BC_ILOAD1);
        bc->addInsn(BC_IADD);
        bc->addInsn(BC_STORECTXIVAR);
        bc->addTyped(scope);
        bc->addTyped(var);
        bc->addBranch(BC_JA, lcheck);

        // end
        bc->bind(lend);
        bc->addInsn(BC_POP);
        discard = false;
    }


    void visitWhileNode(WhileNode *node) override
    {
        Label lcheck(bc), lend(bc);

        bc->bind(lcheck);
        node->whileExpr()->visit(this);

        bc->addInsn(BC_ILOAD0);
        bc->addBranch(BC_IFICMPE, lend);

        node->loopBlock()->visit(this);
        bc->addBranch(BC_JA, lcheck);
        bc->bind(lend);
        typestack.pop();

        discard = false;
    }


    void visitIfNode(IfNode *node) override
    {
        Label lelse(bc), lend(bc);

        node->ifExpr()->visit(this);
        typestack.pop();
        bc->addInsn(BC_ILOAD0);

        if (node->elseBlock() != nullptr) {
            bc->addBranch(BC_IFICMPE, lelse);
            node->thenBlock()->visit(this);
            bc->addBranch(BC_JA, lend);
            bc->bind(lelse);
            node->elseBlock()->visit(this);
        } else {
            bc->addBranch(BC_IFICMPE, lend);
            node->thenBlock()->visit(this);
        }
        bc->bind(lend);

        discard = false;
    }

    void addFoo(AstFunction *foo) {
        foos.push(foo);
        code->addFunction(new BytecodeFunction(foo));
    }


    void visitBlockNode(BlockNode *node) override
    {
        auto scope = node->scope();
        (void)scopes[scope];

        for (Scope::FunctionIterator it(scope); it.hasNext(); )
            addFoo(it.next());

        for (uint32_t i = 0; i < node->nodes(); ++i) {
            discard = false;

            node->nodeAt(i)->visit(this);
            if (discard) {
                bc->addInsn(BC_POP);
                typestack.pop();
            }
        }
        discard = false;
    }

    void visitFunctionNode(FunctionNode *node) override
    {
        node->visitChildren(this);
    }

    void visitReturnNode(ReturnNode *node) override
    {
        node->visitChildren(this);
        AstNode *ret = node->returnExpr();

        auto type = typestack.top();
        if (ret != nullptr && type != VT_VOID) {
            cast(type);
            typestack.pop();
        }
        bc->addInsn(BC_RETURN);
        discard = false;
    }

    void visitCallNode(CallNode *node) override
    {
        TranslatedFunction *foo = code->functionByName(node->name());

        for (int i = (int)node->parametersNumber() - 1; i >= 0; --i) {
            node->parameterAt(i)->visit(this);
            cast(foo->parameterType(i));
            typestack.pop();
        }

        bc->addInsn(BC_CALL);
        bc->addTyped(foo->id());

        discard = (foo->returnType() != VT_VOID);
        if (foo->returnType() != VT_VOID)
            typestack.push(foo->returnType());
    }

    void visitNativeCallNode(NativeCallNode *foo) override
    {
        // todo:
    }

    void visitPrintNode(PrintNode *node) override
    {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            Instruction instr;

            node->operandAt(i)->visit(this);
            switch (poptop(typestack)) {
                case VT_INT: instr = BC_IPRINT; break;
                case VT_DOUBLE: instr = BC_DPRINT; break;
                case VT_STRING: instr = BC_SPRINT; break;
                default: assert(false);
            }
            bc->addInsn(instr);
        }
        discard = false;
    }
};
} // !namespace ::





Status *BytecodeTranslatorImpl::translate(const string& program, Code ** pCode)
{
    Parser parser;
    Status *ret = parser.parseProgram(program);

    if (!ret->isOk())
        return ret;

    BCVisitor visitor;
//    visitor.init(parser.top()->scope());
    visitor.process(parser.top());

    Scope *scope = parser.top()->node()->body()->scope();
    for (Scope::VarIterator it(scope); it.hasNext(); ) {
        auto var = it.next();
        visitor.code->varNames[var->name()] = visitor.vars[var];
    }

    *pCode = visitor.code.release();
    return ret;
}



