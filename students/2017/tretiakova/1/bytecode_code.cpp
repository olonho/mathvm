#include "../include/ast.h"
#include "../include/mathvm.h"
#include "../include/visitors.h"

#include "include/bytecode_code.h"

#include <vector>
#include <stack>
#include <map>
#include <iostream>
#include <fstream>
#include <string>

namespace mathvm {

int BytecodeCode::get_scope_id(Scope* scope) {
    if(!scope_map.count(scope)) {
        return -1;
    }
    return scope_map[scope];
}

pair<int, int> BytecodeCode::get_var_id(Scope* scope, string name) {
    Scope* cur = scope;
    while(cur && !var_map[cur].count(name)) {
        cur = cur->parent();
    }
    if(!cur) {
        return make_pair(-1, -1);
    }
    // (scope, var)
    int scope_id = get_scope_id(cur);
    assert(scope_id >= 0);
    return make_pair(scope_id, var_map[cur][name]);
}

int BytecodeCode::add_var(Scope* scope, VarType type, string name) {
    int scope_id = get_scope_id(scope);
    if(scope_id < 0) {
        return scope_id;
    }

    if(!var_map[scope].count(name)) {
        uint16_t var_id = var_map[scope].size();
        if(var_id != var_by_scope[scope_id].size()) {
            cerr << "*** MISMATCH: "
                 << typeToName(type) << " "
                 << name << " at scope "
                 << scope << " with id "
                 << scope_id << " got var id "
                 << var_id << " while size is "
                 << var_by_scope[scope_id].size()
                 << endl;
            if(var_map[scope].size() > (1<<16) - 1) {
                cerr << "Variable id pool overflow" << endl;
            }
        }
        assert(var_id == var_by_scope[scope_id].size());
        var_map[scope][name] = var_id;
        var_by_scope[scope_id].emplace_back(type, name);

        switch(type) {
        case VT_INT:
            var_by_scope[scope_id][var_id].setIntValue(0);
            break;
        case VT_DOUBLE:
            var_by_scope[scope_id][var_id].setDoubleValue(0);
            break;
        case VT_STRING:
            var_by_scope[scope_id][var_id].setStringValue("");
            break;
        default:
            break;
        }

        return var_id;
    }
    return var_map[scope][name];
}

uint16_t BytecodeCode::add_scope(Scope* scope) {
    if(!scope_map.count(scope)) {
        uint16_t scope_id = (uint16_t)scope_map.size();
        scope_map[scope] = scope_id;
        assert(scope_id == scopes.size());
        scopes.push_back(scope);
        var_map[scope] = VarNameMap();
        var_by_scope.push_back(vector<LocalVar>());

        for(int i = 0; i < (int)scope->childScopeNumber(); ++i) {
            add_scope(scope->childScopeAt(i));
        }

        for(Scope::VarIterator it(scope); it.hasNext();) {
            AstVar* v = it.next();
            add_var(scope, v->type(), v->name());
        }

        for(Scope::FunctionIterator fit(scope); fit.hasNext();) {
            AstFunction* f = fit.next();
            StackFrame* sf = new StackFrame(f);
            uint16_t f_id = addFunction(sf);
            if(f->name() == "<top>") {
                cerr << "top-top" << endl;
                top_function_id = f_id;
            }
        }

        return scope_id;
    }

    return scope_map[scope];
}

/***/

void BytecodeCode::set_var(LocalVar* to, LocalVar* from) {
    switch(from->type()) {
    case VT_INT:
        to->setIntValue(from->getIntValue());
        return;
    case VT_DOUBLE:
        to->setDoubleValue(from->getDoubleValue());
        return;
    case VT_STRING:
        to->setStringValue(from->getStringValue());
        return;
    default:
        cerr << "Invalid variable type" << endl;
        assert(false);
    }
}

Status* BytecodeCode::call(int call_id, ofstream& out) {
    // cannot define them in the switch-block
    Value t;
    Value b;
    double dval;
    int64_t ival;
    int16_t  offset;
    uint16_t sid;
    uint16_t scope_id;
    uint16_t var_id;
    uint16_t fun_id;
    StackFrame* f;
    Status* status;
    int stack_size;
    int frame_ptr;
    int diff;
    pair<uint16_t, uint16_t> identifier;

    for (size_t bci = 0; bci < call_stack[call_id].bytecode()->length();) {
        size_t length;
        Instruction insn = call_stack[call_id].bytecode()->getInsn(bci);
//        out << bci << ": ";
        const char* name = bytecodeName(insn, &length);
        (void)name;
        switch (insn) {
        case BC_DLOAD:
//            out << name << " " << call_stack[call_id].bytecode()->getDouble(bci + 1);

            dval = call_stack[call_id].bytecode()->getDouble(bci + 1);
            value_stack.emplace(dval);

            break;
        case BC_ILOAD:
//            out << name << " " << call_stack[call_id].bytecode()->getInt64(bci + 1);

            ival = call_stack[call_id].bytecode()->getInt64(bci + 1);
            value_stack.emplace(ival);

            break;
        case BC_DLOAD0:
//            out << name;

            value_stack.emplace(0.0);

            break;
        case BC_ILOAD0:
//            out << name;

            value_stack.emplace(0);

            break;
        case BC_DLOAD1:
//            out << name;

            value_stack.emplace(1.0);

            break;
        case BC_ILOAD1:
//            out << name;

            value_stack.emplace(1);

            break;
        case BC_SLOAD:
//            out << name << " @" << call_stack[call_id].bytecode()->getUInt16(bci + 1);

            sid = call_stack[call_id].bytecode()->getUInt16(bci + 1);
            value_stack.emplace(constantById(sid));

            break;
        case BC_CALL:
//            out << name << " *" << call_stack[call_id].bytecode()->getUInt16(bci + 1);

            fun_id = call_stack[call_id].bytecode()->getUInt16(bci + 1);
            f = (StackFrame*)functionById(fun_id);
            call_stack.push_back(*f);
            stack_size = value_stack.size();
            status = call(call_stack.size() - 1, out);
            if(status->isError()) {
                return status;
            }
            diff = stack_size - value_stack.size();
            if(diff != f->parametersNumber() && diff != f->parametersNumber() - 1) {
                cerr << "Suspicious value stack size ("
                     << "was " << stack_size
                     << ", became " << value_stack.size()
                     << ") after function call '"
                     << functionById(fun_id)->name()
                     << "'" << endl;
                return Status::Error("Suspicious value stack size", bci);
            }
            call_stack.pop_back();

            break;
        case BC_CALLNATIVE:
//            out << name << " *" << call_stack[call_id].bytecode()->getUInt16(bci + 1);
            break;
        case BC_LOADDVAR:
        case BC_STOREDVAR:
        case BC_LOADIVAR:
        case BC_STOREIVAR:
        case BC_LOADSVAR:
        case BC_STORESVAR:
//            out << name << " @" << call_stack[call_id].bytecode()->getUInt16(bci + 1);
            break;
        case BC_LOADCTXDVAR:
//            out << name << " @" << call_stack[call_id].bytecode()->getUInt16(bci + 1)
//                << ":" << call_stack[call_id].bytecode()->getUInt16(bci + 3);

            scope_id = call_stack[call_id].bytecode()->getUInt16(bci + 1);
            var_id = call_stack[call_id].bytecode()->getUInt16(bci + 3);
            identifier = make_pair(scope_id, var_id);

            frame_ptr = lookup_frame(call_id, identifier);
            assert(frame_ptr >= 0);

            value_stack.emplace((*call_stack[frame_ptr].local_vars())[identifier].getDoubleValue());
            break;

        case BC_STORECTXDVAR:
//            out << name << " @" << call_stack[call_id].bytecode()->getUInt16(bci + 1)
//                << ":" << call_stack[call_id].bytecode()->getUInt16(bci + 3);

            scope_id = call_stack[call_id].bytecode()->getUInt16(bci + 1);
            var_id = call_stack[call_id].bytecode()->getUInt16(bci + 3);
            identifier = make_pair(scope_id, var_id);

            frame_ptr = lookup_frame(call_id, identifier);
            assert(frame_ptr >= 0);

            t = value_stack.top();
            value_stack.pop();
            (*call_stack[frame_ptr].local_vars())[identifier] = var_by_scope[scope_id][var_id];
            (*call_stack[frame_ptr].local_vars())[identifier].setDoubleValue(t._doubleValue);

            break;
        case BC_LOADCTXIVAR:
//            out << name << " @" << call_stack[call_id].bytecode()->getUInt16(bci + 1)
//                << ":" << call_stack[call_id].bytecode()->getUInt16(bci + 3);

            scope_id = call_stack[call_id].bytecode()->getUInt16(bci + 1);
            var_id = call_stack[call_id].bytecode()->getUInt16(bci + 3);
            identifier = make_pair(scope_id, var_id);

            frame_ptr = lookup_frame(call_id, identifier);
            assert(frame_ptr >= 0);

            value_stack.emplace((*call_stack[frame_ptr].local_vars())[identifier].getIntValue());
            break;

        case BC_STORECTXIVAR:
//            out << name << " @" << call_stack[call_id].bytecode()->getUInt16(bci + 1)
//                << ":" << call_stack[call_id].bytecode()->getUInt16(bci + 3);

            scope_id = call_stack[call_id].bytecode()->getUInt16(bci + 1);
            var_id = call_stack[call_id].bytecode()->getUInt16(bci + 3);
            identifier = make_pair(scope_id, var_id);

            frame_ptr = lookup_frame(call_id, identifier);
            assert(frame_ptr >= 0);

            t = value_stack.top();
            value_stack.pop();
            (*call_stack[frame_ptr].local_vars())[identifier] = var_by_scope[scope_id][var_id];
            (*call_stack[frame_ptr].local_vars())[identifier].setIntValue(t._intValue);
            break;

        case BC_LOADCTXSVAR:
//            out << name << " @" << call_stack[call_id].bytecode()->getUInt16(bci + 1)
//                << ":" << call_stack[call_id].bytecode()->getUInt16(bci + 3);

            scope_id = call_stack[call_id].bytecode()->getUInt16(bci + 1);
            var_id = call_stack[call_id].bytecode()->getUInt16(bci + 3);
            identifier = make_pair(scope_id, var_id);

            frame_ptr = lookup_frame(call_id, identifier);
            assert(frame_ptr >= 0);

            value_stack.emplace((*call_stack[frame_ptr].local_vars())[identifier].getStringValue());
            break;

        case BC_STORECTXSVAR:
//            out << name << " @" << call_stack[call_id].bytecode()->getUInt16(bci + 1)
//                << ":" << call_stack[call_id].bytecode()->getUInt16(bci + 3);

            scope_id = call_stack[call_id].bytecode()->getUInt16(bci + 1);
            var_id = call_stack[call_id].bytecode()->getUInt16(bci + 3);
            identifier = make_pair(scope_id, var_id);

            frame_ptr = lookup_frame(call_id, identifier);
            assert(frame_ptr >= 0);

            t = value_stack.top();
            value_stack.pop();

            (*call_stack[frame_ptr].local_vars())[identifier] = var_by_scope[scope_id][var_id];
            (*call_stack[frame_ptr].local_vars())[identifier].setStringValue(t._stringValue);
            break;

        case BC_IFICMPNE:
//            out << name << " " << call_stack[call_id].bytecode()->getInt16(bci + 1) + bci + length;// + 1;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            offset = call_stack[call_id].bytecode()->getInt16(bci + 1);
            if(t._intValue != b._intValue) {
                bci += offset;
            }

            break;
        case BC_IFICMPE:
//            out << name << " " << call_stack[call_id].bytecode()->getInt16(bci + 1) + bci + length;// + 1;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            offset = call_stack[call_id].bytecode()->getInt16(bci + 1);
            if(t._intValue == b._intValue) {
                bci += offset;
            }

            break;
        case BC_IFICMPG:
//            out << name << " " << call_stack[call_id].bytecode()->getInt16(bci + 1) + bci + length;// + 1;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            offset = call_stack[call_id].bytecode()->getInt16(bci + 1);
            if(t._intValue > b._intValue) {
                bci += offset;
            }

            break;
        case BC_IFICMPGE:
//            out << name << " " << call_stack[call_id].bytecode()->getInt16(bci + 1) + bci + length;// + 1;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            offset = call_stack[call_id].bytecode()->getInt16(bci + 1);
            if(t._intValue >= b._intValue) {
                bci += offset;
            }

            break;
        case BC_IFICMPL:
//            out << name << " " << call_stack[call_id].bytecode()->getInt16(bci + 1) + bci + length;// + 1;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            offset = call_stack[call_id].bytecode()->getInt16(bci + 1);
            if(t._intValue < b._intValue) {
                bci += offset;
            }

            break;
        case BC_IFICMPLE:
//            out << name << " " << call_stack[call_id].bytecode()->getInt16(bci + 1) + bci + length;// + 1;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            offset = call_stack[call_id].bytecode()->getInt16(bci + 1);
            if(t._intValue <= b._intValue) {
                bci += offset;
            }

            break;
        case BC_JA:
//            out << name << " " << call_stack[call_id].bytecode()->getInt16(bci + 1) + bci + length;// + 1;

            offset = call_stack[call_id].bytecode()->getInt16(bci + 1);
            bci += offset;

            break;
        case BC_RETURN:
//            out << name;
            return Status::Ok();

        case BC_DADD:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._doubleValue + b._doubleValue);
            break;

        case BC_IADD:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._intValue + b._intValue);
            break;

        case BC_DCMP:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            if(t._doubleValue < b._doubleValue) {
                value_stack.emplace(-1);
            } else if(t._doubleValue == b._doubleValue) {
                value_stack.emplace(0);
            } else {
                value_stack.emplace(1);
            }
            break;

        case BC_ICMP:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            if(t._intValue < b._intValue) {
                value_stack.emplace(-1);
            } else if(t._intValue == b._intValue) {
                value_stack.emplace(0);
            } else {
                value_stack.emplace(1);
            }
            break;

        case BC_DDIV:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._doubleValue / b._doubleValue);
            break;

