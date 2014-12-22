#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include <stdexcept>
#include <iostream>

namespace mathvm{

    struct TVar {

      uint16_t id;

      uint16_t cId;

      TVar(uint16_t id, uint16_t cId): id(id), cId(cId){}

    };

    class ScopeData {
    public:
      BytecodeFunction *function;

      ScopeData *parentScope;

      std::map<std::string, uint16_t> myVariables;

      int nlocals;

      ScopeData(BytecodeFunction *function, ScopeData *parentScope = NULL): function(function), parentScope(parentScope), nlocals(0){}

      uint16_t id(){
        return function->id();
      }

      void addAstVariable(const AstVar *astVar){
        ++nlocals;
        myVariables.insert(std::make_pair(astVar->name(), myVariables.size()));
      }

      TVar findVariable(const AstVar *var){

        if(myVariables.find(var->name()) != myVariables.end()) {
          return TVar(myVariables.at(var->name()), id());

        } else if(parentScope){
            return parentScope->findVariable(var);

        } else throw std::runtime_error("Variable not found");
      }

      uint16_t variablesCount(){
        return myVariables.size();
      }

    };


    Instruction MY_PRINT(VarType type){
      switch(type){

        case VT_INT: return BC_IPRINT;

        case VT_DOUBLE: return BC_DPRINT;

        case VT_STRING: return BC_SPRINT;

        default: throw std::runtime_error("Unprintable type");
      }
    }

    Instruction MY_COMPARE(VarType type){
      switch(type){

        case VT_INT: return BC_ICMP;

        case VT_DOUBLE: return BC_DCMP;

        default: throw std::runtime_error("Uncompareable types");
      }
    }

    Instruction MY_NEGATE(VarType type){
        switch(type){

        case VT_INT: return BC_INEG;

        case VT_DOUBLE: return BC_DNEG;

        default: throw std::runtime_error("Unloadable type");
        }
    }

    Instruction MY_LOAD_VARIABLE(VarType type){
        switch(type){

        case VT_INT: return BC_LOADIVAR;

        case VT_DOUBLE: return BC_LOADDVAR;

        case VT_STRING: return BC_LOADSVAR;

        default: throw std::runtime_error("Unloadable type");
        }
    }

    Instruction MY_LOAD_CONTEXT_VARIABLE(VarType type){
        switch(type){

        case VT_INT: return BC_LOADCTXIVAR;

        case VT_DOUBLE: return BC_LOADCTXDVAR;

        case VT_STRING: return BC_LOADCTXSVAR;

        default: throw std::runtime_error("Unloadable type");
        }
    }

    Instruction MY_STORE_VARIABLE(VarType type){
        switch(type){

        case VT_INT: return BC_STOREIVAR;

        case VT_DOUBLE: return BC_STOREDVAR;

        case VT_STRING: return BC_STORESVAR;

        default: throw std::runtime_error("Unstoreable type");
        }
    }

    Instruction MY_STORE_CONTEXT_VARIABLE(VarType type){
        switch(type){

        case VT_INT: return BC_STORECTXIVAR;

        case VT_DOUBLE: return BC_STORECTXDVAR;

        case VT_STRING: return BC_STORECTXSVAR;

        default: throw std::runtime_error("Unstoreable type");
        }
    }

    Instruction MY_ADD(VarType type){
        switch(type){

        case VT_INT: return BC_IADD;

        case VT_DOUBLE: return BC_DADD;

        default: throw std::runtime_error("Cannot add these types");
        }
    }

    Instruction MY_SUBTRACT(VarType type){
        switch(type){

        case VT_INT: return BC_ISUB;

        case VT_DOUBLE: return BC_DSUB;

        default: throw std::runtime_error("Cannot add these types");
        }
    }

    Instruction MY_MULTIPLY(VarType type){
        switch(type){

        case VT_INT: return BC_IMUL;

        case VT_DOUBLE: return BC_DMUL;

        default: throw std::runtime_error("Cannot add these types");
        }
    }

