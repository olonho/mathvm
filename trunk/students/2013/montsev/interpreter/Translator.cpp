#include "parser.h"
#include "errors.h"

#include <sstream>
#include <stdexcept>

#include <iostream>

using namespace mathvm;

// constants

static const uint16_t MAX_ID = 65535;

// Ast visitor implementation
class TranslatorVisitor : public AstVisitor {

public: // constructors

    TranslatorVisitor(AstFunction* top, Code* code)
        : _code(code), _bcFunction(new BytecodeFunction(top)), _scope(0) 
        {
            _code->addFunction(_bcFunction);
        }

    virtual ~TranslatorVisitor() {
        for(vector<VarScope*>::iterator it = _scopes.begin(); it != _scopes.end(); ++it) {
            delete *it;
        }
    }

private: // structs

    // internal compiler structs

    struct Var {

        string name;
        VarType type;
        uint16_t ctx;
        uint16_t id;

        Var() {}

        Var(const string& name, VarType type, uint16_t ctx, uint16_t id)
            : name(name), type(type), ctx(ctx), id(id) {}
    };

    struct VarScope {
        typedef map<string, Var> VarMap;
        typedef map<string, BytecodeFunction*> FuncMap;

        typedef VarMap::iterator VarIter;

        VarScope(VarScope* parent, size_t lastId)
            : _parent(parent), _lastId(lastId) {}

        VarScope* parent() const {
            return _parent;
        }

        void declareFunction(const string& name, BytecodeFunction* func) {
            if (_functions.find(name) != _functions.end()) {
                reportFunctionDuplicateError(name);
            }
            _functions[name] = func;
        }

        uint16_t declareVar(const string& name, VarType type, uint16_t ctx) {
            size_t len = _lastId;
            _lastId++;

            if (len >= MAX_ID) {
                reportOverflowError(len);
            }

            if (_locals.find(name) != _locals.end()) {
                reportDuplicateError(name);
            }

            Var newvar(name, type, ctx, len);
            _locals[name] = newvar;
            return len;
        }

        BytecodeFunction* resolveFunction(const string& name) {
            if (_functions.find(name) == _functions.end()) {
                if (_parent != 0) {
                    return _parent->resolveFunction(name);
                }

                reportResolveError(name);
            }
            return _functions.at(name);
        }
        
        Var resolveName(const string& name) {
            if (_locals.find(name) == _locals.end()) {
                if (_parent != 0) {
                    return _parent->resolveName(name);
                }
                reportResolveError(name);
            }
            return _locals.at(name);
        }

        size_t varCount() const {
            return _lastId;
        }

    private: 

        VarMap _locals;
        FuncMap _functions;
        VarScope* _parent;
        size_t _lastId;

        const VarMap& vars() const {
            return _locals;
        }

        // error handlers

        void reportFunctionDuplicateError(const string& name) const {
            stringstream msg;
            msg << "Duplicate function: "
                << name
                << endl;
            throw error(msg.str());
        }

        void reportOverflowError(size_t vars) const {
            stringstream msg;
            msg << "Too much local variables: "
                << vars << endl;
            throw error(msg.str());
        }

        void reportDuplicateError(const string& name) const {
            stringstream msg;
            msg << "Duplicate variable: "
                << name << endl;
            throw error(msg.str());
        }

        void reportResolveError(const string& name) const {
            stringstream msg;
            msg << "Cannot resolve name: "
                << name << endl;
            throw error(msg.str());
        }

    };

    union Val {
        int64_t ival;
        double dval;
        uint16_t sval;
    };  

private: // fields

    Code* _code;

    // Current bytecode function
    BytecodeFunction* _bcFunction;

    // Current context scope
    VarScope* _scope;

    // Type of the last expression
    VarType _lastType;

    // Collection of all scopes
    vector<VarScope*> _scopes;

private: // methods

    // checkers

    void checkNumericTypes(VarType left, VarType right) const {
        if ((left != VT_INT && left != VT_DOUBLE) || 
            (right != VT_INT && right != VT_DOUBLE)) {

            stringstream msg;
            msg << "Invalid types for numeric operation. "
                << "Left operand type: " << typeToName(left)
                << ". Right operand type: " << typeToName(right);
            throw error(msg.str());
        }
    }

