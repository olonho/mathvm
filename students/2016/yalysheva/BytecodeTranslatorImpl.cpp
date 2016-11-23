//
// Created by natalia on 11.11.16.
//
#include <limits>
#include <algorithm>
#include <dlfcn.h>
#include "InterpreterCodeImpl.h"
#include "../../../vm/parser.h"
#include "../../../include/visitors.h"

namespace mathvm {

class TranslatorException : public std::exception {
    string const _message;
    uint32_t const _position;
public:
    TranslatorException(string const &message, uint32_t position) :
            _message("translation error: " + message),
            _position(position) {}

    const char *what() const throw() {
        return _message.c_str();
    }

    uint32_t position() const {
        return _position;
    }

    ~TranslatorException() throw() {}
};

struct TranslatorVisitor : public AstBaseVisitor {

    TranslatorVisitor(InterpreterCodeImpl *code, AstFunction *top) : _code(code), _top(top) {
        registerFunction(_top);
        translateFunction(_top);
    }

    virtual ~TranslatorVisitor() {
    }

    virtual void visitForNode(ForNode *node) override {
        VarType type;
        uint16_t ctx;
        uint16_t id;
        findVar(node, node->var()->name(), type, ctx, id);

        if (type != VT_INT || !node->inExpr()->isBinaryOpNode()
            || node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
            throw TranslatorException("incorrect for expression", node->position());
        }

        node->inExpr()->asBinaryOpNode()->left()->visit(this);
        storeVar(node, type, ctx, id);
        Label startLabel(bytecode()->currentLabel());

        node->inExpr()->asBinaryOpNode()->right()->visit(this);
        loadVar(node, type, ctx, id);

        Label endLabel(bytecode());
        bytecode()->addBranch(BC_IFICMPG, endLabel);

        node->body()->visit(this);

        loadVar(node, type, ctx, id);
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addInsn(BC_IADD);
        storeVar(node, type, ctx, id);

        bytecode()->addBranch(BC_JA, startLabel);
        bytecode()->bind(endLabel);
    }

    virtual void visitPrintNode(PrintNode *node) override {
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            switch (tosType()) {
                case VT_INT:
                    bytecode()->addInsn(BC_IPRINT);
                    break;
                case VT_DOUBLE:
                    bytecode()->addInsn(BC_DPRINT);
                    break;
                case VT_STRING:
                    bytecode()->addInsn(BC_SPRINT);
                    break;
                default:
                    throw TranslatorException("incorrect print type", node->position());
            }
            _types.pop_back();
        }
    }

    virtual void visitLoadNode(LoadNode *node) override {
        VarType type;
        uint16_t ctx;
        uint16_t id;
        findVar(node, node->var()->name(), type, ctx, id);
        if (type != node->var()->type()) {
            throw TranslatorException("incorrect variable type", node->position());
        }
        loadVar(node, type, ctx, id);
    }

    virtual void visitIfNode(IfNode *node) override {
        node->ifExpr()->visit(this);
        if (tosType() != VT_INT) {
            throw TranslatorException("invalid if expression", node->position());
        }
        Label elseLabel(bytecode());
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addBranch(BC_IFICMPE, elseLabel);

        node->thenBlock()->visit(this);
        Label endLabel(bytecode());
        if (node->elseBlock())
            bytecode()->addBranch(BC_JA, endLabel);
        bytecode()->bind(elseLabel);

        if (node->elseBlock()) {
            node->elseBlock()->visit(this);
            bytecode()->bind(endLabel);
        }
    }