    Instruction MY_DIVIDE(VarType type){
        switch(type){

        case VT_INT: return BC_IDIV;

        case VT_DOUBLE: return BC_DDIV;

        default: throw std::runtime_error("Cannot add these types");
        }
    }

    class BCVisitor: public AstBaseVisitor{
	    public:

	    BCVisitor(){}

      Status * translateToBytecode(Code *code, AstFunction *top) {
          myCode = code;
          BytecodeFunction *bytecodeFunction = new BytecodeFunction(top);
          myCode->addFunction(bytecodeFunction);
          currentScope = new ScopeData(bytecodeFunction);

          try {
              Scope *topScope = top->scope()->childScopeAt(0);
              initVars(topScope);
              initFunctions(topScope);

              for (size_t i = 0; i < top->node()->body()->nodes(); ++i){
                  top->node()->body()->nodeAt(i)->visit(this);
              }

              bytecodeFunction->setLocalsNumber(currentScope->nlocals);
              getByteCode()->addInsn(BC_STOP);
              return Status::Ok();
          }
          catch (std::runtime_error &err) {
              return Status::Error(err.what());
          }
      }

	    void visitBinaryOpNode(BinaryOpNode * node){
		    switch(node->kind()){

            case tAND: case tOR:
                addBoolOp(node); break;

            case tAOR: case tAAND: case tAXOR:
                addBitOp(node); break;

            case tADD: case tSUB: case tMUL: case tDIV:
                addArithmOp(node); break;

            case tEQ: case tNEQ: case tGT: case tLT: case tGE: case tLE:
                addComparison(node); break;

            case tMOD:
                addModOp(node); break;

            default:
                throw std::runtime_error("Invalid binop type"); break;
        }
	    }

      void visitUnaryOpNode(UnaryOpNode * node){
          node->operand()->visit(this);
          switch (node->kind()) {

              case tNOT:
                  getByteCode()->addInsn(BC_ILOAD1);
                  getByteCode()->addInsn(BC_IAXOR);
                  break;

              case tSUB:
                  getByteCode()->addInsn(MY_NEGATE(TOSType)); break;

              default:
                  throw std::runtime_error("Bad kind");
          }
      }

      void visitIfNode(IfNode * node){
          Label le(getByteCode());
          node->ifExpr()->visit(this);

          if (node->elseBlock()){
              Label lElse(getByteCode());
              getByteCode()->addInsn(BC_ILOAD0);
              getByteCode()->addBranch(BC_IFICMPE, lElse);
              node->thenBlock()->visit(this);

              getByteCode()->addBranch(BC_JA, le);
              getByteCode()->bind(lElse);
              node->elseBlock()->visit(this);

          } else {
              getByteCode()->addInsn(BC_ILOAD0);
              getByteCode()->addBranch(BC_IFICMPE, le);
              node->thenBlock()->visit(this);
          }

          getByteCode()->bind(le);
      }

      void visitForNode(ForNode * node){
          Label ls(getByteCode()), le(getByteCode());

          BinaryOpNode *inExpr = node->inExpr()->asBinaryOpNode();
          inExpr->left()->visit(this);
          storeVar(node->var());

          getByteCode()->bind(ls);
          loadVar(node->var());
          inExpr->right()->visit(this);

          getByteCode()->addBranch(BC_IFICMPL, le);
          node->body()->visit(this);

          loadVar(node->var());
          getByteCode()->addInsn(BC_ILOAD1);
          getByteCode()->addInsn(BC_IADD);
          storeVar(node->var());
          getByteCode()->addBranch(BC_JA, ls);
          getByteCode()->bind(le);
      }

      void visitWhileNode(WhileNode * node){
          Label ls(getByteCode()), le(getByteCode());
          getByteCode()->bind(ls);

          node->whileExpr()->visit(this);
          getByteCode()->addInsn(BC_ILOAD0);
          getByteCode()->addBranch(BC_IFICMPE, le);

          node->loopBlock()->visit(this);
          getByteCode()->addBranch(BC_JA, ls);
          getByteCode()->bind(le);
      }

      void visitIntLiteralNode(IntLiteralNode * node){
          TOSType = VT_INT;
          getByteCode()->addInsn(BC_ILOAD);
          getByteCode()->addInt64(node->literal());
      }