    void checkOpType(VarType left, VarType right, 
                     VarType expected, TokenKind kind) const {

        if (left != expected || right != expected) {
            stringstream msg;
            msg << "Invalid binary operation. " << tokenOp(kind)
                << ". Left operand type: " << typeToName(left)
                << ". Right operand type: " << typeToName(right)
                << ". Expected type: " << typeToName(expected);
            throw error(msg.str());
        }
    }

    void checkVarType(VarType expected, VarType found) const {
        if (expected != found) {
            stringstream msg;
            msg << "Type error. Expected: "
                << typeToName(expected)
                << ". Found: "
                << typeToName(found);
            throw error(msg.str());
        }
    }

    void checkNotNull(void* ptr, const string& msg) const {
        if (!ptr) {
            throw error(msg);
        }
    }

    // utils

    VarScope* constructScope(VarScope* parent, size_t vars) {
        VarScope* varscope = new VarScope(parent, vars);
        _scopes.push_back(varscope);
        return varscope;
    }

    Bytecode* bc() {
        return _bcFunction->bytecode();
    }

    // bytecode helpers

    #define GENERATE(name, type) \
        void name(type op) { bc()->name(op); }

    GENERATE(addInsn, Instruction);
    GENERATE(bind, Label&);
    GENERATE(addInt16, int16_t);
    GENERATE(addInt64, int64_t);
    GENERATE(addDouble, double);
    GENERATE(addUInt16, uint16_t);

    #undef GENERATE

    #define GENERATE(name, type1, type2) \
        void name(type1 op1, type2 op2) { bc()->name(op1, op2); }

    GENERATE(addBranch, Instruction, Label&);
    #undef GENERATE