    virtual void visitBinaryOpNode(BinaryOpNode *node) override {

        if (node->kind() == tOR || node->kind() == tAND) {
            processLazyBinaryOp(node);
            return;
        }
        VarType resultType;
        node->right()->visit(this);
        node->left()->visit(this);

        switch (node->kind()) {
            case tAOR:
                resultType = processBinaryOp(node, BC_IAOR, BC_INVALID);
                break;
            case tAAND:
                resultType = processBinaryOp(node, BC_IAAND, BC_INVALID);
                break;
            case tAXOR:
                resultType = processBinaryOp(node, BC_IAXOR, BC_INVALID);
                break;
            case tEQ:
                intDoubleCast(node);
                resultType = VT_INT;
                {
                    bytecode()->addInsn(BC_ICMP);
                    bytecode()->addInsn(BC_ILOAD0);
                    Label trueLabel(bytecode());
                    bytecode()->addBranch(BC_IFICMPE, trueLabel);
                    bytecode()->addInsn(BC_ILOAD0);
                    Label falseLabel(bytecode());
                    bytecode()->addBranch(BC_JA, falseLabel);
                    bytecode()->bind(trueLabel);
                    bytecode()->addInsn(BC_ILOAD1);
                    bytecode()->bind(falseLabel);
                }
                break;
            case tNEQ:
                intDoubleCast(node);
                resultType = VT_INT;
                bytecode()->addInsn(BC_ICMP);
                break;
            case tGT:
                resultType = processCompareOp(node, 2, false);
                break;
            case tGE:
                resultType = processCompareOp(node, 3, false);
                break;
            case tLT:
                resultType = processCompareOp(node, 2, true);
                break;
            case tLE:
                resultType = processCompareOp(node, 3, true);
                break;
            case tADD:
                resultType = processBinaryOp(node, BC_IADD, BC_DADD);
                break;
            case tSUB:
                resultType = processBinaryOp(node, BC_ISUB, BC_DSUB);
                break;
            case tMUL:
                resultType = processBinaryOp(node, BC_IMUL, BC_DMUL);
                break;
            case tDIV:
                resultType = processBinaryOp(node, BC_IDIV, BC_DDIV);
                break;
            case tMOD:
                resultType = processBinaryOp(node, BC_IMOD, BC_INVALID);
                break;
            default:
                throw TranslatorException("incorrect binary operator", node->position());
        }
        _types.pop_back();
        _types.pop_back();
        _types.push_back(resultType);

    }

    virtual void visitCallNode(CallNode *node) override {
        TranslatedFunction *tf = _code->functionByName(node->name());
        if (!tf) {
            throw TranslatorException("undefined function call", node->position());
        }

        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            VarType const type =
                    _code->functionByName(node->name())->parameterType(i);
            castTypes(node, tosType(), type);
            _types.pop_back();
        }
        bytecode()->addInsn(BC_CALL);
        bytecode()->addInt16(tf->id());

