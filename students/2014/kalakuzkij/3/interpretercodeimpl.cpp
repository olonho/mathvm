#include "interpretercodeimpl.h"
#include <vector>
using std::vector;
#include "mathvm.h"
#include <iostream>
using std::cout;
#include "placeholder.h"
#include <stdexcept>
using std::runtime_error;


namespace mathvm{
    Status *InterpreterCodeImpl::execute(vector<Var *> &vars) {
        stack_types.clear();
        stack_types.reserve(256);

        //this->disassemble();
        running = true;

        BytecodeFunction * f = (BytecodeFunction *) functionById(0);
        eval_func(f);
        return Status::Ok();
    }
    Bytecode * InterpreterCodeImpl::init_function(BytecodeFunction* bf, vector<BytecodeFunction*>& bf_stack)
    {
        bf_stack.push_back(bf);

        scope_vars.newScope();
        //locals
        AbstractVarContext::VarIterator vi(&a_vars, bf->scopeId());
        while(vi.hasNext()){
            string name;
            VarType type;
            int id;
            vi.next(id, name, type);
            scope_vars.addVar(id, new Var(type, name));
        }
        //parameters
        for(int i = bf->parametersNumber() - 1; i >= 0; --i){
            int vid = get_var_by_name(bf->parameterName(i), bf->scopeId());
            switch(bf->parameterType(i)){
            case VT_INT:
                scope_vars.getVar(vid)->setIntValue(_stack.get<int64_t>());
                break;
            case VT_DOUBLE:
                scope_vars.getVar(vid)->setDoubleValue(_stack.get<double>());
                break;
            case VT_STRING:{
                int sid = _stack.get<int16_t>();
                const char * tmp = ptrmap.get(sid);//new char[constantById(sid).size()+1];
                //strcpy(tmp, constantById(sid).c_str());
                scope_vars.getVar(vid)->setStringValue(tmp);
                }
                break;
            default:
                break;
            }
            stack_types.pop_back();
        }
        Bytecode* bc = bf->bytecode();

        return bc;
    }

