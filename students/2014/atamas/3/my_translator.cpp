#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include <stdexcept>
#include <iostream>

namespace mathvm{
    Instruction BC_NEG(VarType type){
        switch(type){
        case VT_INT: return BC_INEG;
        case VT_DOUBLE: return BC_DNEG;
        default: throw std::runtime_error("Can't load this type");
        }
    }

    Instruction BC_LOADVAR(VarType type){
        switch(type){
        case VT_INT: return BC_LOADIVAR;
        case VT_DOUBLE: return BC_LOADDVAR;
        case VT_STRING: return BC_LOADSVAR;
        default: throw std::runtime_error("Can't load this type");
        }
    }

    Instruction BC_LOADCTXVAR(VarType type){
        switch(type){
        case VT_INT: return BC_LOADCTXIVAR;
        case VT_DOUBLE: return BC_LOADCTXDVAR;
        case VT_STRING: return BC_LOADCTXSVAR;
        default: throw std::runtime_error("Can't load this type");
        }
    }

    Instruction BC_STOREVAR(VarType type){
        switch(type){
        case VT_INT: return BC_STOREIVAR;
        case VT_DOUBLE: return BC_STOREDVAR;
        case VT_STRING: return BC_STORESVAR;
        default: throw std::runtime_error("Can't store this type");
        }
    }

    Instruction BC_STORECTXVAR(VarType type){
        switch(type){
        case VT_INT: return BC_STORECTXIVAR;
        case VT_DOUBLE: return BC_STORECTXDVAR;
        case VT_STRING: return BC_STORECTXSVAR;
        default: throw std::runtime_error("Can't store this type");
        }
    }

    Instruction BC_ADD(VarType type){
        switch(type){
        case VT_INT: return BC_IADD;
        case VT_DOUBLE: return BC_DADD;
        default: throw std::runtime_error("Can't add this types");
        }
    }

    Instruction BC_SUB(VarType type){
        switch(type){
        case VT_INT: return BC_ISUB;
        case VT_DOUBLE: return BC_DSUB;
        default: throw std::runtime_error("Can't add this types");
        }
    }

    Instruction BC_MUL(VarType type){
        switch(type){
        case VT_INT: return BC_IMUL;
        case VT_DOUBLE: return BC_DMUL;
        default: throw std::runtime_error("Can't add this types");
        }
    }

    Instruction BC_DIV(VarType type){
        switch(type){
        case VT_INT: return BC_IDIV;
        case VT_DOUBLE: return BC_DDIV;
        default: throw std::runtime_error("Can't add this types");
        }
    }

    Instruction BC_PRINT(VarType type){
        switch(type){
        case VT_INT: return BC_IPRINT;
        case VT_DOUBLE: return BC_DPRINT;
        case VT_STRING: return BC_SPRINT;
        default: throw std::runtime_error("Can't print this type");
        }
    }

    Instruction BC_CMP(VarType type){
        switch(type){
        case VT_INT: return BC_ICMP;
        case VT_DOUBLE: return BC_DCMP;
        default: throw std::runtime_error("Can't compare this types");
        }
    }

    struct TVar {
    uint16_t id;
    uint16_t contextId;
    TVar(uint16_t id, uint16_t contextId)
        : id(id), contextId(contextId)
        {}
    };

    class TScope {
    public:
        BytecodeFunction *fn;
        TScope *parent;
        std::map<std::string, uint16_t> vars;
        int locals_number;

        TScope(BytecodeFunction *fn, TScope *parent = NULL)
            : fn(fn), parent(parent), locals_number(0)
            {}

        uint16_t id()
            { return fn->id(); }

        void addVar(const AstVar *var){
            ++locals_number;
            vars.insert(std::make_pair(var->name(), vars.size()));
        }

        TVar findVar(const AstVar *var){
            if(vars.find(var->name()) != vars.end()) return TVar(vars[var->name()], id());
            else if(parent) return parent->findVar(var);
            else throw std::runtime_error("Var not found");
        }

