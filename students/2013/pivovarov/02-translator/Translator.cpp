#include "InterpreterCodeImpl.h"

#include "ast.h"
#include "mathvm.h"
#include "visitors.h"
#include "parser.h"

#include <string>
#include <vector>
#include <map>
using std::string;
using std::vector;
using std::map;
using std::pair;
using std::make_pair;

#include <dlfcn.h>

namespace mathvm {

const uint16_t MAX_INDEX = 65535;

class TranslatorVisitor : AstVisitor {
    #define GET_INSN_IDS(PREFIX, SUFFIX, TYPE)                      \
        (TYPE == VT_INT? BC_##PREFIX##I##SUFFIX :                   \
        (TYPE == VT_DOUBLE? BC_##PREFIX##D##SUFFIX :                \
        (TYPE == VT_STRING? BC_##PREFIX##S##SUFFIX :                \
        throw logic_error("GET_INSN_IDS: " + type2str(TYPE)) )))

    #define GET_INSN_ID(PREFIX, SUFFIX, TYPE)                       \
        (TYPE == VT_INT? BC_##PREFIX##I##SUFFIX :                   \
        (TYPE == VT_DOUBLE? BC_##PREFIX##D##SUFFIX :                \
        throw logic_error("GET_INSN_ID: " + type2str(TYPE)) ))

    #define GET_INSN(INSN)                                          \
        BC_##INSN

    #define ADD_INSN_IDS(PREFIX, SUFFIX, TYPE)                      \
        bc()->addInsn(GET_INSN_IDS(PREFIX, SUFFIX, TYPE))

    #define ADD_INSN_ID(PREFIX, SUFFIX, TYPE)                       \
        bc()->addInsn(GET_INSN_ID(PREFIX, SUFFIX, TYPE))

    #define ADD_INSN(INSN)                                          \
        bc()->addInsn(GET_INSN(INSN))

    #define ADD_U16(INT)                                            \
        bc()->addInt16(INT)

    #define ADD_U64(INT)                                            \
        bc()->addInt64(INT)

    #define ADD_DOUBLE(DOUBLE)                                      \
        bc()->addDouble(DOUBLE)

