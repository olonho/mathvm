namespace mathvm{

namespace Interpreter{

  typedef uint64_t iptr_t;

  class SVal{
        union {int64_t i; double d; uint16_t u;} values;

    public:

        SVal(){

        }

        static SVal saveI(int64_t val){
            SVal sValueHolder;
            sValueHolder.values.i = val;
            return sValueHolder;
        }

        static SVal saveD(double val){
            SVal sValueHolder;
            sValueHolder.values.d = val;
            return sValueHolder;
        }

        static SVal saveU(uint16_t val){
            SVal sValueHolder;
            sValueHolder.values.u = val;
            return sValueHolder;
        }

        int64_t i(){
            return values.i;
        }

        double d(){
            return values.d;
        }

        uint16_t u(){
            return values.u;
        }
  };

  class ScopeData{

        std::vector<SVal> ctx;
        BytecodeFunction * bcFun;
        ScopeData * parentScope;
        iptr_t iPtr;

        public:

          ScopeData(BytecodeFunction * fn, ScopeData * parent = NULL):
          ctx(fn->localsNumber()),
          bcFun(fn),
          parentScope(parent),
          iPtr(0){}

          SVal getContextVariable(){
              uint16_t cId = getUint();
              uint16_t vId = getUint();
              return getContextVariable(vId, cId);
          }

          void setContextVariable(SVal val){
              uint16_t vId = getUint();
              uint16_t cId = getUint();
              setContextVariable(vId, cId, val);
          }

          void setVar(SVal val){
              uint16_t vId = getUint();
              ctx.at(vId) = val;
          }

          ScopeData * parent(){
            return parentScope;
          }

          Instruction getBytecodeInstruction(){
            return bcFun->bytecode()->getInsn(iPtr++);
          }

          SVal getVariable(){
            return ctx.at(getUint());
          }

          Bytecode * bytecode(){
              return bcFun->bytecode();
          }

          void jump(int16_t offset){
              iPtr+=offset;
          }

          uint16_t getUint(){
            uint16_t val = bcFun->bytecode()->getUInt16(iPtr);
            iPtr += sizeof(uint16_t);
            return val;
          }

          int16_t getInt16(){
            int16_t val = bcFun->bytecode()->getInt16(iPtr);
            iPtr += sizeof(int16_t);
            return val;
          }

          int64_t getInt(){
            int64_t val = bcFun->bytecode()->getInt64(iPtr);
            iPtr += sizeof(int64_t);
            return val;
          }

          double getDouble(){
            double val = bcFun->bytecode()->getDouble(iPtr);
            iPtr += sizeof(double);
            return val;
          }


      private:
          SVal getContextVariable(uint16_t vId, uint16_t cId){
              if(cId == bcFun->id()){
                  return ctx.at(vId);
              } else if(parentScope == 0){
                  throw "";
              } else{
                  return parentScope->getContextVariable(vId, cId);
              }
          }

          void setContextVariable(uint16_t vId, uint16_t cId, SVal val){
              if(cId == bcFun->id()){
                  ctx.at(vId) = val;
              } else if(parentScope == NULL){
                  throw "Invalid context var ID";
              } else{
                  parentScope->setContextVariable(vId, cId, val);
              }
          }
  };

//namespace Interpreter
}


    class ExecutableCode: public Code{

    public:
        Status * execute(vector<Var*>& vars);

    private:
        Interpreter::ScopeData * currentScope;

        Bytecode * bytecode(){
            return currentScope->bytecode();
        }
    };

//namespace mathvm
}