        uint16_t variablesCount(){
            return vars.size();
        }
    };

    class BytecodeVisitor: public AstBaseVisitor{
	public:

	    BytecodeVisitor(){}

        Status * translateToBytecode(Code *code, AstFunction *top){
            _code = code;

            BytecodeFunction *bytecodeFunction = new BytecodeFunction(top);
            _code->addFunction(bytecodeFunction);
            _curScope = new TScope(bytecodeFunction);

            try {
                Scope *topScope = top->scope()->childScopeAt(0);
                initVars(topScope);
                initFunctions(topScope);
                //genBlock(top->node()->body());
                for (size_t i = 0; i < top->node()->body()->nodes(); ++i)
                    top->node()->body()->nodeAt(i)->visit(this);
                bytecodeFunction->setLocalsNumber(_curScope->locals_number);
                bc()->addInsn(BC_STOP);
                return Status::Ok();
            } catch (std::runtime_error &err) {
                return Status::Error(err.what());
            }
        }
	
	    void visitBinaryOpNode(BinaryOpNode * node){
		    switch(node->kind()){
            case tADD: case tSUB: case tMUL: case tDIV:
                addArithmeticOperation(node); break;
            case tEQ: case tNEQ: case tGT: case tLT: case tGE: case tLE:
                addComparitionOperation(node); break;
            case tAND: case tOR:
                addBooleanOperation(node); break;
            case tAOR: case tAAND: case tAXOR:
                addBitwiseOperation(node); break;
            case tMOD:
                addModuloOperation(node); break;
            default:
                throw std::runtime_error("Invalide type of binary operation");
                break;
            }
	    }

        void visitUnaryOpNode(UnaryOpNode * node){
            node->operand()->visit(this);
            switch (node->kind()) {
                case tNOT:
                    bc()->addInsn(BC_ILOAD1);
                    bc()->addInsn(BC_IAXOR);
                    break;
                case tSUB:
                    bc()->addInsn(BC_NEG(_tosType)); break;
                default:
                    throw std::runtime_error("Bad kind");
            }
        }

        void visitIfNode(IfNode * node){
            Label lEnd(bc());
            node->ifExpr()->visit(this);
            if (node->elseBlock()){
                Label lElse(bc());
                bc()->addInsn(BC_ILOAD0);
                bc()->addBranch(BC_IFICMPE, lElse);
                node->thenBlock()->visit(this);
                bc()->addBranch(BC_JA, lEnd);
                bc()->bind(lElse);
                node->elseBlock()->visit(this);
            } else{
                bc()->addInsn(BC_ILOAD0);
                bc()->addBranch(BC_IFICMPE, lEnd);
                node->thenBlock()->visit(this);
            }
            bc()->bind(lEnd);
        }

        void visitForNode(ForNode * node){
            Label lBegin(bc()), lEnd(bc());

            BinaryOpNode *inExpr = node->inExpr()->asBinaryOpNode();
            inExpr->left()->visit(this);
            storeVar(node->var());

            bc()->bind(lBegin);
            loadVar(node->var());
            inExpr->right()->visit(this);
            bc()->addBranch(BC_IFICMPL, lEnd);
            node->body()->visit(this);
            loadVar(node->var());
            bc()->addInsn(BC_ILOAD1);
            bc()->addInsn(BC_IADD);
            storeVar(node->var());
            bc()->addBranch(BC_JA, lBegin);
            bc()->bind(lEnd);
        }

        void visitWhileNode(WhileNode * node){
            Label lBegin(bc()), lEnd(bc());
            bc()->bind(lBegin);
            node->whileExpr()->visit(this);
            bc()->addInsn(BC_ILOAD0);
            bc()->addBranch(BC_IFICMPE, lEnd);
            node->loopBlock()->visit(this);
            bc()->addBranch(BC_JA, lBegin);
            bc()->bind(lEnd);
        }