        _types.push_back(_code->functionByName(node->name())->returnType());
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) override {
        double const literal = node->literal();
        if (literal == 0.0) {
            bytecode()->addInsn(BC_DLOAD0);
        } else if (literal == 1.0) {
            bytecode()->addInsn(BC_DLOAD1);
        } else if (literal == -1.0) {
            bytecode()->addInsn(BC_DLOADM1);
        } else {
            bytecode()->addInsn(BC_DLOAD);
            bytecode()->addDouble(literal);
        }
        _types.push_back(VT_DOUBLE);
    }

    virtual void visitStoreNode(StoreNode *node) override {
        VarType type;
        uint16_t ctx;
        uint16_t id;
        findVar(node, node->var()->name(), type, ctx, id);

        node->value()->visit(this);

        switch (node->op()) {
            case tASSIGN:
                break;
            case tINCRSET:
                loadVar(node, type, ctx, id);
                processBinaryOp(node, BC_IADD, BC_DADD);
                break;
            case tDECRSET:
                loadVar(node, type, ctx, id);
                processBinaryOp(node, BC_ISUB, BC_DSUB);
                break;
            default:
                throw TranslatorException("incorrect assignment", node->position());
        }

        storeVar(node, type, ctx, id);
        loadVar(node, type, ctx, id);
    }

    virtual void visitStringLiteralNode(StringLiteralNode *node) override {
        uint16_t const id = _code->makeStringConstant(node->literal());
        bytecode()->addInsn(BC_SLOAD);
        bytecode()->addUInt16(id);
        _types.push_back(VT_STRING);
    }

    virtual void visitWhileNode(WhileNode *node) override {
        Label startLabel(bytecode()->currentLabel());

        node->whileExpr()->visit(this);
        if (tosType() != VT_INT) {
            throw TranslatorException("incorrect while expression", node->position());
        }
        Label endLabel(bytecode());
        bytecode()->addInsn(BC_ILOAD0);
        bytecode()->addBranch(BC_IFICMPE, endLabel);

        node->loopBlock()->visit(this);
        bytecode()->addBranch(BC_JA, startLabel);
        bytecode()->bind(endLabel);
    }

    virtual void visitIntLiteralNode(IntLiteralNode *node) override {
        int64_t const literal = node->literal();
        switch (literal) {
            case 0:
                bytecode()->addInsn(BC_ILOAD0);
                break;
            case 1:
                bytecode()->addInsn(BC_ILOAD1);
                break;
            case -1:
                bytecode()->addInsn(BC_ILOADM1);
                break;
            default:
                bytecode()->addInsn(BC_ILOAD);
                bytecode()->addInt64(literal);
        }
        _types.push_back(VT_INT);
    }

    virtual void visitUnaryOpNode(UnaryOpNode *node) override {
        node->operand()->visit(this);
        switch (node->kind()) {
            case tSUB:
                if (tosType() != VT_DOUBLE && tosType() != VT_INT) {
                    throw TranslatorException("incorrect unary minus", node->position());
                }
                bytecode()->addInsn((tosType() == VT_INT) ? BC_INEG : BC_DNEG);
                break;
            case tNOT:
                if (tosType() == VT_STRING) {
                    bytecode()->addInsn(BC_S2I);
                    _types.pop_back();
                    _types.push_back(VT_INT);
                }
                if (tosType() != VT_INT) {
                    throw TranslatorException("incorrect negation", node->position());
                } else {
                    bytecode()->addInsn(BC_ILOAD0);
                    Label falseLabel(bytecode());
                    bytecode()->addBranch(BC_IFICMPE, falseLabel);
                    bytecode()->addInsn(BC_ILOAD0);
                    Label endLabel(bytecode());
                    bytecode()->addBranch(BC_JA, endLabel);
                    bytecode()->bind(falseLabel);
                    bytecode()->addInsn(BC_ILOAD1);
                    bytecode()->bind(endLabel);
                }
                break;
            default:
                throw TranslatorException("incorrect unary operator", node->position());
        }
    }

    virtual void visitNativeCallNode(NativeCallNode *node) override {
        void *code = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
        if (!code) {
            throw TranslatorException("incorrect native function", node->position());
        }
        uint16_t const id = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), code);

        bytecode()->addInsn(BC_CALLNATIVE);
        bytecode()->addInt16(id);
        ++_retCounts.back();
        VarType returnType = _retTypes.back();
        if (returnType != VT_VOID) {
            _types.push_back(returnType);
            storeVar(node, returnType, curCtx(), 0);
        }
        bytecode()->addBranch(BC_JA, _retLabels.back());
    }

    virtual void visitBlockNode(BlockNode *node) override {
        bool const scope = node->scope() != _scopes.back().second;
        if (scope)
            pushScope(0, node->scope());

        Scope::FunctionIterator functionIterator(node->scope());
        while (functionIterator.hasNext()) {
            registerFunction(functionIterator.next());
        }

        functionIterator = Scope::FunctionIterator(node->scope());
        while (functionIterator.hasNext()) {
            translateFunction(functionIterator.next());
        }

        for (uint32_t i = 0; i < node->nodes(); ++i) {
            AstNode *node_i = node->nodeAt(i);
            node_i->visit(this);
            if (node_i->isForNode() || node_i->isWhileNode()
                || node_i->isIfNode() || node_i->isPrintNode())
                continue;
            bytecode()->addInsn(BC_POP);
            _types.pop_back();
        }

        if (scope) {
            _scopes.pop_back();
            _vars.pop_back();
        }
    }

    virtual void visitReturnNode(ReturnNode *node) override {
        ++_retCounts.back();

        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
        } else if (_retTypes.back() == VT_VOID) {
            _types.push_back(_retTypes.back());
            bytecode()->addBranch(BC_JA, _retLabels.back());
            return;
        } else {
            throw TranslatorException("empty return", node->position());
        }

        storeVar(node, _retTypes.back(), curCtx(), 0, "incorrect return type");
        _types.push_back(_retTypes.back());
        bytecode()->addBranch(BC_JA, _retLabels.back());
    }

    virtual void visitFunctionNode(FunctionNode *node) override {
        for (int64_t i = static_cast<int64_t>(node->parametersNumber()) - 1;
             i >= 0; --i) {
            addStoreBytecode(node, node->parameterType(i), curCtx(), i + 1);
        }
        bool native = node->body()->nodes() > 0 && node->body()->nodeAt(0)
                      && node->body()->nodeAt(0)->isNativeCallNode();
        if (native)
            node->body()->nodeAt(0)->visit(this);
        else
            node->body()->visit(this);
        if (_retTypes.back() != VT_VOID && _retCounts.back() == 0) {
            throw TranslatorException("missing return", node->position());
        }
        bytecode()->bind(_retLabels.back());
        loadVar(node, node->returnType(), curCtx(), 0);
        _types.pop_back();
        bool const stop = node->name() == AstFunction::top_name;
        bytecode()->addInsn(stop ? BC_STOP : BC_RETURN);
    }