    #define ADD_BRANCH(TYPE, LABEL)                                 \
        bc()->addBranch(BC_IFICMP##TYPE, LABEL)

    #define ADD_BRANCH_JA(LABEL)                                    \
        bc()->addBranch(BC_JA, LABEL)

    #define BIND(LABEL)                                             \
        bc()->bind(LABEL)

    #define VISIT(NODE)                                             \
        NODE->visit(this)

    #define LOAD_VAR(V)                                             \
        ADD_INSN_IDS(LOADCTX, VAR, V.type);                         \
        ADD_U16(V.fun);                                             \
        ADD_U16(V.num)

    #define STORE_VAR(V)                                            \
        ADD_INSN_IDS(STORECTX, VAR, V.type);                        \
        ADD_U16(V.fun);                                             \
        ADD_U16(V.num)

    struct Var {
        Var(uint16_t fun, uint16_t num, VarType const & type)
        : fun(fun), num(num), type(type) {}

        uint16_t fun;
        uint16_t num;
        VarType type;
    };

    struct Fun {
        Fun(uint16_t id, Signature const & signature, bool isNative)
            : id(id), signature(signature), isNative(isNative) {}

        uint16_t id;
        Signature const & signature;
        bool isNative;
    };

    struct FunScope {
        uint16_t id;

        vector<VarType> stack;
        map<string, Fun> funs;
        shared_ptr<FunScope> parent;
        uint16_t vars_count;

        FunScope(shared_ptr<FunScope> parent, uint16_t id)
            : id(id), parent(parent), vars_count(0) {}

        Fun findFun(string const & name) const {
            map<string, Fun>::const_iterator it;
            it = funs.find(name);

            if ( it != funs.end() ) {
                return it->second;
            }

            if ( parent != NULL ) {
                return parent->findFun(name);
            }

            throw logic_error("Fun not found: " + name);
        }

        void addFun(uint16_t id, string const & name, Signature const & signature, bool isNative) {
            if ( !funs.insert(make_pair(name, Fun(id, signature, isNative))).second ) {
                throw logic_error("Duplicated fun: " + name);
            }
        }

        uint16_t addVar() {
            if (vars_count == MAX_INDEX) {
                throw logic_error("Too much local variables");
            }
            return vars_count++;
        }
    };

    struct VarScope {
        map<string, Var> vars;
        shared_ptr<FunScope> fun;
        shared_ptr<VarScope> parent;

        VarScope(shared_ptr<FunScope> fun, shared_ptr<VarScope> parent)
            : fun(fun), parent(parent) {}

        Var findVar(string const & name) const {
            map<string, Var>::const_iterator it;
            it = vars.find(name);

            if ( it != vars.end() ) {
                return it->second;
            }

            if ( parent != NULL ) {
                return parent->findVar(name);
            }

            throw logic_error("Var not found: " + name);
        }

        void addVar(string const & name, VarType const & type) {
            uint16_t id = fun->addVar();
            if ( !vars.insert(make_pair(name, Var(fun->id, id, type))).second ) {
                throw logic_error("Duplicated var: " + name);
            }
        }
    };

    shared_ptr<VarScope> initVarScope(Scope * ascope) {
        VarScope * scope = new VarScope(fun_scope, var_scope);
        Scope::VarIterator vit(ascope);

        while(vit.hasNext()) {
            AstVar * var = vit.next();
            scope->addVar(var->name(), var->type());
        }

        return shared_ptr<VarScope>(scope);
    }

    void updateFunScope(Scope * ascope) {
        Scope::FunctionIterator fit(ascope);
        vector<shared_ptr<TranslatorVisitor> > funs;

        while(fit.hasNext()) {
            FunctionNode * node = fit.next()->node();

            funs.push_back(shared_ptr<TranslatorVisitor>(new TranslatorVisitor(this, node)));

            TranslatedFunction * result = funs.back()->function();
            fun_scope->addFun(result->id(), result->name(), result->signature(), funs.back()->isNativeFun());
        }

        for (uint16_t i = 0; i < funs.size(); ++i) {
            funs[i]->run();
        }
    }

    void processBlockNode(BlockNode * node) {
        for (uint32_t i = 0; i < node->nodes(); ++i) {
              VISIT(node->nodeAt(i));

            // clear stack after function call without StoreNode. ex: "<...>; sum(1,2);"
            CallNode * cnode = node->nodeAt(i)->asCallNode();
            if (cnode != NULL) {
                Fun fun = fun_scope->findFun(cnode->name());
                if (fun.signature[0].first != VT_VOID) {
                      ADD_INSN(POP);
                    pop();
                }
            }
        }
    }

    void processLogicOperator(TokenKind const & token) {
        convertTop2Numeric();

        VarType type = top();
        pop(type);
        pop(type);

        if (token == tNEQ) {
              ADD_INSN_ID(, CMP, type);
            push(VT_INT);
            return;
        }

        Label end(bc());
        Label no(bc());

        if (type != VT_INT) {
          ADD_INSN_ID(, CMP, type);
          ADD_INSN(ILOAD0);
        }

        switch(token) {
            case tEQ:   // ==
                  ADD_BRANCH(E, no); break;
            case tGT:   // >
                  ADD_BRANCH(G, no); break;
            case tGE:   // >=
                  ADD_BRANCH(GE, no); break;
            case tLT:   // <
                  ADD_BRANCH(L, no); break;
            case tLE:   // <=
                  ADD_BRANCH(LE, no); break;
            default: throw logic_error("Unknown logic token: " + int2str(token));
        }

        if (type == VT_INT) {
              ADD_INSN(ILOAD0);
              ADD_BRANCH_JA(end);
            BIND(no);
              ADD_INSN(ILOAD1);
            BIND(end);
        } else { // invert order after 'cmp 0 (cmp x y)'
              ADD_INSN(ILOAD1);
              ADD_BRANCH_JA(end);
            BIND(no);
              ADD_INSN(ILOAD0);
            BIND(end);
        }

        push(VT_INT);
    }

    void processIntOperator(TokenKind const & token) {
        convertTop2Int();

        VarType type = top();
        pop(type);
        pop(type);

        switch(token) {
            case tOR:    // ||
                  ADD_INSN(IAOR); break;
            case tAND:   // &&
                  ADD_INSN(IMUL); break;
            case tAAND:  // &
                  ADD_INSN(IAAND); break;
            case tAOR:   // |
                  ADD_INSN(IAOR); break;
            case tAXOR:  // ^
                  ADD_INSN(IAXOR); break;
            default: throw logic_error("Bad int token: " + int2str(token));
        }

        push(VT_INT);
    }

    void processNumericOperator(TokenKind const & token) {
        convertTop2Numeric();

        VarType type = top();
        pop(type);
        pop(type);

        switch(token) {
            case tADD:      // +
                  ADD_INSN_ID(, ADD, type); break;
            case tSUB:      // -
                  ADD_INSN_ID(, SUB, type); break;
            case tMUL:      // *
                  ADD_INSN_ID(, MUL, type); break;
            case tDIV:      // /
                  ADD_INSN_ID(, DIV, type); break;
            default: throw logic_error("Bad arithmetic token: " + int2str(token));
        }

        push(type);
    }

    InterpreterCodeImpl * code;
    FunctionNode * root;

    BytecodeFunction_ * result;
    shared_ptr<VarScope> var_scope;
    shared_ptr<FunScope> fun_scope;

    bool isRoot;
    bool isNative;
public:
    TranslatorVisitor(InterpreterCodeImpl * code, FunctionNode * root)
        : code(code), root(root), isRoot(true), isNative(false) {
            result = new BytecodeFunction_(root->name(), root->signature());
            code->addFunction(result);

            fun_scope = shared_ptr<FunScope>(new FunScope(shared_ptr<FunScope>(), result->id()));
            var_scope = shared_ptr<VarScope>();
        }

    TranslatorVisitor(TranslatorVisitor * parent, FunctionNode * root)
        : code(parent->code), root(root), isRoot(false), isNative(false) {
            result = new BytecodeFunction_(root->name(), root->signature());
            code->addFunction(result);

            fun_scope = shared_ptr<FunScope>(new FunScope(parent->fun_scope, result->id()));
            var_scope = parent->var_scope;

            if (root->body()->nodes() == 2 && root->body()->nodeAt(0)->isNativeCallNode()) {
                isNative = true;
            }
        }

    virtual ~TranslatorVisitor() {}

    bool isNativeFun() {
        return isNative;
    }

    TranslatedFunction * function() {
        return result;
    }

    void run() {
        if (!isNative) {
            BlockNode * node = root->body();
            var_scope = initVarScope(node->scope());

            // load parameters from stack
            for (uint16_t i = root->parametersNumber(); i-- > 0;) { //reverse loop
                var_scope->addVar(root->parameterName(i), root->parameterType(i));

                Var var = findVar(root->parameterName(i), root->parameterType(i));
                  STORE_VAR(var);
            }

            fun_scope->addFun(result->id(), root->name(), root->signature(), false);
            updateFunScope(node->scope());

            code->addFunctionData(fun_scope->id, FunctionData(result));

            processBlockNode(node);
            if (isRoot) {
                  ADD_INSN(STOP);
            }
            ADD_INSN(INVALID);

            var_scope = var_scope->parent;
            code->getFunctionData(fun_scope->id)->local_vars = fun_scope->vars_count;
        } else {
            NativeCallNode * node = root->body()->nodeAt(0)->asNativeCallNode();

            if (node->nativeSignature().size() != root->signature().size()) {
               throw logic_error("CallNative: invalid function signature");
            }

            void * pointer = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
            if (pointer == 0)
                throw logic_error("CallNative: " + node->nativeName() + " not found");

            NativeFunction_ * native = new NativeFunction_(node->nativeName(), node->nativeSignature(), pointer);
            code->addFunctionData(result->id(), FunctionData(native));
        }
    }

    virtual void visitBinaryOpNode(BinaryOpNode * node) {
        VISIT(node->right());
        VISIT(node->left());

        switch(node->kind()) {
            case tOR:       // ||
            case tAND:      // &&
            case tAAND:     // &
            case tAOR:      // |
            case tAXOR:     // ^
                  processIntOperator(node->kind());
                return;
            case tNEQ:      // !=
            case tEQ:       // ==
            case tGT:       // >
            case tGE:       // >=
            case tLT:       // <
            case tLE:       // <=
                  processLogicOperator(node->kind());
                return;
            case tADD:      // +
            case tSUB:      // -
            case tMUL:      // *
            case tDIV:      // /
                  processNumericOperator(node->kind());
                return;
            case tMOD:      // %
                assertInt(top(0));
                assertInt(top(1));
                  ADD_INSN(IMOD);
                pop(VT_INT);
                return;
            default:
                throw logic_error("BinaryOp: unknown kind " + int2str(node->kind()));
        }
    }

    virtual void visitUnaryOpNode(UnaryOpNode * node) {
        Label yes(bc());
        Label end(bc());

          VISIT (node->operand());

        VarType type = top();

        switch (node->kind()) {
            case tNOT:  // "!"
                convertStackTop1(VT_INT);
                pop();
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
                assertNumeric(type);
                pop();
                  ADD_INSN_ID(, NEG, type);
                push(type);
                return;
            default:
                throw logic_error("UnaryOp: unknown kind " + int2str(node->kind()));
        }
    }

    virtual void visitStringLiteralNode(StringLiteralNode * node) {
          ADD_INSN(SLOAD);
          ADD_U16(code->makeStringConstant(node->literal()));
        push(VT_STRING);
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode * node) {
        if (node->literal() == 0) {
              ADD_INSN(DLOAD0);
        } else {
              ADD_INSN(DLOAD);
              ADD_DOUBLE(node->literal());
        }
        push(VT_DOUBLE);
    }

    virtual void visitIntLiteralNode(IntLiteralNode * node) {
        switch(node->literal()) {
            case 0:
                  ADD_INSN(ILOAD0); break;
            case 1:
                  ADD_INSN(ILOAD1); break;
            case -1:
                  ADD_INSN(ILOADM1); break;
            default:
                  ADD_INSN(ILOAD);
                  ADD_U64(node->literal());
        }
        push(VT_INT);
    }

    virtual void visitLoadNode(LoadNode * node) {
        AstVar const * avar = node->var();
        Var var = findVar(avar->name(), avar->type());

          LOAD_VAR(var);
        push(var.type);
    }

    virtual void visitStoreNode(StoreNode * node) {
        AstVar const * avar = node->var();
        Var var = findVar(avar->name(), avar->type());

          VISIT(node->value());

        convertStackTop1(var.type);
        switch(node->op()) {
            case tASSIGN:
                break;
            case tINCRSET:
                  LOAD_VAR(var);
                  ADD_INSN_ID(, ADD, var.type);
                break;
            case tDECRSET:
                  LOAD_VAR(var);
                  ADD_INSN_ID(, SUB, var.type);
                break;
            default: throw logic_error("StoreNode: unknown op");
        }

          STORE_VAR(var);
        pop(var.type);
    }

    virtual void visitIfNode(IfNode * node) {
        Label els(bc());
        Label end(bc());

        if (!node->elseBlock()) {
              VISIT(node->ifExpr());
            convertStackTop1(VT_INT);
            pop();
              ADD_INSN(ILOAD0);
              ADD_BRANCH(E, end);
              VISIT(node->thenBlock());
            BIND(end);
        } else {
              VISIT(node->ifExpr());
            convertStackTop1(VT_INT);
            pop();
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
        convertStackTop1(VT_INT);
        pop();
          ADD_INSN(ILOAD0);
          ADD_BRANCH(E, end);
          VISIT(node->loopBlock());
          ADD_BRANCH_JA(begin);

        BIND(end);
    }

    virtual void visitBlockNode(BlockNode * node) {
        var_scope = initVarScope(node->scope());
        updateFunScope(node->scope());

        processBlockNode(node);

        var_scope = var_scope->parent;
    }

    virtual void visitForNode(ForNode * fnode) {
        BlockNode * node = fnode->body();
        Label begin(bc());
        Label end(bc());

        var_scope = initVarScope(node->scope());
        updateFunScope(node->scope());

        AstVar const * avar = fnode->var();
        assertInt(avar->type());
        BinaryOpNode * expr = fnode->inExpr()->asBinaryOpNode();
        if (expr == NULL || expr->kind() != tRANGE) {
            throw logic_error("Bad For loop expression");
        }

        Var var = findVar(avar->name(), VT_INT);

        var_scope->addVar("<for_end>", VT_INT);
        Var for_end = findVar("<for_end>", VT_INT);

          VISIT(expr->left());
          STORE_VAR(var);
          VISIT(expr->right());
          STORE_VAR(for_end);
        pop(VT_INT);
        pop(VT_INT);

        BIND(begin);
          LOAD_VAR(for_end);
          LOAD_VAR(var);
          ADD_BRANCH(G, end);

        processBlockNode(node);

          LOAD_VAR(var);
          ADD_INSN(ILOAD1);
          ADD_INSN_ID(, ADD, VT_INT);
          STORE_VAR(var);

          ADD_BRANCH_JA(begin);
        BIND(end);

        var_scope = var_scope->parent;
    }

    virtual void visitReturnNode(ReturnNode * node) {
        if (node->returnExpr()) {
              VISIT(node->returnExpr());
            convertStackTop1(root->signature()[0].first);
            pop(root->signature()[0].first);
        }
        if (isRoot) {
            assertVoid(root->signature()[0].first);
              ADD_INSN(STOP);
        } else {
              ADD_INSN(RETURN);
        }

        assertEmptyStack();
    }

    virtual void visitFunctionNode(FunctionNode * node) {
        throw logic_error("Visited Function node");
    }

    virtual void visitCallNode(CallNode * node) {
        Fun fun = fun_scope->findFun(node->name());

        if (fun.signature.size() != 1 + node->parametersNumber()) {
            throw logic_error("Call: invalid function signature");
        }

        for (uint16_t i = 0; i < node->parametersNumber(); ++i) {
              VISIT(node->parameterAt(i));
            convertStackTop1(fun.signature[i+1].first);
            pop(fun.signature[i+1].first); // eaten by called function
        }

        if (fun.isNative){
          ADD_INSN(CALLNATIVE);
        } else {
          ADD_INSN(CALL);
        }
          ADD_U16(fun.id);
        if (fun.signature[0].first != VT_VOID) {
            push(fun.signature[0].first);
        }
    }

    virtual void visitNativeCallNode(NativeCallNode * node) {
        throw logic_error("NativeCall node should be processed in TranslatorVisitor()");
    }

    virtual void visitPrintNode(PrintNode * node) {
        for (uint32_t i = 0; i < node->operands(); ++i) {
              VISIT(node->operandAt(i));
              ADD_INSN_IDS(, PRINT, top());
              pop();
        }
    }

private: // --------------------------------------------- //

    Bytecode * bc() {
        return result->bytecode();
    }

    void pop() {
        if (fun_scope->stack.size() == 0) {
            throw logic_error("Stack underflow");
        }
        fun_scope->stack.pop_back();
    }

    void pop(VarType const & type) {
        assertSame(type, top());
        pop();
    }

    void push(VarType const & type) {
        fun_scope->stack.push_back(type);

        if (code->getFunctionData(result->id())->stack_size < fun_scope->stack.size()) {
            code->getFunctionData(result->id())->stack_size = fun_scope->stack.size();
        }
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

    Var findVar(string const & name, VarType const & type) {
        Var var = var_scope->findVar(name);
        assertSame(var.type, type);
        return var;
    }

    void assertEmptyStack() {
        if (fun_scope->stack.size() > 0) {
            throw logic_error("Stack leak");
        }
    }

    bool isIntType(VarType const & type) {
        return type == VT_INT;
    }

    bool isNumericType(VarType const & type) {
        return type == VT_INT || type == VT_DOUBLE;
    }

    void assertVoid(VarType const & type) {
        if (type != VT_VOID) {
            throw logic_error("assertVoid: " + type2str(type));
        }
    }

    void assertInt(VarType const & type) {
        if (type != VT_INT) {
            throw logic_error("assertInt: " + type2str(type));
        }
    }

    void assertNumeric(VarType const & type) {
        if (type != VT_INT && type != VT_DOUBLE) {
            throw logic_error("assertNumeric: " + type2str(type));
        }
    }

    void assertSame(VarType const & left, VarType const & right) {
        if (left != right) {
            throw logic_error("assertSame: " + type2str(left) + " " + type2str(right));
        }
    }

    void convertStackTop1(VarType const & result) {
        VarType stack = top();

        if (stack == result) return;
        if (stack == VT_INT && result == VT_DOUBLE) {
              ADD_INSN(I2D);
            pop(VT_INT);
            push(VT_DOUBLE);
            return;
        }
        if (stack == VT_STRING && result == VT_INT) {
              ADD_INSN(S2I);
            pop(VT_STRING);
            push(VT_INT);
            return;
        }
        if (stack == VT_DOUBLE && result == VT_INT) {
            cerr << "Double to Int cast. Undefined behavior." << endl;
              ADD_INSN(D2I);
            pop(VT_DOUBLE);
            push(VT_INT);
            return;
        }
        throw logic_error("bad top of stack conversion: " + type2str(stack) + " " + type2str(result) );
    }

    void convertTop2Numeric() {
        VarType upper = top(0);
        VarType lower = top(1);
        if (upper == lower) return;
        if (upper == VT_INT && lower == VT_DOUBLE) {
              ADD_INSN(I2D);
            pop(VT_INT);
            push(VT_DOUBLE);
            return;
        }
        if (upper == VT_DOUBLE && lower == VT_INT) {
              ADD_INSN(SWAP);
              ADD_INSN(I2D);
              ADD_INSN(SWAP);
            pop(VT_DOUBLE);
            pop(VT_INT);
            push(VT_DOUBLE);
            push(VT_DOUBLE);
            return;
        }
        throw logic_error("bad top2 of stack to arithmetic conversion: " + type2str(upper) + " " + type2str(lower) );
    }

    void convertTop2Int() {
        VarType upper = top(0);
        VarType lower = top(1);
        if (upper == VT_INT && lower == VT_INT) return;
        if (upper == VT_INT && lower == VT_DOUBLE) {
              ADD_INSN(I2D);
            pop(VT_INT);
            push(VT_DOUBLE);
            return;
        }
        if (upper == VT_DOUBLE && lower == VT_INT) {
              ADD_INSN(SWAP);
              ADD_INSN(I2D);
              ADD_INSN(SWAP);
            pop(VT_DOUBLE);
            pop(VT_INT);
            push(VT_DOUBLE);
            push(VT_DOUBLE);
            return;
        }
        throw logic_error("bad top2 of stack to int conversion: " + type2str(upper) + " " + type2str(lower) );
    }

    #undef GET_INSN_IDS
    #undef GET_INSN_ID
    #undef GET_INSN
    #undef ADD_INSN_IDS
    #undef ADD_INSN_ID
    #undef ADD_INSN
    #undef ADD_U16
    #undef ADD_U64
    #undef ADD_DOUBLE
    #undef ADD_BRANCH
    #undef ADD_BRANCH_JA
    #undef BIND
    #undef VISIT
    #undef LOAD_VAR
    #undef STORE_VAR
};

Status * BytecodeTranslatorImpl::translateBytecode(string const & program, InterpreterCodeImpl ** code) {
    Parser parser;
    Status * status = parser.parseProgram(program);
    if (status && status->isError()) {
        return status;
    } else {
        *code = new InterpreterCodeImpl();
        AstFunction * root = parser.top();
        TranslatorVisitor visitor(*code, root->node());
        try {
            visitor.run();
        } catch(logic_error & e) {
            delete *code;
            return new Status(e.what());
        }
        return new Status();
    }
}

Status * BytecodeTranslatorImpl::translate(string const & program, Code ** code) {
    return translateBytecode(program, (InterpreterCodeImpl**)code);
}

}