    void addLogicOperator(TokenKind kind) {
        addInsn2(BC_ICMP, BC_DCMP, _lastType);
        _lastType = VT_INT;
        Label endIf(bc());
        Label elseIf(bc());
        switch (kind) {
            case tEQ: {
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPE, elseIf);
                break; 
            }
            case tGT: {
                addInsn(BC_ILOAD1);
                addBranch(BC_IFICMPE, elseIf);
                break; 
            }
            case tGE: {
                addInsn(BC_ILOADM1);
                addBranch(BC_IFICMPNE, elseIf);
                break; 
            }
            case tLT: {
                addInsn(BC_ILOADM1);
                addBranch(BC_IFICMPE, elseIf);
                break;
            }
            case tNEQ: {
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPNE, elseIf);
                break; 
            }
            case tLE: {
                addInsn(BC_ILOAD1);
                addBranch(BC_IFICMPNE, elseIf);
                break; 
            }
            default: break;
        }
        addInsn(BC_ILOAD0);
        addBranch(BC_JA, endIf);
        bind(elseIf);
        addInsn(BC_ILOAD1);
        bind(endIf);
    }

    void addIntOperator(TokenKind kind) {
        Label elseIf(bc());
        Label endIf(bc());
        switch (kind) {
            case tOR: {
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPNE, elseIf);
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPNE, elseIf);
                addInsn(BC_ILOAD0);
                addBranch(BC_JA, endIf);
                bind(elseIf);
                addInsn(BC_ILOAD1);
                bind(endIf);
                break;
            }
            case tAND: {
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPE, elseIf);
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPE, elseIf);
                addInsn(BC_ILOAD1);
                addBranch(BC_JA, endIf);
                bind(elseIf);
                addInsn(BC_ILOAD0);
                bind(endIf);                    
                break;
            }
            case tAOR:
                addInsn(BC_IAOR); break;
            case tAAND:
                addInsn(BC_IAAND); break;
            case tAXOR:
                addInsn(BC_IAXOR); break;
            default: break;
        }
    }

    void addNumericOperator(TokenKind kind) {
        switch (kind) {
            case tADD: 
                addInsn2(BC_IADD, BC_DADD, _lastType); break;
            case tSUB:
                addInsn2(BC_ISUB, BC_DSUB, _lastType); break;
            case tMUL:
                addInsn2(BC_IMUL, BC_DMUL, _lastType); break; 
            case tDIV:
                addInsn2(BC_IDIV, BC_DDIV, _lastType); break;
            default: break;
        }
    }

    void addConvertOps(VarType upper, VarType lower) {
        checkNumericTypes(upper, lower);
        if (upper != lower) {
            if (lower == VT_DOUBLE) {
                addIntToDoubleConv();
            } else if (upper == VT_DOUBLE) {
                addInsn(BC_STOREDVAR3);
                addIntToDoubleConv();
                addInsn(BC_LOADDVAR3);
            }
            _lastType = VT_DOUBLE;
        }
    }

    void addDirectCast(VarType from, VarType to) {
        if (from == VT_INT && to == VT_DOUBLE) {
            addIntToDoubleConv();
            _lastType = VT_DOUBLE;
        } else if (from == VT_DOUBLE && to == VT_INT) {
            addDoubleToIntConv();
            _lastType = VT_INT;
        }
    }

    void addDoubleToIntConv() {
        addInsn(BC_D2I);
    }

    void addIntToDoubleConv() {
        addInsn(BC_I2D);
    }

    void addLiteralOnTOS(VarType type, Val u) {
        switch(type) {
            case VT_INT:
                addInsn(BC_ILOAD);
                addInt64(u.ival);
                break;
            case VT_DOUBLE:
                addInsn(BC_DLOAD);
                addDouble(u.dval);
                break;
            case VT_STRING:
                addInsn(BC_SLOAD);
                addInt16(u.sval);
                break;
            default:
                throw error("Unsupported type for operation. ");
        }
    }

    void addVarInsn3(Instruction bcInt, Instruction bcDouble, 
                     Instruction bcString, Var var) {

        switch (var.type) {
            case VT_INT:
                addInsn(bcInt);
                break;
            case VT_DOUBLE:
                addInsn(bcDouble);
                break;
            case VT_STRING:
                addInsn(bcString);
                break; 
            default:
                throw error("Invalid type of variable: " + var.name);
        }
        addUInt16(var.id);
    }

    void addInsn2(Instruction bcInt, Instruction bcDouble, VarType type) {
        switch (type) {
            case VT_INT:
                addInsn(bcInt);
                break;
            case VT_DOUBLE:
                addInsn(bcDouble);
                break;
            case VT_STRING:
                throw error("Invalid operation on string. " );
                break;
            default:
                throw error("Unsupported type for operation. ");
        }
    }

    void addLoadVarInsn(Var var) {
        if (var.ctx != _bcFunction->id()) {
            addVarInsn3(BC_LOADCTXIVAR, BC_LOADCTXDVAR, BC_LOADCTXSVAR, var);
            addUInt16(var.ctx);
            return;
        }
        addVarInsn3(BC_LOADIVAR, BC_LOADDVAR, BC_LOADSVAR, var);
    }

    void addStoreVarInsn(Var var) {
        if (var.ctx != _bcFunction->id()) {
            addVarInsn3(BC_STORECTXIVAR, BC_STORECTXDVAR, BC_STORECTXSVAR, var);
            addUInt16(var.ctx);
            return;
        }
        addVarInsn3(BC_STOREIVAR, BC_STOREDVAR, BC_STORESVAR, var);
    }

    void addAddInsn(VarType type) {
        addInsn2(BC_IADD, BC_DADD, type);
    }

    void addSubInsn(VarType type) {
        addInsn2(BC_ISUB, BC_DSUB, type);
    }

    // visitors

    void visitBinaryOpNode(BinaryOpNode* node) {
        TokenKind kind = node->kind();
        // Left is upper, right is lower
        node->right()->visit(this);
        VarType lower = _lastType;
        node->left()->visit(this);
        VarType upper = _lastType;
        switch (kind) {
            case tOR: 
            case tAND:
            case tAOR:
            case tAAND:
            case tAXOR:  
                checkOpType(upper, lower, VT_INT, kind);
                addIntOperator(kind);
                break;

            case tEQ:
            case tNEQ:
            case tGT:
            case tGE:
            case tLT:
            case tLE: 
                addConvertOps(upper, lower);
                addLogicOperator(kind);
                break;

            case tADD:
            case tSUB:
            case tMUL:
            case tDIV:
                addConvertOps(upper, lower);
                addNumericOperator(kind);
                break;

            case tMOD: 
                checkOpType(upper, lower, VT_INT, kind);
                addInsn(BC_IMOD);
                break;
            case tRANGE:
                checkOpType(upper, lower, VT_INT, kind);
                break;
            default: throw error("Unknown token. ");
        }
    }

    void visitUnaryOpNode(UnaryOpNode* node) {
        TokenKind kind = node->kind();
        node->operand()->visit(this);
        checkNumericTypes(_lastType, VT_INT);

        if (_lastType == VT_DOUBLE && kind == tNOT) {
            throw error("Can't do logical not on double. ");
        }
        switch (kind) {
            case tSUB:
                addInsn2(BC_INEG, BC_DNEG, _lastType);
                break;
            case tNOT: {
                Label elseIf(bc());
                Label endIf(bc());
                addInsn(BC_ILOAD0);
                addBranch(BC_IFICMPNE, elseIf);
                addInsn(BC_ILOAD1);
                addBranch(BC_JA, endIf);
                bind(elseIf);
                addInsn(BC_ILOAD0);
                bind(endIf);
                break;
            }
            default: throw error("Unknown token. ");
        }

    }

    void visitStringLiteralNode(StringLiteralNode* node) {
        Val literal;
        uint16_t id = _code->makeStringConstant(node->literal());
        literal.sval = id;
        addLiteralOnTOS(VT_STRING, literal);
        _lastType = VT_STRING;
    }

    void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        Val literal;
        literal.dval = node->literal();
        addLiteralOnTOS(VT_DOUBLE, literal);
        _lastType = VT_DOUBLE;
    }

    void visitIntLiteralNode(IntLiteralNode* node) {
        Val literal;
        literal.ival = node->literal();
        addLiteralOnTOS(VT_INT, literal);
        _lastType = VT_INT;
    }

    void visitLoadNode(LoadNode* node) {
        VarType nodeType = node->var()->type();
        string nodeName = node->var()->name();

        Var var = _scope->resolveName(nodeName);

        checkVarType(var.type, nodeType);

        addLoadVarInsn(var);
        _lastType = var.type;
    }

    void visitStoreNode(StoreNode* node) {
        node->visitChildren(this);

        Var var = _scope->resolveName(node->var()->name());

        // Int to double implicit conversion
        if (var.type == VT_DOUBLE && _lastType == VT_INT) {
            addIntToDoubleConv();
        } else {
            checkVarType(var.type, _lastType);
        }

        try {
            switch (node->op()) {
                case tINCRSET:
                    addLoadVarInsn(var);
                    addAddInsn(var.type);
                    break;
                case tDECRSET:
                    addLoadVarInsn(var);
                    addSubInsn(var.type);
                    break;
                case tASSIGN:
                    break;
                default: throw error("Unknown store operator. ");
            }
        } catch (exception& e) {
            stringstream msg;
            msg << e.what() << "Variable name is: " << var.name << endl;
            throw error(msg.str());
        }
        
        addStoreVarInsn(var);
    }

    void visitForNode(ForNode* node) {
        // IVAR3 - general purpose register
        
        checkNotNull((void*) node->var(), "Invalid for expression. ");

        Var var(_scope->resolveName(node->var()->name()));
        checkVarType(var.type, VT_INT);

        node->inExpr()->visit(this);

        Label beginFor(bc());
        Label endFor(bc());
        
        addStoreVarInsn(var);

        bind(beginFor);

        addInsn(BC_STOREIVAR3);
        addInsn(BC_LOADIVAR3);
        addLoadVarInsn(var);

        addBranch(BC_IFICMPG, endFor);

        addInsn(BC_LOADIVAR3);

        node->body()->visit(this);

        addLoadVarInsn(var);
        addInsn(BC_ILOAD1);
        addInsn(BC_IADD);
        addStoreVarInsn(var);

        addBranch(BC_JA, beginFor);
        bind(endFor);
    }
        
    void visitWhileNode(WhileNode* node) {
        Label loopWhile(bc());
        Label endWhile(bc());
        bind(loopWhile);

        node->whileExpr()->visit(this);
        VarType exprType = _lastType;
        checkVarType(exprType, VT_INT);

        addInsn(BC_ILOAD0);
        addBranch(BC_IFICMPE, endWhile);
        node->loopBlock()->visit(this);
        addBranch(BC_JA, loopWhile);
        bind(endWhile);
    }

    void visitIfNode(IfNode* node) {
        node->ifExpr()->visit(this);
        addDirectCast(_lastType, VT_INT);
        checkVarType(VT_INT, _lastType);
        
        Label elseIf(bc());
        Label endIf(bc());
        addInsn(BC_ILOAD0);

        if (node->elseBlock()) {
            addBranch(BC_IFICMPE, elseIf);
            node->thenBlock()->visit(this);
            addBranch(BC_JA, endIf);
            bind(elseIf);
            node->elseBlock()->visit(this);
        } else {
            addBranch(BC_IFICMPE, endIf);
            node->thenBlock()->visit(this);
        }
        bind(endIf);
    }

    // Block visitor helpers

    void declareFunction(AstFunction* func) {
        BytecodeFunction* bcFunction = new BytecodeFunction(func);
        _code->addFunction(bcFunction);
        _scope->declareFunction(func->name(), bcFunction);
    }

    void processFunction(AstFunction* func) {
        BytecodeFunction* savedFunction = _bcFunction;
        _bcFunction = _scope->resolveFunction(func->name());
        func->node()->visit(this);
        _bcFunction = savedFunction;
    }

    void declareFunctions(Scope* astScope) {
        Scope::FunctionIterator funciter(astScope);
        while (funciter.hasNext()) {
            declareFunction(funciter.next());
        }
    }

    void processFunctions(Scope* astScope) {
        Scope::FunctionIterator funciter(astScope);
        while (funciter.hasNext()) {
            processFunction(funciter.next());
        }
    }

    void declareVariables(Scope* astScope, VarScope* varScope) {
        Scope::VarIterator variter(astScope);

        while (variter.hasNext()) {
            AstVar* ptr = variter.next();
            varScope->declareVar(ptr->name(), ptr->type(), _bcFunction->id());
            _bcFunction->setLocalsNumber(_bcFunction->localsNumber() + 1);
        }
    }

    void visitBlockNode(BlockNode* node) {

        Scope* astScope = node->scope();

        uint32_t size = node->nodes();

        VarScope* varScope = constructScope(_scope, _scope->varCount());

        declareVariables(astScope, varScope);

        VarScope* savedScope = _scope;
        _scope = varScope;

        declareFunctions(astScope);
        processFunctions(astScope);

        for (uint32_t i = 0; i < size; ++i) {
            node->nodeAt(i)->visit(this);
        }

        _scope = savedScope;
    }

    void visitPrintNode(PrintNode* node) {
        size_t nodes = node->operands();
        for (size_t i = 0; i < nodes; ++i) {
            node->operandAt(i)->visit(this);
            switch (_lastType) {
                case VT_INT:
                    addInsn(BC_IPRINT);
                    break;
                case VT_DOUBLE:
                    addInsn(BC_DPRINT);
                    break;
                case VT_STRING:
                    addInsn(BC_SPRINT); 
                    break;
                default: throw error("Unknown type. ");
            }
        }
    }

    void visitCallNode(CallNode* node) {
        size_t params = node->parametersNumber();

        TranslatedFunction* f = _scope->resolveFunction(node->name());

        if (params != f->parametersNumber()) {
            throw error("Invalid call. ");
        }

        if (params != 0) {
            for (int i = params - 1; i >= 0; --i) {
                node->parameterAt(i)->visit(this);
                VarType type = f->parameterType(i);
                addDirectCast(_lastType, type);
                checkVarType(type , _lastType);
            }
        }

        addInsn(BC_CALL);
        addUInt16(f->id());

        _lastType = f->returnType();
    }

    void visitReturnNode(ReturnNode* node) {
        if (node->returnExpr()) {
            node->returnExpr()->visit(this);
            VarType type = _bcFunction->returnType();
            addDirectCast(_lastType, type);
            checkVarType(type, _lastType);
        }
        addInsn(BC_RETURN);
    }

    void visitNativeCallNode(NativeCallNode* node) {
        // FIXME Stub code only 
        uint16_t nativeId = _code->makeNativeFunction(node->nativeName(), 
                                                      node->nativeSignature(), 0);
        addInsn(BC_CALLNATIVE);
        addUInt16(nativeId);
    }

    void visitFunctionNode(FunctionNode* node) {

        VarScope* saveScope = _scope;

        VarScope* varscope = constructScope(_scope, 0);

        _bcFunction->setScopeId(_scopes.size() - 1);

        _scope = varscope;

        size_t params = node->parametersNumber();

        for (size_t i = 0; i < params; ++i) {
            _scope->declareVar(node->parameterName(i), node->parameterType(i), _bcFunction->id());
            addStoreVarInsn(_scope->resolveName(node->parameterName(i)));
        }

        node->body()->visit(this);

        if (_bcFunction->id() == 0) {
            bc()->addInsn(BC_RETURN);
        }
        
        _scope = saveScope;
    }

};

Status* BytecodeTranslatorImpl::translate(const string& source, Code** code) {
    Parser parser;

    if (Status* s = parser.parseProgram(source)) {
        return s;
    }
    
    AstFunction* main = parser.top();

    TranslatorVisitor visitor(main, *code);

    try {
        main->node()->visit(&visitor);
    } catch (exception& e) {
        return new Status(e.what());
    }

    return 0;
    
}
