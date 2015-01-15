namespace mathvm{
    namespace Interpriter{
        class StackItem{
        union{
            int64_t i;
            double d;
            const char * c;
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

        static StackItem fromConstCharPtr(const char * val){
            StackItem stackItem;
            stackItem._item.c = val;
            return stackItem;
        }

        int64_t asInt(){
            return _item.i;
        }

        double asDouble(){
            return _item.d;
        }

        const char * asConstCharPtr(){
            return _item.c;
        }
    };

        class Scope{
            BytecodeFunction * _fn;
            uint16_t _id;
            std::vector<StackItem> _context;
            Scope * _parent;
            Scope * _clojure_parent;
            uint64_t instructionPointer;
        public:

            Scope(BytecodeFunction * fn, Scope * parent, Scope * clojure_parent):
            _fn(fn),
            _id(fn->id()),
            _context(fn->localsNumber()),
            _parent(parent),
            _clojure_parent(clojure_parent),
            instructionPointer(0){}

            Scope * parent(){
                return _parent;
            }

            Scope * clojure_parent(){
                return _clojure_parent;
            }

            BytecodeFunction * fn(){
                return _fn;
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

            StackItem getVarById(uint16_t id){
                return _context[id];
            }

            inline StackItem getCtxVar(uint16_t var_id, uint16_t context_id){
                Scope * curr_scope = this;
                while(context_id != curr_scope->_id){
                    curr_scope = curr_scope->_clojure_parent;
                }
                return curr_scope->_context[var_id];
            }


            inline void setCtxVar(uint16_t var_id, uint16_t context_id, StackItem val){
                Scope * curr_scope = this;
                while(context_id != curr_scope->_id){
                    curr_scope = curr_scope->_clojure_parent;
                }
                curr_scope->_context[var_id] = val;
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