      void visitDoubleLiteralNode(DoubleLiteralNode * node){
          TOSType = VT_DOUBLE;
          getByteCode()->addInsn(BC_DLOAD);
          getByteCode()->addDouble(node->literal());
      }

      void visitStringLiteralNode(StringLiteralNode * node){
          TOSType = VT_STRING;
          getByteCode()->addInsn(BC_SLOAD);
          getByteCode()->addUInt16(myCode->makeStringConstant(node->literal()));
      }

      void visitPrintNode(PrintNode * node){
         for(unsigned int i = 0; i < node->operands(); ++i){
             node->operandAt(i)->visit(this);
             getByteCode()->addInsn(MY_PRINT(TOSType));
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
                  getByteCode()->addInsn(MY_ADD(TOSType)); break;

              case tDECRSET:
                  loadVar(node->var());
                  getByteCode()->addInsn(MY_SUBTRACT(TOSType)); break;

              default: break;
          }
          storeVar(node->var());
      }


      void visitFunctionNode(FunctionNode *node){
          BytecodeFunction *bcFn = (BytecodeFunction*) myCode->functionByName(node->name());
          bool isNative = node->body()->nodes() != 0 && node->body()->nodeAt(0)->isNativeCallNode();
          ScopeData scope(bcFn, currentScope);
          currentScope = &scope;

          for (size_t i = 0; i < node->parametersNumber(); ++i) {
              AstVar *var = node->body()->scope()->lookupVariable(node->parameterName(i));
              currentScope->addAstVariable(var);
              storeVar(var);
          }

          if (isNative) node->body()->nodeAt(0)->visit(this);
          else node->body()->visit(this);

          bcFn->setLocalsNumber(currentScope->nlocals);
          currentScope = currentScope->parentScope;
      }

      void visitReturnNode(ReturnNode *node) {
          if (node->returnExpr()) {
              node->returnExpr()->visit(this);
              castTos(currentScope->function->returnType());
          }

          getByteCode()->addInsn(BC_RETURN);
      }

      void visitCallNode(CallNode *node)
      {
          BytecodeFunction *function = (BytecodeFunction*) myCode->functionByName(node->name());
          if (!function) throw std::runtime_error("Function not declared");

          size_t paramsNum = node->parametersNumber();
          if (paramsNum != function->parametersNumber()) throw std::runtime_error("Incorrect number of arguments");

          for (size_t i = paramsNum; paramsNum > 0 && i > 0; --i) {
              node->parameterAt(i-1)->visit(this);
              castTos(function->parameterType(i-1));
          }

          getByteCode()->addInsn(BC_CALL);
          getByteCode()->addUInt16(function->id());

          if (function->returnType() != VT_VOID)
              TOSType = function->returnType();
      }

      void visitNativeCallNode(NativeCallNode *node){
          //oops...
      }

      void visitBlockNode(BlockNode * node){
          initVars(node->scope());
          initFunctions(node->scope());
          visitAllNodesInBlock(node);
      }

	private:
      VarType TOSType;
      ScopeData * currentScope;
      Code *myCode;
      Bytecode *getByteCode(){
          return currentScope->function->bytecode();
      }

      void visitAllNodesInBlock(BlockNode *node){
          for (size_t i = 0; i < node->nodes(); ++i)
              node->nodeAt(i)->visit(this);
      }

