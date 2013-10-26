#include "Translator.h"

#include "CodeImpl.h"

#include <string>
#include <vector>
#include <map>
using std::string;
using std::vector;
using std::map;
using std::pair;
using std::make_pair;

#include <stdexcept>

namespace mathvm {

const uint16_t MAX_INDEX = 65535;

class TranslatorVisitor : AstVisitor {
    #define GET_INSN_IDS(PREFIX, SUFFIX, TYPE)          \
        (TYPE == VT_INT? BC_##PREFIX##I##SUFFIX :       \
        (TYPE == VT_DOUBLE? BC_##PREFIX##D##SUFFIX :    \
        (TYPE == VT_STRING? BC_##PREFIX##S##SUFFIX :    \
        throw logic_error("GET_INSN_IDS") )))

    #define GET_INSN_ID(PREFIX, SUFFIX, TYPE)           \
        (TYPE == VT_INT? BC_##PREFIX##I##SUFFIX :       \
        (TYPE == VT_DOUBLE? BC_##PREFIX##D##SUFFIX :    \
        throw logic_error("GET_INSN_ID") ))

    #define GET_INSN(INSN)                              \
        BC_##INSN

    #define ADD_INSN_IDS(PREFIX, SUFFIX, TYPE)          \
        bc()->addInsn(GET_INSN_IDS(PREFIX, SUFFIX, TYPE))

    #define ADD_INSN_ID(PREFIX, SUFFIX, TYPE)           \
        bc()->addInsn(GET_INSN_ID(PREFIX, SUFFIX, TYPE))

    #define ADD_INSN(INSN)                              \
        bc()->addInsn(GET_INSN(INSN))

    #define ADD_U16(INT)                                \
        bc()->addInt16(INT)

    #define ADD_U64(INT)                                \
        bc()->addInt64(INT)

    #define ADD_DOUBLE(DOUBLE)                          \
        bc()->addDouble(DOUBLE)

