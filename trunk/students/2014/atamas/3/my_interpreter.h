namespace mathvm{
    namespace Interpriter{
        class StackItem{
        union{
            int64_t i;
            double d;
            uint16_t u;
        } _item;
    public:
        StackItem(){}
        static StackItem fromInt(int64_t val){
            StackItem stackItem;
            stackItem._item.i = val;
            return stackItem;
        }

        static StackItem fromDouble(double val){
            StackItem stackItem;
            stackItem._item.d = val;
            return stackItem;
        }

        static StackItem fromUint(uint16_t val){
            StackItem stackItem;
            stackItem._item.u = val;
            return stackItem;
        }

        int64_t asInt(){
            return _item.i;
        }

        double asDouble(){
            return _item.d;
        }

        uint16_t asUint(){
            return _item.u;
        }
    };

        class Scope{
            BytecodeFunction * _fn;
            std::vector<StackItem> _context;
            Scope * _parent;
            uint64_t instructionPointer;
        public:

            Scope(BytecodeFunction * fn, Scope * parent = NULL):
            _fn(fn),
            _context(fn->localsNumber()),
            _parent(parent),
            instructionPointer(0){}

            Scope * parent(){
                return _parent;
            }

            Instruction getInstruction(){
                return _fn->bytecode()->getInsn(instructionPointer++);
            }

            uint16_t getUint(){
                uint16_t val = _fn->bytecode()->getUInt16(instructionPointer);
                instructionPointer += sizeof(uint16_t);
                return val;
            }

            int16_t getInt16(){
                int16_t val = _fn->bytecode()->getInt16(instructionPointer);
                instructionPointer += sizeof(int16_t);
                return val;
            }

            int64_t getInt(){
                int64_t val = _fn->bytecode()->getInt64(instructionPointer);
                instructionPointer += sizeof(int64_t);
                return val;
            }

            double getDouble(){
                double val = _fn->bytecode()->getDouble(instructionPointer);
                instructionPointer += sizeof(double);
                return val;
            }

            StackItem getVar(){
                return _context[getUint()];
            }

            StackItem getCtxVar(){
                uint16_t ctx_id = getUint();
                uint16_t var_id = getUint();
                return getCtxVar(var_id, ctx_id);
            }

            void setCtxVar(StackItem val){
                uint16_t ctx_id = getUint();
                uint16_t var_id = getUint();
                setCtxVar(var_id, ctx_id, val);
            }

            void setVar(StackItem val){
                uint16_t var_id = getUint();
                _context[var_id] = val;
            }

            Bytecode * bytecode(){
                return _fn->bytecode();
            }

            void jump(int16_t offset){
                instructionPointer+=offset;
            }
        private:
            StackItem getCtxVar(uint16_t var_id, uint16_t context_id){
                if(context_id == _fn->id()){
                    return _context[var_id];
                } else if(_parent == 0){
                    throw "Not found";
                } else{
                    return _parent->getCtxVar(var_id, context_id);
                }
            }

            void setCtxVar(uint16_t var_id, uint16_t context_id, StackItem val){
                if(context_id == _fn->id()){
                    _context[var_id] = val;
                } else if(_parent == NULL){
                    throw "Not found";
                } else{
                    _parent->setCtxVar(var_id, context_id, val);
                }
            }
        };
    }

    class ExecutableCode: public Code{
    public:
        Status * execute(vector<Var*>& vars);
    private:
        Interpriter::Scope * currentScope;
        Bytecode * bytecode(){
            return currentScope->bytecode();
        }
    };
}
