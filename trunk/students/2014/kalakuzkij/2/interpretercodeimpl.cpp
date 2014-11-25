#include "interpretercodeimpl.h"
#include <vector>
using std::vector;
#include "mathvm.h"


namespace mathvm{
    Status *InterpreterCodeImpl::execute(vector<Var *> &vars) {
        this->disassemble();
        running = true;

        BytecodeFunction * f = (BytecodeFunction *) functionById(0);
        eval_func(f);
        return Status::Ok();
    }
    void InterpreterCodeImpl::eval_func(BytecodeFunction* bf){
        //parameters
        scope_vars.push_back(map<int16_t, Var*>());
//        for(int i = 0; i < bf->parametersNumber(); ++i){
//            int vid = get_var_by_name(bf->parameterName(i));
//            scope_vars.back()[vid] = new Var(bf->parameterType(i), bf->parameterName(i));
//        }
        //locals
        AbstractVarContext::VarIterator vi(&a_vars, bf->scopeId());
        while(vi.hasNext()){
            string name;
            VarType type;
            int id;
            vi.next(id, name, type);
            scope_vars.back()[id] = new Var(type, name);
        }
        Bytecode* bc = bf->bytecode();

        for (size_t bci = 0; bci < bc->length() && running;) {
            size_t length;
            Instruction insn = bc->getInsn(bci);
            bytecodeName(insn, &length);
            switch (insn) {
            case BC_LAST:
                break;
            case BC_INVALID:
                break;
            case BC_DLOAD:{
                _stack.add<double>(bc->getDouble(bci+1));
                stack_types.push_back(VT_DOUBLE);
                break;
            }
            case BC_ILOAD:{
                _stack.add<int64_t>(bc->getInt64(bci+1));
                stack_types.push_back(VT_INT);
                break;
            }
            case BC_SLOAD:{
                _stack.add<int16_t>(bc->getInt16(bci+1));
                stack_types.push_back(VT_STRING);
                break;
            }
            case BC_DLOAD0:{
                _stack.add<double>(0.0);
                stack_types.push_back(VT_DOUBLE);
                break;
            }
            case BC_ILOAD0:{
                _stack.add<int64_t>(0);
                stack_types.push_back(VT_INT);
                break;
            }
            case BC_SLOAD0:{
                _stack.add<int16_t>(0);
                stack_types.push_back(VT_STRING);
                break;
            }
            case BC_DLOAD1:{
                _stack.add<double>(1.0);
                stack_types.push_back(VT_DOUBLE);
                break;
            }
            case BC_ILOAD1:{
                _stack.add<int64_t>(1);
                stack_types.push_back(VT_INT);
                break;
            }
            case BC_DLOADM1:{
                _stack.add<double>(-1.0);
                stack_types.push_back(VT_DOUBLE);
                break;
            }
            case BC_ILOADM1:{
                _stack.add<int64_t>(-1.0);
                stack_types.push_back(VT_INT);
                break;
            }
            case BC_DADD:{
                double a = _stack.get<double>();
                double b = _stack.get<double>();
                _stack.add<double>(a+b);
                stack_types.pop_back();
                break;
            }
            case BC_IADD:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(a+b);
                stack_types.pop_back();
                break;
            }
            case BC_DSUB:{
                double a = _stack.get<double>();
                double b = _stack.get<double>();
                _stack.add<double>(a-b);
                stack_types.pop_back();
                break;
            }
            case BC_ISUB:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(a-b);
                stack_types.pop_back();
                break;
            }
            case BC_DMUL:{
                double a = _stack.get<double>();
                double b = _stack.get<double>();
                _stack.add<double>(a*b);
                stack_types.pop_back();
                break;
            }
            case BC_IMUL:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(a*b);
                stack_types.pop_back();
                break;
            }
            case BC_DDIV:{
                double a = _stack.get<double>();
                double b = _stack.get<double>();
                _stack.add<double>(a/b);
                stack_types.pop_back();
                break;
            }
            case BC_IDIV:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(a/b);
                stack_types.pop_back();
                break;
            }
            case BC_IMOD:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(a%b);
                stack_types.pop_back();
                break;
            }
            case BC_DNEG:{
                double a = _stack.get<double>();
                _stack.add<double>(-a);
                stack_types.pop_back();
                break;
            }
            case BC_INEG:{
                int64_t a = _stack.get<int64_t>();
                _stack.add<int64_t>(-a);
                stack_types.pop_back();
                break;
            }
            case BC_IAOR:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(a|b);
                stack_types.pop_back();
                break;
            }
            case BC_IAAND:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(a&b);
                stack_types.pop_back();
                break;
            }
            case BC_IAXOR:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(a^b);
                stack_types.pop_back();
                break;
            }
            case BC_IPRINT:{
                int64_t a = _stack.get<int64_t>();
                stack_types.pop_back();
                printf("%ld", a);
                break;
            }
            case BC_DPRINT:{
                double a = _stack.get<double>();
                stack_types.pop_back();
                printf("%F", a);
                break;
            }
            case BC_SPRINT:{
                int16_t sid = _stack.get<int16_t>();
                stack_types.pop_back();
                printf("%s", this->constantById(sid).c_str());
                break;
            }
            case BC_I2D:{
                int64_t a = _stack.get<int64_t>();
                stack_types.pop_back();
                _stack.add<double>(double(a));
                stack_types.push_back(VT_DOUBLE);
                break;
            }
            case BC_D2I:{
                double a = _stack.get<double>();
                stack_types.pop_back();
                _stack.add<int64_t>(int64_t(a));
                stack_types.push_back(VT_INT);
                break;
            }
            case BC_S2I:
                //ignore
                break;
            case BC_SWAP:{
                if (stack_types.back() == VT_INT){
                    stack_types.pop_back();
                    int64_t a = _stack.get<int64_t>();
                    if (stack_types.back() == VT_DOUBLE){
                        stack_types.pop_back();
                        double b = _stack.get<double>();
                        _stack.add<int64_t>(a);
                        _stack.add<double>(b);
                        stack_types.push_back(VT_INT);
                        stack_types.push_back(VT_DOUBLE);
                    }else if (stack_types.back() == VT_INT){
                        int64_t b = _stack.get<int64_t>();
                        _stack.add<int64_t>(a);
                        _stack.add<int64_t>(b);
                        stack_types.push_back(VT_INT);
                    }else{
                        assert(1 == 0);
                    }
                }else if (stack_types.back() == VT_DOUBLE){
                    stack_types.pop_back();
                    double a = _stack.get<double>();
                    if (stack_types.back() == VT_DOUBLE){
                        double b = _stack.get<double>();
                        _stack.add<double>(a);
                        _stack.add<double>(b);
                        stack_types.push_back(VT_DOUBLE);
                    }else if (stack_types.back() == VT_INT){
                        stack_types.pop_back();
                        int64_t b = _stack.get<int64_t>();
                        _stack.add<double>(a);
                        _stack.add<int64_t>(b);
                        stack_types.push_back(VT_DOUBLE);
                        stack_types.push_back(VT_INT);
                    }else {
                        assert(1 == 0);
                    }
                }else {
                    assert(1 == 0);
                }

                break;
            }
            case BC_POP:{
                switch (stack_types.back()) {
                case VT_INT:
                    _stack.get<int64_t>();
                    break;
                case VT_STRING:
                    _stack.get<int16_t>();
                    break;
                case VT_DOUBLE:
                    _stack.get<double>();
                    break;
                default:
                    break;
                }
                break;
            }
            case BC_LOADDVAR0://, "Load double from variable 0, push on TOS.", 1)
            case BC_LOADDVAR1://, "Load double from variable 1, push on TOS.", 1)
            case BC_LOADDVAR2://, "Load double from variable 2, push on TOS.", 1)
            case BC_LOADDVAR3://, "Load double from variable 3, push on TOS.", 1)
            case BC_LOADIVAR0://, "Load int from variable 0, push on TOS.", 1)
            case BC_LOADIVAR1://, "Load int from variable 1, push on TOS.", 1)
            case BC_LOADIVAR2://, "Load int from variable 2, push on TOS.", 1)
            case BC_LOADIVAR3://, "Load int from variable 3, push on TOS.", 1)
            case BC_LOADSVAR0://, "Load string from variable 0, push on TOS.", 1)
            case BC_LOADSVAR1://, "Load string from variable 1, push on TOS.", 1)
            case BC_LOADSVAR2://, "Load string from variable 2, push on TOS.", 1)
            case BC_LOADSVAR3://, "Load string from variable 3, push on TOS.", 1)
            case BC_STOREDVAR0://, "Pop TOS and store to double variable 0.", 1)
            case BC_STOREDVAR1://, "Pop TOS and store to double variable 1.", 1)
            case BC_STOREDVAR2://, "Pop TOS and store to double variable 0.", 1)
            case BC_STOREDVAR3://, "Pop TOS and store to double variable 3.", 1)
            case BC_STOREIVAR0://, "Pop TOS and store to int variable 0.", 1)
            case BC_STOREIVAR1://, "Pop TOS and store to int variable 1.", 1)
            case BC_STOREIVAR2://, "Pop TOS and store to int variable 0.", 1)
            case BC_STOREIVAR3://, "Pop TOS and store to int variable 3.", 1)
            case BC_STORESVAR0://, "Pop TOS and store to string variable 0.", 1)
            case BC_STORESVAR1://, "Pop TOS and store to string variable 1.", 1)
            case BC_STORESVAR2://, "Pop TOS and store to string variable 0.", 1)
            case BC_STORESVAR3://, "Pop TOS and store to string variable 3.", 1)
            case BC_LOADDVAR://, "Load double from variable, whose 2-byte is id inlined to insn stream, push on TOS.", 3)
                _stack.add<double>(scope_vars.back()[bc->getInt16(bci + 1)]->getDoubleValue());
                stack_types.push_back(VT_DOUBLE);
                break;
            case BC_LOADIVAR://, "Load int from variable, whose 2-byte id is inlined to insn stream, push on TOS.", 3)
                _stack.add<int64_t>(scope_vars.back()[bc->getInt16(bci + 1)]->getIntValue());
                stack_types.push_back(VT_INT);
                break;
            case BC_LOADSVAR:{//, "Load string from variable, whose 2-byte id is inlined to insn stream, push on TOS.", 3)
                int vid = this->makeStringConstant(scope_vars.back()[bc->getInt16(bci + 1)]->getStringValue());
                _stack.add<int16_t>(vid);
                stack_types.push_back(VT_STRING);
                break;
            }
            case BC_STOREDVAR://, "Pop TOS and store to double variable, whose 2-byte id is inlined to insn stream.", 3)
                scope_vars.back()[bc->getInt16(bci + 1)]->setDoubleValue(_stack.get<double>());
                stack_types.pop_back();
                break;
            case BC_STOREIVAR://, "Pop TOS and store to int variable, whose 2-byte id is inlined to insn stream.", 3)
                scope_vars.back()[bc->getInt16(bci + 1)]->setIntValue(_stack.get<int64_t>());
                stack_types.pop_back();
                break;
            case BC_STORESVAR:{//, "Pop TOS and store to string variable, whose 2-byte id is inlined to insn stream.", 3)
                int sid = _stack.get<int16_t>();
                scope_vars.back()[bc->getInt16(bci + 1)]->setStringValue(constantById(sid).c_str());
                stack_types.pop_back();
                break;
            }
            case BC_LOADCTXDVAR://, "Load double from variable, whose 2-byte context and 2-byte id inlined to insn stream, push on TOS.", 5)
            case BC_LOADCTXIVAR://, "Load int from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS.", 5)
            case BC_LOADCTXSVAR://, "Load string from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS.", 5)
            case BC_STORECTXDVAR://, "Pop TOS and store to double variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5)
            case BC_STORECTXIVAR://, "Pop TOS and store to int variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5)
            case BC_STORECTXSVAR://, "Pop TOS and store to string variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5)
                break;
            case BC_DCMP:{
                double a = _stack.get<double>();
                double b = _stack.get<double>();
                stack_types.pop_back();
                stack_types.pop_back();

                if (a < b){
                    _stack.add<int64_t>(-1);
                }else if (a > b){
                    _stack.add<int64_t>(1);
                }else{
                    _stack.add<int64_t>(0);
                }
                stack_types.push_back(VT_INT);
                break;
            }
            case BC_ICMP:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                stack_types.pop_back();

                if (a < b){
                    _stack.add<int64_t>(-1);
                }else if (a > b){
                    _stack.add<int64_t>(1);
                }else{
                    _stack.add<int64_t>(0);
                }
                break;
            }
            case BC_JA://, "Jump always, next two bytes - signed offset of jump destination.", 3)
            case BC_IFICMPNE://, "Compare two topmost integers and jump if upper != lower, next two bytes - signed offset of jump destination.", 3)
            case BC_IFICMPE://, "Compare two topmost integers and jump if upper == lower, next two bytes - signed offset of jump destination.", 3)
            case BC_IFICMPG://, "Compare two topmost integers and jump if upper > lower, next two bytes - signed offset of jump destination.", 3)
            case BC_IFICMPGE://, "Compare two topmost integers and jump if upper >= lower, next two bytes - signed offset of jump destination.", 3)
            case BC_IFICMPL://, "Compare two topmost integers and jump if upper < lower, next two bytes - signed offset of jump destination.", 3)
            case BC_IFICMPLE://, "Compare two topmost integers and jump if upper <= lower, next two bytes - signed offset of jump destination.", 3)
                break;
            case BC_DUMP://, "Dump value on TOS, without removing it.", 1)
                //it's like print. It's needless
                break;
            case BC_STOP://, "Stop execution.", 1)
                running = false;
                break;
            case BC_CALL://, "Call function, next two bytes - unsigned function id.", 3)
            {
                int fid = bc->getInt16(bci+1);
                eval_func((BytecodeFunction*)functionById(fid));
                break;
            }
            case BC_CALLNATIVE://, "Call native function, next two bytes - id of the native function.", 3)
            case BC_RETURN://, "Return to call location", 1)
                return;
            case BC_BREAK:// , "Breakpoint for the debugger.", 1)
                break;
            }
            bci += length;
        }
        return;
    }
}