        void visitIntLiteralNode(IntLiteralNode * node){
            _tosType = VT_INT;
            bc()->addInsn(BC_ILOAD);
            bc()->addInt64(node->literal());
        }

        void visitDoubleLiteralNode(DoubleLiteralNode * node){
            _tosType = VT_DOUBLE;
            bc()->addInsn(BC_DLOAD);
            bc()->addDouble(node->literal());
        }

        void visitStringLiteralNode(StringLiteralNode * node){
            _tosType = VT_STRING;
            bc()->addInsn(BC_SLOAD);
            bc()->addUInt16(_code->makeStringConstant(node->literal()));
        }

        void visitPrintNode(PrintNode * node){
           for(unsigned int i = 0; i < node->operands(); ++i){
               node->operandAt(i)->visit(this);
               bc()->addInsn(BC_PRINT(_tosType));
           }
        }

        void visitLoadNode(LoadNode * node){
            loadVar(node->var());
        }


        void visitStoreNode(StoreNode * node){
            node->value()->visit(this);
            castTos(node->var()->type());
            switch(node->op()){
            case tINCRSET:
                loadVar(node->var());
                bc()->addInsn(BC_ADD(_tosType));
            break;
            case tDECRSET:
                loadVar(node->var());
                bc()->addInsn(BC_SUB(_tosType));
            break;
            default: break;
            }
            storeVar(node->var());
        }


        void visitFunctionNode(FunctionNode *node){
            BytecodeFunction *bcFn = (BytecodeFunction*) _code->functionByName(node->name());
            bool isNative = node->body()->nodes() > 0
                         && node->body()->nodeAt(0)->isNativeCallNode();

            TScope scope(bcFn, _curScope);
            _curScope = &scope;

            for (size_t i = 0; i < node->parametersNumber(); ++i) {
                AstVar *var = node->body()->scope()->lookupVariable(node->parameterName(i));
                _curScope->addVar(var);
                storeVar(var);
            }

            if (isNative)
                node->body()->nodeAt(0)->visit(this);
            else
                node->body()->visit(this);
            bcFn->setLocalsNumber(_curScope->locals_number);
            _curScope = _curScope->parent;
        }

        void visitReturnNode(ReturnNode *node) {
            if (node->returnExpr()) {
                node->returnExpr()->visit(this);
                castTos(_curScope->fn->returnType());
            }
            bc()->addInsn(BC_RETURN);
        }

        void visitCallNode(CallNode *node)
        {
            BytecodeFunction *fn = (BytecodeFunction*) _code->functionByName(node->name());
            if (!fn) throw std::runtime_error("Function wasn't declarated");

            size_t paramsNum = node->parametersNumber();
            if (paramsNum != fn->parametersNumber()) throw std::runtime_error("Worng number of arguments");

            for (size_t i = paramsNum; paramsNum > 0 && i > 0; --i) {
                node->parameterAt(i-1)->visit(this);
                castTos(fn->parameterType(i-1));
            }

            bc()->addInsn(BC_CALL);
            bc()->addUInt16(fn->id());

            if (fn->returnType() != VT_VOID)
                _tosType = fn->returnType();
        }

        void visitNativeCallNode(NativeCallNode *node){
//            void *code = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
//            if (!code) throw std::runtime_error("Native function not found");
//            uint16_t fnId = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), code);
//            bc()->addInsn(BC_CALLNATIVE);
//            bc()->addUInt16(fnId);
//            bc()->addInsn(BC_RETURN);
        }

        void visitBlockNode(BlockNode * node){
            initVars(node->scope());
            initFunctions(node->scope());
            genBlock(node);
        }

	private:
        VarType _tosType;
        TScope * _curScope;
        Code *_code;
        Bytecode *bc(){
            return _curScope->fn->bytecode();
        }

        void genBlock(BlockNode *node){
            for (size_t i = 0; i < node->nodes(); ++i)
                node->nodeAt(i)->visit(this);
        }

