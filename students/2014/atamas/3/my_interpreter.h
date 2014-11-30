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
            void setVar(StackItem val){
                _context[getUint()] = val;
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