      void addComparison(BinaryOpNode * node){
          node->right()->visit(this);
          VarType rType = TOSType;
          node->left()->visit(this);
          binOpTypeCast(TOSType, rType);
          getByteCode()->addInsn(MY_COMPARE(TOSType));

          switch(node->kind()){

              case tGT:
                  getByteCode()->addInsn(BC_INEG);

              case tLT:
                  getByteCode()->addInsn(BC_ILOAD);
                  getByteCode()->addInt64(-2);
                  getByteCode()->addInsn(BC_IAAND);
                  getByteCode()->addInsn(BC_ILOAD);
                  getByteCode()->addInt64(-2);
                  getByteCode()->addInsn(BC_SWAP);
                  getByteCode()->addInsn(BC_IDIV);
                  break;

              case tLE:
                  getByteCode()->addInsn(BC_INEG);

              case tGE:
                  getByteCode()->addInsn(BC_ILOAD);
                  getByteCode()->addInt64(-2);
                  getByteCode()->addInsn(BC_IAAND);
                  getByteCode()->addInsn(BC_ILOAD);
                  getByteCode()->addInt64(-2);
                  getByteCode()->addInsn(BC_SWAP);
                  getByteCode()->addInsn(BC_IDIV);
                  getByteCode()->addInsn(BC_ILOAD1);
                  getByteCode()->addInsn(BC_IAXOR);
                  break;

              case tEQ:
                  getByteCode()->addInsn(BC_ILOAD1);
                  getByteCode()->addInsn(BC_IAXOR);

              case tNEQ:
                  getByteCode()->addInsn(BC_ILOAD1);
                  getByteCode()->addInsn(BC_IAAND);
                  break;

              default:
                  throw std::runtime_error("Bad comparison kind");
          }
      }

      void addArithmOp(BinaryOpNode *node){
          node->right()->visit(this);
          VarType rType = TOSType;
          node->left()->visit(this);

          binOpTypeCast(TOSType, rType);
          switch(node->kind()){

              case tADD:
                  getByteCode()->addInsn(MY_ADD(TOSType));
                  break;

              case tSUB:
                  getByteCode()->addInsn(MY_SUBTRACT(TOSType));
                  break;

              case tMUL:
                  getByteCode()->addInsn(MY_MULTIPLY(TOSType));
                  break;

              case tDIV:
                  getByteCode()->addInsn(MY_DIVIDE(TOSType));
                  break;

              default: throw std::runtime_error("Unknown arithmetic operation");
          }
      }

      void addBoolOp(BinaryOpNode * node){
          Label lsr(getByteCode()), le(getByteCode());
          node->left()->visit(this);

          switch(node->kind()){

              case tAND:
                  getByteCode()->addInsn(BC_ILOAD0); break;

              case tOR:
                  getByteCode()->addInsn(BC_ILOAD1); break;

              default:
                  throw  std::runtime_error("No such boolean operation");
          }

          getByteCode()->addBranch(BC_IFICMPE, lsr);
          node->right()->visit(this);
          getByteCode()->addBranch(BC_JA, le);
          getByteCode()->bind(lsr);

          switch(node->kind()){

              case tAND:
                  getByteCode()->addInsn(BC_ILOAD0); break;

              case tOR:
                  getByteCode()->addInsn(BC_ILOAD1); break;

              default:
                  throw  std::runtime_error("No such boolean operation");
          }
          getByteCode()->bind(le);
      }

      void addBitOp(BinaryOpNode * node){
          node->right()->visit(this);
          node->left()->visit(this);
          switch(node->kind()){

              case tAOR:
                  getByteCode()->addInsn(BC_IAOR); break;

              case tAAND:
                  getByteCode()->addInsn(BC_IAAND); break;

              case tAXOR:
                  getByteCode()->addInsn(BC_IAXOR); break;

              default:
                  throw  std::runtime_error("Bad variable type");
          }
      }

      void addModOp(BinaryOpNode * node){
          node->right()->visit(this);
          if(TOSType != VT_INT) throw std::runtime_error("Bad variable type");
          node->left()->visit(this);
          if(TOSType != VT_INT) throw std::runtime_error("Bad variable type");
          getByteCode()->addInsn(BC_IMOD);
      }

      void binOpTypeCast(VarType lType, VarType rType){
          switch(lType){

              case VT_INT:
                  switch(rType){

                      case VT_INT:
                          return;

                      case VT_DOUBLE:
                          TOSType = VT_DOUBLE;
                          getByteCode()->addInsn(BC_I2D);
                          return;

                      default:
                          throw std::logic_error("Wrong types");
                  }
              case VT_DOUBLE:
                  switch(rType){

                      case VT_INT:
                          TOSType = VT_DOUBLE;
                          getByteCode()->addInsn(BC_SWAP);
                          getByteCode()->addInsn(BC_I2D);
                          getByteCode()->addInsn(BC_SWAP);
                          return;

                      case VT_DOUBLE:
                          return;

                      default:
                          throw std::logic_error("Wrong types");
                  }
              default:
                  throw std::logic_error("Wrong types");
          }
      }