    #define ADD_BRANCH(TYPE, LABEL)                     \
        bc()->addBranch(BC_IFICMP##TYPE, LABEL)

    #define ADD_BRANCH_JA(LABEL)                        \
        bc()->addBranch(BC_JA, LABEL)

    #define BIND(LABEL)                                 \
        bc()->bind(LABEL)

    #define VISIT(NODE)                                 \
        NODE->visit(this)

    #define LOAD_VAR(V)                                 \
        ADD_INSN_IDS(LOADCTX, VAR, V.type);             \
        ADD_U16(V.fun);                                 \
        ADD_U16(V.num)

    #define STORE_VAR(V)                                \
        ADD_INSN_IDS(STORECTX, VAR, V.type);            \
        ADD_U16(V.fun);                                 \
        ADD_U16(V.num)

    struct Var {
        Var(uint16_t fun, uint16_t num, VarType const & type)
            : fun(fun), num(num), type(type) {}

        uint16_t fun;
        uint16_t num;
        VarType const & type;
    };

    struct Fun {
        Fun(uint16_t id, Signature const & signature)
            : id(id), signature(signature) {}

        uint16_t id;
        Signature const & signature;
    };

    struct FunScope {
        uint16_t id;

        vector<VarType> stack;
        map<string, Fun> funs;
        FunScope * parent;
        uint16_t vars_count;

        FunScope(FunScope * parent, uint16_t id)
            : id(id), parent(parent), vars_count(0) {}

        Fun findFun(string const & name) {
            map<string, Fun>::iterator it;
            it = funs.find(name);

            if ( it != funs.end() ) {
                return it->second;
            }

            if ( parent != NULL) {
                return parent->findFun(name);
            }

            throw logic_error("Fun not found: " + name);
        }

        void addFun(uint16_t id, string const & name, Signature const & signature) {
            funs.insert(make_pair(name, Fun(id, signature)));
        }

        void addVar() {
            if (vars_count == MAX_INDEX) {
                throw logic_error("Too much local variables");
            }
            vars_count++;
        }
    };

    struct VarScope {
        map<string, Var> vars;
        FunScope * fun;
        VarScope * parent;

        VarScope(FunScope * fun, VarScope * parent)
            : fun(fun), parent(parent) {}

        Var findVar(string const & name) {
            map<string, Var>::iterator it;
            it = vars.find(name);

            if ( it != vars.end() ) {
                return it->second;
            }

            if ( parent != NULL) {
                return parent->findVar(name);
            }

            throw logic_error("Var not found: " + name);
        }

        uint16_t addVar(string const & name, VarType const & type) {
            if (vars.size() == MAX_INDEX) {
                throw logic_error("Too much local variables");
            }
            vars.insert(make_pair(name, Var(fun->id, vars.size(), type)));
            fun->addVar();
            return vars.size() - 1;
        }
    };

    Code * code;
    FunctionNode * root;

    BytecodeFunction * result;
    VarScope * var_scope;
    FunScope * fun_scope;

public:
    TranslatorVisitor(Code * code, FunctionNode * root)
        : code(code), root(root) {}

    virtual ~TranslatorVisitor() {
        delete fun_scope;
        delete var_scope;
    }

    BytecodeFunction * run(FunScope * fun_parent = NULL, VarScope * var_parent = NULL) {
        AstFunction dummy(root, NULL);
        result = new BytecodeFunction(&dummy); // god damn constructor

        uint16_t id = code->addFunction(result);
        result->assignId(id);

        fun_scope = new FunScope(fun_parent, id);
        var_scope = new VarScope(fun_scope, var_parent);

        BlockNode * node = root->body();
        initVarScope(node->scope());
        updateFunScope(node->scope());

        for (uint16_t i = 0; i < root->parametersNumber(); ++i) {
            Var var = var_scope->findVar(root->parameterName(i));
            assertSame(var.type, root->parameterType(i));
              STORE_VAR(var);
        }

        for (uint16_t i = 0; i < node->nodes(); ++i) {
              VISIT(node->nodeAt(i));
        }

        VarScope * var_old = var_scope;
        var_scope = var_scope->parent;
        delete var_old;

        return result;
    }

    virtual void visitBinaryOpNode(BinaryOpNode * node) {
        Label yes(bc());
        Label end(bc());

        VISIT(node->right());
        VISIT(node->left());

        assertSame(top(0), top(1));
        assertArithmetic(top());

        VarType type = top();
        pop();
        pop();

        switch(node->kind()) {
            case tOR:       //"||"
                assertInt(type);
                  ADD_INSN(IAOR);
                push(VT_INT);
                return;
            case tAND:      //"&&"
                assertInt(type);
                  ADD_INSN(IMUL);
                push(VT_INT);
                return;
            case tAAND:     //"&"
                assertInt(type);
                  ADD_INSN(IAAND);
                push(VT_INT);
                return;
            case tAOR:      //"|"
                assertInt(type);
                  ADD_INSN(IAOR);
                push(VT_INT);
                return;
            case tAXOR:     //"^"
                assertInt(type);
                  ADD_INSN(IAXOR);
                push(VT_INT);
                return;
            case tEQ:       //"=="
                  ADD_INSN_ID(, CMP, type);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH(E, yes);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH_JA(end);
                BIND(yes);
                  ADD_INSN(ILOAD1);
                BIND(end);
                push(VT_INT);
                return;
            case tNEQ:      //"!="
                  ADD_INSN_ID(, CMP, type);
                push(VT_INT);
                return;
            case tGT:       //">"
                  ADD_INSN_ID(, CMP, type);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH(L, yes);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH_JA(end);
                BIND(yes);
                  ADD_INSN(ILOAD1);
                BIND(end);
                push(VT_INT);
                return;
            case tGE:       //">="
                  ADD_INSN_ID(, CMP, type);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH(LE, yes);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH_JA(end);
                BIND(yes);
                  ADD_INSN(ILOAD1);
                BIND(end);
                push(VT_INT);
                return;
            case tLT:       //"<"
                  ADD_INSN_ID(, CMP, type);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH(G, yes);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH_JA(end);
                BIND(yes);
                  ADD_INSN(ILOAD1);
                BIND(end);
                push(VT_INT);
                return;
            case tLE:       //"<="
                  ADD_INSN_ID(, CMP, type);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH(GE, yes);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH_JA(end);
                BIND(yes);
                  ADD_INSN(ILOAD1);
                BIND(end);
                push(VT_INT);
                return;
            case tADD:      //"+"
                  ADD_INSN_ID(, ADD, type);
                push(type);
                return;
            case tSUB:      //"-"
                  ADD_INSN_ID(, SUB, type);
                push(type);
                return;
            case tMUL:      //"*"
                  ADD_INSN_ID(, MUL, type);
                push(type);
                return;
            case tDIV:      //"/"
                  ADD_INSN_ID(, DIV, type);
                push(type);
                return;
            case tMOD:      //"%"
                assertInt(type);
                  ADD_INSN(IMOD);
                push(type);
                return;
            default:
                throw logic_error("BinaryOp: unknown kind");
        }

        throw logic_error("BinaryOp: illegal state");
    }

    virtual void visitUnaryOpNode(UnaryOpNode * node) {
        node->operand()->visit(this);

        Label yes(bc());
        Label end(bc());

        VarType type = top();
        pop();

        switch (node->kind()) {
            case tNOT:  // "!"
                assertInt(type);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH(E, yes);
                  ADD_INSN(ILOAD0);
                  ADD_BRANCH_JA(end);
                BIND(yes);
                  ADD_INSN(ILOAD1);
                BIND(end);
                push(VT_INT);
                return;
            case tSUB:  // "-"
                assertArithmetic(type);
                  ADD_INSN_ID(, NEG, type);
                push(type);
                return;
            default:
                throw logic_error("UnaryOp: unknown kind");
        }

        throw logic_error("UnaryOp: illegal state");
    }

    virtual void visitStringLiteralNode(StringLiteralNode * node) {
          ADD_INSN(SLOAD);
          ADD_U16(code->makeStringConstant(node->literal()));
        push(VT_STRING);
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode * node) {
          ADD_INSN(DLOAD);
          ADD_DOUBLE(node->literal());
        push(VT_DOUBLE);
    }

    virtual void visitIntLiteralNode(IntLiteralNode * node) {
          ADD_INSN(ILOAD);
          ADD_U64(node->literal());
        push(VT_INT);
    }

    virtual void visitLoadNode(LoadNode * node) {
        AstVar const * avar = node->var();
        Var var = var_scope->findVar(avar->name());
        assertSame(var.type, avar->type());

          LOAD_VAR(var);
        push(var.type);
    }

    virtual void visitStoreNode(StoreNode * node) {
        AstVar const * avar = node->var();
        Var var = var_scope->findVar(avar->name());
        assertSame(var.type, avar->type());

          VISIT(node->value());
          STORE_VAR(var);
        pop(var.type);
    }

    virtual void visitIfNode(IfNode * node) {
        Label els(bc());
        Label end(bc());

        if (!node->elseBlock()) {
              VISIT(node->ifExpr());
            assertInt(pop());
              ADD_INSN(ILOAD0);
              ADD_BRANCH(E, end);
              VISIT(node->thenBlock());
            BIND(end);
        } else {
              VISIT(node->ifExpr());
            assertInt(pop());
              ADD_INSN(ILOAD0);
              ADD_BRANCH(E, els);
              VISIT(node->thenBlock());
              ADD_BRANCH_JA(end);
            BIND(els);
              VISIT(node->elseBlock());
            BIND(end);
        }
    }

    virtual void visitWhileNode(WhileNode * node) {
        Label begin(bc());
        Label end(bc());

        BIND(begin);
          VISIT(node->whileExpr());
        assertInt(pop());
          ADD_INSN(ILOAD0);
          ADD_BRANCH(E, end);
          VISIT(node->loopBlock());
          ADD_BRANCH_JA(begin);

        BIND(end);
    }

    void initVarScope(Scope * ascope) {
        VarScope * scope = new VarScope(fun_scope, var_scope);
        Scope::VarIterator vit(ascope);

        while(vit.hasNext()) {
            AstVar * var = vit.next();
            scope->addVar(var->name(), var->type());
        }

        var_scope = scope;
    }

    void updateFunScope(Scope * ascope) {
        Scope::FunctionIterator fit(ascope);

        while(fit.hasNext()) {
            AstFunction * fun = fit.next();
            VISIT(fun->node());
        }
    }

    virtual void visitBlockNode(BlockNode * node) {
        initVarScope(node->scope());
        updateFunScope(node->scope());

        for (uint16_t i = 0; i < node->nodes(); ++i) {
              VISIT(node->nodeAt(i));
        }

        VarScope * var_old = var_scope;
        var_scope = var_scope->parent;
        delete var_old;
    }

    virtual void visitForNode(ForNode * fnode) {
        BlockNode * node = fnode->body();
        Label begin(bc());
        Label end(bc());

        initVarScope(node->scope());
        updateFunScope(node->scope());

        AstVar const * avar = fnode->var();
        assertInt(avar->type());
        BinaryOpNode * expr = fnode->inExpr()->asBinaryOpNode();
        if (expr == NULL || expr->kind() != tRANGE) {
            throw logic_error("Bad For loop expression");
        }

        Var var = var_scope->findVar(avar->name());
        assertSame(var.type, avar->type());
        assertInt(var.type);

          VISIT(expr->left());
          STORE_VAR(var);
        pop(var.type);
        BIND(begin);
          VISIT(expr->right());
          LOAD_VAR(var);
        pop(var.type);
          ADD_BRANCH(G, end);

        for (uint16_t i = 0; i < node->nodes(); ++i) {
              VISIT(node->nodeAt(i));
        }

          LOAD_VAR(var);
          ADD_INSN(ILOAD1);
          ADD_INSN_ID(, ADD, VT_INT);
          STORE_VAR(var);

          ADD_BRANCH_JA(begin);
        BIND(end);

        VarScope * var_old = var_scope;
        var_scope = var_scope->parent;
        delete var_old;
    }

    virtual void visitFunctionNode(FunctionNode * node) { // TODO
        TranslatorVisitor visitor(code, node);
        BytecodeFunction * result = visitor.run(fun_scope, var_scope);

        fun_scope->addFun(result->id(), result->name(), result->signature());
    }

    virtual void visitReturnNode(ReturnNode * node) {
        if (node->returnExpr()) {
              VISIT(node->returnExpr());
            pop(root->signature()[0].first);
        }
          ADD_INSN(RETURN);
        assertEmptyStack();

        VarScope * var_old = var_scope;
        FunScope * fun_old = fun_scope;
        var_scope = var_scope->parent;
        fun_scope = fun_scope->parent;
        delete var_old;
        delete fun_old;
    }

    virtual void visitCallNode(CallNode * node) {
        Fun fun = fun_scope->findFun(node->name());

        if (fun.signature.size() != 1 + node->parametersNumber()) {
            throw logic_error("Call: invalid signature");
        }

        for (uint16_t i = 0; i < node->parametersNumber(); ++i) {
              VISIT(node->parameterAt(i));
            assertSame(top(), fun.signature[i+1].first);
            pop(); // eated by called function
        }

          ADD_INSN(CALL);
          ADD_U16(fun.id);
    }

    virtual void visitNativeCallNode(NativeCallNode * node) { // TODO LATER

    }

    virtual void visitPrintNode(PrintNode * node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
              VISIT(node->operandAt(i));
              ADD_INSN_IDS(, PRINT, pop());
        }
    }

private: // --------------------------------------------- //