        void addComparitionOperation(BinaryOpNode * node){
            node->right()->visit(this);
            VarType rhsType = _tosType;
            node->left()->visit(this);
            binOpTypeCast(_tosType, rhsType);
            bc()->addInsn(BC_CMP(_tosType));
            switch(node->kind()){
            case tEQ:
                bc()->addInsn(BC_ILOAD1);
                bc()->addInsn(BC_IAXOR);
            case tNEQ:
                bc()->addInsn(BC_ILOAD1);
                bc()->addInsn(BC_IAAND);
                break;
            case tGT:
                bc()->addInsn(BC_INEG);
            case tLT:
                bc()->addInsn(BC_ILOAD);
                bc()->addInt64(-2);
                bc()->addInsn(BC_IAAND);
                bc()->addInsn(BC_ILOAD);
                bc()->addInt64(-2);
                bc()->addInsn(BC_SWAP);
                bc()->addInsn(BC_IDIV);
                break;
            case tLE:
                bc()->addInsn(BC_INEG);
            case tGE:
                bc()->addInsn(BC_ILOAD);
                bc()->addInt64(-2);
                bc()->addInsn(BC_IAAND);
                bc()->addInsn(BC_ILOAD);
                bc()->addInt64(-2);
                bc()->addInsn(BC_SWAP);
                bc()->addInsn(BC_IDIV);
                bc()->addInsn(BC_ILOAD1);
                bc()->addInsn(BC_IAXOR);
                break;
            default: throw std::runtime_error("Bad kind");
            }
        }

        void addArithmeticOperation(BinaryOpNode *node){
            node->right()->visit(this);
            VarType rhsType = _tosType;
            node->left()->visit(this);

            binOpTypeCast(_tosType, rhsType);
            switch(node->kind()){
            case tADD:
                bc()->addInsn(BC_ADD(_tosType));
                break;
            case tSUB:
                bc()->addInsn(BC_SUB(_tosType));
                break;
            case tMUL:
                bc()->addInsn(BC_MUL(_tosType));
                break;
            case tDIV:
                bc()->addInsn(BC_DIV(_tosType));
                break;
            default: throw std::runtime_error("No such arithmetic operation.");
            }
        }

        void addBooleanOperation(BinaryOpNode * node){
            Label lSkipRight(bc()), lEnd(bc());
            node->left()->visit(this);
            switch(node->kind()){
            case tAND: bc()->addInsn(BC_ILOAD0); break;
            case tOR: bc()->addInsn(BC_ILOAD1); break;
            default: throw  std::runtime_error("No such boolean operation.");
            }
            bc()->addBranch(BC_IFICMPE, lSkipRight);
            node->right()->visit(this);
            bc()->addBranch(BC_JA, lEnd);
            bc()->bind(lSkipRight);
            switch(node->kind()){
            case tAND: bc()->addInsn(BC_ILOAD0); break;
            case tOR: bc()->addInsn(BC_ILOAD1); break;
            default: throw  std::runtime_error("No such boolean operation.");
            }
            bc()->bind(lEnd);
        }

        void addBitwiseOperation(BinaryOpNode * node){
            node->right()->visit(this);
            node->left()->visit(this);
            switch(node->kind()){
            case tAOR: bc()->addInsn(BC_IAOR); break;
            case tAAND: bc()->addInsn(BC_IAAND); break;
            case tAXOR: bc()->addInsn(BC_IAXOR); break;
            default: throw  std::runtime_error("Bad type");
            }
        }

        void addModuloOperation(BinaryOpNode * node){
            node->right()->visit(this);
            if(_tosType != VT_INT) throw std::runtime_error("Bad type");
            node->left()->visit(this);
            if(_tosType != VT_INT) throw std::runtime_error("Bad type");
            bc()->addInsn(BC_IMOD);
        }