    void InterpreterCodeImpl::eval_func(BytecodeFunction* bf){
        vector<BytecodeFunction*> bf_stack;
        vector<size_t> bci_stack;

        Bytecode* bc = init_function(bf, bf_stack);
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
                _stack.add<double>(b-a);
                stack_types.pop_back();
                break;
            }
            case BC_ISUB:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(b-a);
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
                _stack.add<double>(b/a);
                stack_types.pop_back();
                break;
            }
            case BC_IDIV:{
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(b/a);
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
                break;
            }
            case BC_INEG:{
                int64_t a = _stack.get<int64_t>();
                _stack.add<int64_t>(-a);
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
                cout << a;
                break;
            }
            case BC_DPRINT:{
                double a = _stack.get<double>();
                stack_types.pop_back();
                cout << a;
                break;
            }
            case BC_SPRINT:{
                uint16_t sid = _stack.get<int16_t>();
                stack_types.pop_back();
                cout << ptrmap.get(sid);//this->constantById(sid);
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
            case BC_S2I:{
                int sid = _stack.get<int16_t>();
                stack_types.pop_back();
                const char * sptr = ptrmap.get(sid);
                _stack.add<int64_t>(reinterpret_cast<int64_t>(sptr));
                stack_types.push_back(VT_INT);
            }
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

                stack_types.pop_back();
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
                _stack.add<double>(scope_vars.getVar(bc->getInt16(bci + 1))->getDoubleValue());
                stack_types.push_back(VT_DOUBLE);
                break;
            case BC_LOADIVAR://, "Load int from variable, whose 2-byte id is inlined to insn stream, push on TOS.", 3)
                _stack.add<int64_t>(scope_vars.getVar(bc->getInt16(bci + 1))->getIntValue());
                stack_types.push_back(VT_INT);
                break;
            case BC_LOADSVAR:{//, "Load string from variable, whose 2-byte id is inlined to insn stream, push on TOS.", 3)
                int vid = ptrmap.add(scope_vars.getVar(bc->getInt16(bci + 1))->getStringValue());
                _stack.add<int16_t>(vid);
                stack_types.push_back(VT_STRING);
                break;
            }
            case BC_STOREDVAR://, "Pop TOS and store to double variable, whose 2-byte id is inlined to insn stream.", 3)
                scope_vars.getVar(bc->getInt16(bci + 1))->setDoubleValue(_stack.get<double>());
                stack_types.pop_back();
                break;
            case BC_STOREIVAR://, "Pop TOS and store to int variable, whose 2-byte id is inlined to insn stream.", 3)
                scope_vars.getVar(bc->getInt16(bci + 1))->setIntValue(_stack.get<int64_t>());
                stack_types.pop_back();
                break;
            case BC_STORESVAR:{//, "Pop TOS and store to string variable, whose 2-byte id is inlined to insn stream.", 3)
                int sid = _stack.get<int16_t>();
                const char * tmp = ptrmap.get(sid);
                scope_vars.getVar(bc->getInt16(bci + 1))->setStringValue(tmp);
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
                length = bc->getInt16(bci+1)+1;
                break;
            case BC_IFICMPNE://, "Compare two topmost integers and jump if upper != lower, next two bytes - signed offset of jump destination.", 3)
            {
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(b);
                _stack.add<int64_t>(a);
                if (a != b){
                    length = bc->getInt16(bci+1)+1;
                }

                break;
            }
            case BC_IFICMPE://, "Compare two topmost integers and jump if upper == lower, next two bytes - signed offset of jump destination.", 3)
            {
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(b);
                _stack.add<int64_t>(a);
                if (a == b){
                    length = bc->getInt16(bci+1)+1;
                }
                break;
            }
            case BC_IFICMPG://, "Compare two topmost integers and jump if upper > lower, next two bytes - signed offset of jump destination.", 3)
            {
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(b);
                _stack.add<int64_t>(a);
                if (a > b){
                    length = bc->getInt16(bci+1)+1;
                }
                break;
            }
            case BC_IFICMPGE://, "Compare two topmost integers and jump if upper >= lower, next two bytes - signed offset of jump destination.", 3)
            {
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(b);
                _stack.add<int64_t>(a);
                if (a >= b){
                    length = bc->getInt16(bci+1)+1;
                }
                break;
            }
            case BC_IFICMPL://, "Compare two topmost integers and jump if upper < lower, next two bytes - signed offset of jump destination.", 3)
            {
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(b);
                _stack.add<int64_t>(a);
                if (a < b){
                    length = bc->getInt16(bci+1)+1;
                }
                break;
            }
            case BC_IFICMPLE://, "Compare two topmost integers and jump if upper <= lower, next two bytes - signed offset of jump destination.", 3)
            {
                int64_t a = _stack.get<int64_t>();
                int64_t b = _stack.get<int64_t>();
                _stack.add<int64_t>(b);
                _stack.add<int64_t>(a);
                if (a <= b){
                    length = bc->getInt16(bci+1)+1;
                }
                break;
            }
            case BC_DUMP://, "Dump value on TOS, without removing it.", 1)
                //it's like print. It's needless
                break;
            case BC_STOP://, "Stop execution.", 1)
                running = false;
                break;
            case BC_CALL://, "Call function, next two bytes - unsigned function id.", 3)
            {
                int fid = bc->getInt16(bci+1);
                bci_stack.push_back(bci+length);
                length = 0;
                bci = 0;
                bc = init_function((BytecodeFunction*)functionById(fid), bf_stack);
                //eval_func((BytecodeFunction*)functionById(fid));
                break;
            }
            case BC_CALLNATIVE://, "Call native function, next two bytes - id of the native function.", 3)
            {
                int fid = bc->getInt16(bci+1);
                Signature const * signature;
                string const * name;
                const void * proxy_addr = nativeById(fid, &signature, &name);

                VarType returnType = (*signature)[0].first;

                vector<PlaceHolder> args;
                args.push_back(PlaceHolder(0.0));
                for (uint i = 1; i < signature->size(); ++i){

                    switch((*signature)[i].first){
                    case VT_DOUBLE:{
                        double val = scope_vars.getVar(this->get_var_by_name((*signature)[i].second, nativeScopeIds[fid]))->getDoubleValue();
                        args.push_back(PlaceHolder(val));
                        break;
                    }
                    case VT_STRING:{
                        const char * val = scope_vars.getVar(this->get_var_by_name((*signature)[i].second, nativeScopeIds[fid]))->getStringValue();
                        args.push_back(PlaceHolder(reinterpret_cast<int64_t>(val)));
                        break;
                    }
                    case VT_INT:{

                        int64_t val = scope_vars.getVar(this->get_var_by_name((*signature)[i].second, nativeScopeIds[fid]))->getIntValue();
                        args.push_back(PlaceHolder(val));
                        break;
                    }
                    default:
                        throw runtime_error("Bad type in native function signature");
                        break;
                    }
                }

                switch(returnType) {
                    case VT_DOUBLE:
                    {
                        double result = reinterpret_cast<double (*)(PlaceHolder *)>(proxy_addr)(&args[0]);
                        _stack.add<double>(result);
                        stack_types.push_back(VT_DOUBLE);
                        break;
                    }
                    case VT_INT:
                    case VT_STRING:
                    case VT_VOID:
                    {
                        int64_t result = reinterpret_cast<int64_t (*)(PlaceHolder *)>(proxy_addr)(&args[0]);
                        if (returnType == VT_STRING){
                            int id = ptrmap.add((char*) result);
                            _stack.add<int16_t>(id);
                            stack_types.push_back(VT_STRING);
                        }else
                        if (returnType != VT_VOID){
                            _stack.add<int64_t>(result);
                            stack_types.push_back(VT_INT);
                        }
                        break;
                    }
                    default:
                        throw runtime_error("Bad return type in native function signature");
                        return;
                }
                break;
            }
            case BC_RETURN://, "Return to call location", 1)
                scope_vars.back();
                bci = bci_stack.back();
                length = 0;
                bf_stack.pop_back();
                bc = bf_stack.back()->bytecode();
                bci_stack.pop_back();
                break;
            case BC_BREAK:// , "Breakpoint for the debugger.", 1)
                break;
            }
            bci += length;
        }

        return;
    }
}