    Bytecode * bc() {
        return result->bytecode();
    }

    VarType pop() {
        if (fun_scope->stack.size() == 0) {
            throw logic_error("Stack underflow");
        }
        VarType ret = fun_scope->stack.back();
        fun_scope->stack.pop_back();
        return ret;
    }

    VarType pop(VarType type) {
        assertSame(type, top());
        return pop();
    }

    void push(VarType type) {
        fun_scope->stack.push_back(type);
    }

    VarType top() {
        return fun_scope->stack.back();
    }

    VarType top(uint32_t depth) {
        uint32_t index = fun_scope->stack.size() - depth - 1;
        if (index < 0) {
            throw logic_error("Stack underflow");
        }
        return fun_scope->stack[index];
    }

    void assertEmptyStack() {
        if (fun_scope->stack.size() > 0) {
            throw logic_error("Stack overflow");
        }
    }

    bool isIntType(VarType type) {
        return type == VT_INT;
    }

    bool isArithmeticType(VarType type) {
        return type == VT_INT || type == VT_DOUBLE;
    }

    void assertInt(VarType type) {
        if (type != VT_INT) {
            throw logic_error("assertInt");
        }
    }

    void assertArithmetic(VarType type) {
        if (type != VT_INT && type != VT_DOUBLE) {
            throw logic_error("assertArithmetic");
        }
    }

    void assertInvalid(VarType type) {
        if (type != VT_INVALID) {
            throw logic_error("assertInvalid");
        }
    }

    void assertSame(VarType left, VarType right) {
        if (left != right) {
            throw logic_error("assertSame");
        }
    }

    #undef GET_INSN_IDS
    #undef GET_INSN_ID
    #undef GET_INSN
    #undef ADD_INSN_IDS
    #undef ADD_INSN_ID
    #undef ADD_INSN
    #undef ADD_BRANCH
    #undef ADD_BRANCH_JA
    #undef BIND
    #undef VISIT
};

Status* BytecodeTranslator::translate(string const & program, Code ** code) {
    Parser parser;
    Status * status = parser.parseProgram(program);
    if (status && status->isError()) {
        return status;
    } else {
        *code = new CodeImpl();
        AstFunction * root = parser.top();
        TranslatorVisitor visitor(*code, root->node());
        visitor.run();
        return NULL;
    }

}

}