        void binOpTypeCast(VarType lhsType, VarType rhsType){
            switch(lhsType){
                case VT_INT:
                    switch(rhsType){
                        case VT_INT: return;
                        case VT_DOUBLE:
                            _tosType = VT_DOUBLE;
                            bc()->addInsn(BC_I2D);
                            return;
                        default:
                            throw std::logic_error("Wrong types");
                    }
                case VT_DOUBLE:
                    switch(rhsType){
                        case VT_INT:
                            _tosType = VT_DOUBLE;
                            bc()->addInsn(BC_SWAP);
                            bc()->addInsn(BC_I2D);
                            bc()->addInsn(BC_SWAP);
                            return;
                        case VT_DOUBLE: return;
                        default:
                            throw std::logic_error("Wrong types");
                    }
                default:
                    throw std::logic_error("Wrong types");
            }
        }

        void castTos(VarType type){
            switch(_tosType){
            case VT_INT:
                switch(type){
                case VT_INT: return;
                case VT_DOUBLE:
                    bc()->addInsn(BC_I2D);
                    _tosType = VT_DOUBLE;
                    return;
                default: throw std::runtime_error("Bad cast");
                }
                break;
            case VT_DOUBLE:
                switch(type){
                case VT_INT:
                    bc()->addInsn(BC_D2I);
                    _tosType = VT_INT;
                    return;
                case VT_DOUBLE: return;
                default: throw std::runtime_error("Bad cast");
                }
                break;
            default: throw std::runtime_error("Bad cast");
            }
        }

        void initVars(Scope *scope){
            Scope::VarIterator it(scope);
            while (it.hasNext())
                _curScope->addVar(it.next());
        }

        void initFunctions(Scope * scope){
            Scope::FunctionIterator it(scope);
            while (it.hasNext()) {
                AstFunction *fn = it.next();
                BytecodeFunction *bcFn = (BytecodeFunction*) _code->functionByName(fn->name());
                if (!bcFn) {
                    bcFn = new BytecodeFunction(fn);
                    _code->addFunction(bcFn);
                }
            }

            it = Scope::FunctionIterator(scope);
            while (it.hasNext())
                it.next()->node()->visit(this);
        }

        void loadVar(const AstVar * node){
            TVar var = _curScope->findVar(node);
            _tosType = node->type();
            if (var.contextId == _curScope->id()) {
                bc()->addInsn(BC_LOADVAR(_tosType));
                bc()->addUInt16(var.id);
            } else{
                bc()->addInsn(BC_LOADCTXVAR(_tosType));
                bc()->addUInt16(var.contextId);
                bc()->addUInt16(var.id);
            }
        }

        void storeVar(const AstVar *astVar){
            TVar var = _curScope->findVar(astVar);
            if (var.contextId == _curScope->id()) {
                bc()->addInsn(BC_STOREVAR(astVar->type()));
                bc()->addUInt16(var.id);
            } else {
                bc()->addInsn(BC_STORECTXVAR(astVar->type()));
                bc()->addUInt16(var.contextId);
                bc()->addUInt16(var.id);
            }
        }

        void initVariables(Scope * scope){
            Scope::VarIterator it(scope);
            while (it.hasNext()) _curScope->addVar(it.next());
        }

		string escapeString(string const & s){
			string res;
			for(unsigned int i = 0; i < s.size(); ++i){
				switch(s[i]){
					case '\n': res += "\\n";break;	
					case '\t': res += "\\t";break;
					case '\r': res += "\\r";break;
					case '\\': res += "\\\\";break;
					case '\'': res += "\\'";break;
					default: res += s[i];break;
				}
			}
			return res;
		}
    };

    class MyBytecodeTranslatorImpl : public Translator {
      public:
        virtual Status* translate(const string& program, Code ** code)  {
            Parser parser;
            Status* status = parser.parseProgram(program);
            if (status->isError())
                return status;

            BytecodeVisitor visitor;

            status = visitor.translateToBytecode(*code, parser.top());
            return status;
        }

        ~MyBytecodeTranslatorImpl(){}
    };

    Translator* Translator::create(const string& impl) {
       if (impl == "bytecode") {
         return new MyBytecodeTranslatorImpl();
       } else {
          return NULL;
       }
    }
}