        case BC_IDIV:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._intValue / b._intValue);
            break;

        case BC_DMUL:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._doubleValue * b._doubleValue);
            break;

        case BC_IMUL:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._intValue * b._intValue);
            break;

        case BC_DNEG:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            value_stack.emplace(-t._doubleValue);
            break;

        case BC_INEG:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            value_stack.emplace(-t._intValue);
            break;

        case BC_DPRINT:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            cout << t._doubleValue;
            break;

        case BC_IPRINT:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            cout << t._intValue;
            break;

        case BC_SPRINT:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            cout << t._stringValue;
            break;

        case BC_DSUB:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._doubleValue - b._doubleValue);
            break;

        case BC_ISUB:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._intValue - b._intValue);
            break;

        case BC_IMOD:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._intValue % b._intValue);
            break;

        case BC_IAAND:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._intValue & b._intValue);
            break;

        case BC_IAOR:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._intValue | b._intValue);
            break;

        case BC_IAXOR:
//            out << name;

            t = value_stack.top();
            value_stack.pop();
            b = value_stack.top();
            value_stack.pop();
            value_stack.emplace(t._intValue ^ b._intValue);
            break;

        case BC_DUMP:
//            out << "--- idle";
            break;

        case BC_I2D:
//            out << name;
            t = value_stack.top();
            value_stack.pop();
            value_stack.emplace((double)t._intValue);
            break;

        case BC_D2I:
//            out << name;
            t = value_stack.top();
            value_stack.pop();
            value_stack.emplace((int)t._doubleValue);
            break;

        case BC_S2I:
//            out << name;
            t = value_stack.top();
            value_stack.pop();
            value_stack.emplace(stoi(t._stringValue));
            break;

        case BC_POP:
//            out << name;
            value_stack.pop();
            break;

        default:
//            out << name;
            break;
        }
//        out << endl;
        bci += length;
    }
    return Status::Ok();
}

void BytecodeCode::print_funs(ofstream& out) {
    int it;
    BytecodeFunction* fun;
    for(it = 0, fun = (BytecodeFunction*)functionById(it); fun != 0; ++it, fun = (BytecodeFunction*)functionById(it)) {
        out << "function " << fun->name() << ": " << endl;
        fun->bytecode()->dump(out);
    }
}

Status* BytecodeCode::execute(vector<Var *> &vars) {
    ofstream out("debug.output");
    print_funs(out);

    uint16_t top_scope_id = 1; // sic!
    StackFrame sf(*(StackFrame*)(functionById(top_function_id)));
    map<pair<uint16_t, uint16_t>, LocalVar>* global_vars = sf.local_vars();
    for(Var* var: vars) {
        LocalVar lvar = *(LocalVar*)var;
        pair<int, int> var_id = get_var_id(scopes[top_scope_id], lvar.name());
        if(var_id.first != -1 && var_id.second != -1) {
            (*global_vars)[var_id] = lvar;
        }
    }
    call_stack.push_back(sf);
    Status* status = call(0, out);
    return status;
}

}