      void castTos(VarType type){
          switch(TOSType){

              case VT_INT:
                  switch(type){

                      case VT_INT:
                          return;

                      case VT_DOUBLE:
                          getByteCode()->addInsn(BC_I2D);
                          TOSType = VT_DOUBLE;
                          return;

                      default:
                          throw std::runtime_error("Bad TOS cast");
                  }
                  break;

              case VT_DOUBLE:
                  switch(type){

                      case VT_INT:
                          getByteCode()->addInsn(BC_D2I);
                          TOSType = VT_INT;
                          return;

                      case VT_DOUBLE:
                          return;

                      default:
                          throw std::runtime_error("Bad TOS cast");
                  }
                  break;

              default:
                  throw std::runtime_error("Bad TOS cast");
          }
      }

      void initVars(Scope *scope){
          Scope::VarIterator it(scope);
          while (it.hasNext())
              currentScope->addAstVariable(it.next());
      }

      void initFunctions(Scope * scope){
          Scope::FunctionIterator iter(scope);

          while (iter.hasNext()) {
              AstFunction *function = iter.next();
              BytecodeFunction *bcFn = (BytecodeFunction*) myCode->functionByName(function->name());

              if (!bcFn) {
                  bcFn = new BytecodeFunction(function);
                  myCode->addFunction(bcFn);
              }
          }

          iter = Scope::FunctionIterator(scope);
          while (iter.hasNext())
              iter.next()->node()->visit(this);
      }

      void loadVar(const AstVar * node){
          TVar var = currentScope->findVariable(node);
          TOSType = node->type();
          if (var.cId == currentScope->id()) {
              getByteCode()->addInsn(MY_LOAD_VARIABLE(TOSType));
              getByteCode()->addUInt16(var.id);
          } else {
              getByteCode()->addInsn(MY_LOAD_CONTEXT_VARIABLE(TOSType));
              getByteCode()->addUInt16(var.cId);
              getByteCode()->addUInt16(var.id);
          }
      }

      void storeVar(const AstVar *astVar){
          TVar var = currentScope->findVariable(astVar);
          if (var.cId != currentScope->id()) {
              getByteCode()->addInsn(MY_STORE_CONTEXT_VARIABLE(astVar->type()));
              getByteCode()->addUInt16(var.cId);
              getByteCode()->addUInt16(var.id);
          } else {
            getByteCode()->addInsn(MY_STORE_VARIABLE(astVar->type()));
            getByteCode()->addUInt16(var.id);
          }
      }

      void initVariables(Scope * scope){
          Scope::VarIterator it(scope);
          while (it.hasNext()) currentScope->addAstVariable(it.next());
      }

  		string escapeString(string const & s){
  			string res;
  			for(unsigned int i = 0; i < s.size(); ++i){
  				switch(s.at(i)){

    					case '\n':
                  res.append("\\n");
                  break;

    					case '\t':
                  res += "\\t";
                  break;

    					case '\r':
                  res += "\\r";
                  break;

    					case '\\':
                  res += "\\\\";
                  break;

    					case '\'':
                  res += "\\'";
                  break;

    					default:
                  res += s.at(i);
                  break;
  				}
  			}
  			return res;
  		}
    };

    class BCTranslator : public Translator {

    public:
        virtual Status* translate(const string& program, Code ** code)  {

            Parser parser;
            Status* status = parser.parseProgram(program);

            if (status->isError())
                return status;
            BCVisitor visitor;
            status = visitor.translateToBytecode(*code, parser.top());
            return status;
        }

        ~BCTranslator(){}
    };

    Translator* Translator::create(const string& impl_str) {
       if (impl_str == "bytecode") {
         return new BCTranslator();
       } else {
          return NULL;
       }
    }

}