private:

    void registerFunction(AstFunction *func) {
        if (_code->functionByName(func->name())) {
            throw TranslatorException("duplicate declaration", func->node()->position());
        }
        BytecodeFunction *bfunc = new BytecodeFunction(func);
        uint16_t const fid = _code->addFunction(bfunc);
        _funids[func->name()] = fid;
    }

    void translateFunction(AstFunction *fun) {
        BytecodeFunction *bcfun = (BytecodeFunction *) _code->functionByName(fun->name());

        pushScope(fun->node(), fun->node()->body()->scope());
        _functions.push_back(fun->node());
        _bcs.push_back(bcfun->bytecode());
        _retLabels.push_back(Label(bytecode()));
        _retTypes.push_back(fun->returnType());
        _retCounts.push_back(0);

        fun->node()->visit(this);

        _bcs.pop_back();
        _retLabels.pop_back();
        _retTypes.pop_back();
        _retCounts.pop_back();
        _functions.pop_back();
        _scopes.pop_back();
        _vars.pop_back();
    }

    VarType intDoubleCast(AstNode const *node) {
        if ((tosType() != VT_INT && tosType() != VT_DOUBLE)
            || (prevTosType() != VT_INT && prevTosType() != VT_DOUBLE)) {
            throw TranslatorException("incorrect operands", node->position());
        }
        bool isTosDouble = tosType() == VT_DOUBLE;
        bool isPrevTosDouble = prevTosType() == VT_DOUBLE;
        if (isTosDouble && !isPrevTosDouble) {
            bytecode()->addInsn(BC_SWAP);
            bytecode()->addInsn(BC_I2D);
            bytecode()->addInsn(BC_SWAP);
        } else if (isPrevTosDouble && !isTosDouble) {
            bytecode()->addInsn(BC_I2D);
        }
        return (isTosDouble || isPrevTosDouble) ? VT_DOUBLE : VT_INT;
    }

    void processLazyBinaryOp(BinaryOpNode *node) {
        bool const isAnd = node->kind() == tAND;
        node->left()->visit(this);
        if (tosType() != VT_INT) {
            throw TranslatorException("incorrect operands", node->position());
        }
        _types.pop_back();

        bytecode()->addInsn(BC_ILOAD0);
        Label trueLabel(bytecode());
        bytecode()->addBranch(isAnd ? BC_IFICMPNE : BC_IFICMPE, trueLabel);
        bytecode()->addInsn(isAnd ? BC_ILOAD0 : BC_ILOAD1);
        Label falseLabel(bytecode());
        bytecode()->addBranch(BC_JA, falseLabel);
        bytecode()->bind(trueLabel);

        node->right()->visit(this);
        if (tosType() != VT_INT) {
            throw TranslatorException("incorrect operands", node->position());
        }
        _types.pop_back();

        bytecode()->bind(falseLabel);
        _types.push_back(VT_INT);
    }

    VarType processBinaryOp(
            AstNode const *node,
            Instruction intIns, Instruction doubleIns) {
        VarType result;
        if (doubleIns == BC_INVALID) {
            if (tosType() != VT_INT || prevTosType() != VT_INT)
                throw TranslatorException("incorrect operands", node->position());
            result = VT_INT;
        } else {
            result = intDoubleCast(node);
        }
        bytecode()->addInsn((result == VT_INT) ? intIns : doubleIns);
        return result;
    }

    VarType processCompareOp(AstNode const *node, int64_t mask, bool swap) {
        VarType result = intDoubleCast(node);
        if (swap) bytecode()->addInsn(BC_SWAP);
        bytecode()->addInsn((result == VT_INT) ? BC_ICMP : BC_DCMP);
        bytecode()->addInsn(BC_ILOAD1);
        bytecode()->addInsn(BC_IADD);
        bytecode()->addInsn(BC_ILOAD);
        bytecode()->addInt64(mask);
        bytecode()->addInsn(BC_IAAND);
        return VT_INT;
    }

    void findVar(AstNode const *node, std::string const &name, VarType &type, uint16_t &ctx, uint16_t &id) {
        ctx = 0;
        id = 0;
        bool found = false;
        int i = static_cast<int>(_scopes.size()) - 1;
        for (; !found && i >= 0; --i) {
            FunctionNode *functionNode = _scopes[i].first;
            std::vector<AstVar *> &vars = _vars[i];
            if (functionNode) {
                ctx = _funids[functionNode->name()];
                for (uint32_t j = 0; j < functionNode->parametersNumber(); ++j) {
                    if (functionNode->parameterName(j) == name) {
                        id = j + 1;
                        type = functionNode->parameterType(j);
                        return;
                    }
                }
            }
            AstVar tmp(name, VT_INVALID, 0);
            std::vector<AstVar *>::iterator const iterator(
                    std::lower_bound(vars.begin(), vars.end(), &tmp,
                                     [](AstVar *lhs, AstVar *rhs) {
                                         return lhs->name() < rhs->name();
                                     }));
            if (iterator != vars.end() && (*iterator)->name() == name) {
                id = (iterator - vars.begin()) + (functionNode ? functionNode->parametersNumber() : 0) + 1;
                type = (*iterator)->type();
                found = true;
            }
        }
        if (!found) {
            throw TranslatorException("undefined variable", node->position());
        }
        ++i;
        if (i == 0 || _scopes[i].first)
            return;
        --i;
        for (; i >= 0; --i) {
            FunctionNode *functionNode = _scopes[i].first;
            Scope *scope = _scopes[i].second;
            id += scope->variablesCount();
            if (functionNode) {
                ctx = _funids[functionNode->name()];
                id += functionNode->parametersNumber();
                break;
            }
        }
    }


    void loadVar(AstNode const *node, VarType type, uint16_t ctx, uint16_t id) {
        switch (type) {
            case VT_INT:
                bytecode()->addInsn(BC_LOADCTXIVAR);
                break;
            case VT_DOUBLE:
                bytecode()->addInsn(BC_LOADCTXDVAR);
                break;
            case VT_STRING:
                bytecode()->addInsn(BC_LOADCTXSVAR);
                break;
            case VT_VOID:
                bytecode()->addInsn(BC_ILOAD0);
                break;
            default:
                throw TranslatorException("incorrect variable", node->position());
        }

        if (type != VT_VOID) {
            bytecode()->addInt16(ctx);
            bytecode()->addInt16(id);
        }

        _types.push_back(type);
    }

    void storeVar(AstNode const *node, VarType type, uint16_t ctx, uint16_t id,
                  string const errMsg = "variable store error") {
        castTypes(node, tosType(), type);
        addStoreBytecode(node, type, ctx, id, errMsg);
        _types.pop_back();
    }

    void addStoreBytecode(AstNode const *node, VarType type, uint16_t ctx, uint16_t id, string const errMsg = "variable store error") {
        switch (type) {
            case VT_INT:
                bytecode()->addInsn(BC_STORECTXIVAR);
                break;
            case VT_DOUBLE:
                bytecode()->addInsn(BC_STORECTXDVAR);
                break;
            case VT_STRING:
                bytecode()->addInsn(BC_STORECTXSVAR);
                break;
            default:
                throw TranslatorException(errMsg, node->position());
        }
        bytecode()->addInt16(ctx);
        bytecode()->addInt16(id);
    }

    VarType tosType() const {
        if (_types.empty())
            return VT_INVALID;
        return _types.back();
    }

    VarType prevTosType() const {
        if (_types.size() < 2)
            return VT_INVALID;
        return _types[_types.size() - 2];
    }

    void castTypes(AstNode const *node, VarType from, VarType to) {
        if (from == to)
            return;
        if (from == VT_INT && to == VT_DOUBLE) {
            bytecode()->addInsn(BC_I2D);
        } else if (from == VT_DOUBLE && to == VT_INT) {
            bytecode()->addInsn(BC_D2I);
        } else {
            throw TranslatorException("incorrect type cast", node->position());
        }
    }

    uint16_t curCtx() {
        return _funids[_functions.back()->name()];
    }

    void pushScope(FunctionNode *node, Scope *scope) {
        _scopes.push_back(std::make_pair(node, scope));
        _vars.push_back(std::vector<AstVar *>());
        Scope::VarIterator varIterator(scope);
        while (varIterator.hasNext()) {
            _vars.back().push_back(varIterator.next());
        }
        if (_vars.back().size() > std::numeric_limits<uint16_t>::max()) {
            throw TranslatorException("exceeded number of variables", 0);
        }
        std::sort(_vars.back().begin(), _vars.back().end(),
                  [](AstVar *lhs, AstVar *rhs) {
                      return lhs->name() < rhs->name();
                  });
    }

    Bytecode *bytecode() {
        return _bcs.back();
    }

    InterpreterCodeImpl *_code;
    AstFunction *_top;
    std::vector<VarType> _types;
    std::vector<std::pair<FunctionNode *, Scope *> > _scopes;
    std::vector<std::vector<AstVar *> > _vars;
    std::vector<FunctionNode *> _functions;
    std::map<std::string, uint16_t> _funids;
    std::vector<int> _retCounts;
    std::vector<VarType> _retTypes;
    std::vector<Label> _retLabels;
    std::vector<Bytecode *> _bcs;

};

Status *BytecodeTranslatorImpl::translateBytecode(const string &program, InterpreterCodeImpl **code) {

    Parser parser;
    Status *pStatus = parser.parseProgram(program);

    if (!pStatus->isOk())
        return pStatus;

    *code = new InterpreterCodeImpl();
    TranslatorVisitor visitor(*code, parser.top());

    Code::FunctionIterator functionIterator(*code);
    while(functionIterator.hasNext()){
        BytecodeFunction *bcfun = (BytecodeFunction*)functionIterator.next();
        cout << "function: " << bcfun->name() << ", idx: " << bcfun->id() << endl;
        bcfun->bytecode()->dump(cout);
        cout << endl;
    }
    return pStatus;
}

Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    return translateBytecode(program, (InterpreterCodeImpl **) (code));
}

}
