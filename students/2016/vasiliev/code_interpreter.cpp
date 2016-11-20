#include "code_interpreter.h"
#include "mathvm.h"

code_interpreter::code_interpreter() {}

Status* code_interpreter::execute(vector<Var*>& vars) {
    Code::FunctionIterator it(this);
    while (it.hasNext()) {
        BytecodeFunction* f = (BytecodeFunction*) it.next();
        cout << f->name() << " " << f->id() << endl;
        f->disassemble(cout);
        cout << endl;
    }

    stack.reserve(1024);
    stack.clear();
    calls.reserve(1024);
    calls.clear();
    locals.resize(1024);

    bytecode = ((BytecodeFunction*) functionById(0))->bytecode();
    locals_offset = 0;
    current_id = 0;

    cout << "result:" << endl;
    for (index = 0; index < bytecode->length();) {
        execute_insn();
    }

    return Status::Ok();
}

void code_interpreter::execute_insn() {
    Instruction insn = bytecode->getInsn(index++);
    universal a;
    universal b;
    switch (insn) {

        case BC_CALL:
            index += 2;
            call(bytecode->getInt16(index - 2));
            break;
        case BC_RETURN:
            ret();
            break;

        case BC_ILOAD:
            push_typed<int64_t>(bytecode->getInt64(index));
            index += 8;
            break;
        case BC_ILOAD0:
            push_typed<int64_t>(0);
            break;
        case BC_ILOAD1:
            push_typed<int64_t>(1);
            break;
        case BC_DLOAD:
            push_typed<double>(bytecode->getDouble(index));
            index += 8;
            break;
        case BC_SLOAD:
            push_typed<uint16_t >(bytecode->getUInt16(index));
            index += 2;
            break;
        case BC_STOREIVAR:
            put_local_i(bytecode->getUInt16(index), pop_typed<int64_t>());
            index += 2;
            break;
        case BC_STOREDVAR:
            put_local_d(bytecode->getUInt16(index), pop_typed<double>());
            index += 2;
            break;

        case BC_LOADIVAR:
            push_typed<int64_t>(get_local_i(bytecode->getUInt16(index)));
            index += 2;
            break;
        case BC_LOADDVAR:
            push_typed<double>(get_local_d(bytecode->getUInt16(index)));
            index += 2;
            break;

        case BC_INEG:
            push_typed<int64_t>(-pop_typed<int64_t>());
            break;
        case BC_DNEG:
            push_typed<double>(-pop_typed<double>());
            break;
        case BC_IADD:
            push_typed<int64_t>(pop_typed<int64_t>() + pop_typed<int64_t>());
            break;
        case BC_DADD:
            push_typed<double>(pop_typed<double>() + pop_typed<double>());
            break;
        case BC_ISUB:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            push_typed<int64_t>(b.i - a.i);
            break;
        case BC_DSUB:
            a.d = pop_typed<double>();
            b.d = pop_typed<double>();
            push_typed<double>(b.d - a.d);
            break;
        case BC_IMUL:
            push_typed<int64_t>(pop_typed<int64_t>() * pop_typed<int64_t>());
            break;
        case BC_DMUL:
            push_typed<double>(pop_typed<double>() * pop_typed<double>());
            break;
        case BC_IDIV:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            push_typed<int64_t>(b.i / a.i);
            break;
        case BC_IMOD:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            push_typed<int64_t>(b.i % a.i);
            break;
        case BC_DDIV:
            a.d = pop_typed<double>();
            b.d = pop_typed<double>();
            push_typed<double>(b.d / a.d);
            break;
        case BC_ICMP:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            push_typed<int64_t>((b.i > a.i) - (b.i < a.i));
            break;
        case BC_DCMP:
            a.d = pop_typed<double>();
            b.d = pop_typed<double>();
            push_typed<int64_t>((b.i > a.i) - (b.i < a.i));
            break;


        case BC_JA:
            index += bytecode->getInt16(index);
            break;
        case BC_IFICMPE:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            if (b.i == a.i) index += bytecode->getInt16(index);
            else index += 2;
            break;
        case BC_IFICMPNE:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            if (b.i != a.i) index += bytecode->getInt16(index);
            else index += 2;
            break;
        case BC_IFICMPG:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            if (b.i > a.i) index += bytecode->getInt16(index);
            else index += 2;
            break;
        case BC_IFICMPGE:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            if (b.i >= a.i) index += bytecode->getInt16(index);
            else index += 2;
            break;
        case BC_IFICMPL:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            if (b.i < a.i) index += bytecode->getInt16(index);
            else index += 2;
            break;
        case BC_IFICMPLE:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            if (b.i <= a.i) index += bytecode->getInt16(index);
            else index += 2;
            break;

        case BC_IAAND:
            push_typed<int64_t>(pop_typed<int64_t>() & pop_typed<int64_t>());
            break;
        case BC_IAOR:
            push_typed<int64_t>(pop_typed<int64_t>() | pop_typed<int64_t>());
            break;
        case BC_IAXOR:
            push_typed<int64_t>(pop_typed<int64_t>() ^ pop_typed<int64_t>());
            break;

        case BC_IPRINT:
            cout << pop_typed<int64_t>();
            break;
        case BC_DPRINT:
            cout << pop_typed<double>();
            break;
        case BC_SPRINT:
            cout << constantById(pop_typed<uint16_t>());
            break;

        case BC_SWAP:
            a.i = pop_typed<int64_t>();
            b.i = pop_typed<int64_t>();
            push_typed(a.i);
            push_typed(b.i);
            break;
        default:
            throw "No instruction";
    }


}