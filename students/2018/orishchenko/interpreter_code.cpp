#include "interpreter_code.h"

namespace mathvm {

    InterpreterCode::InterpreterCode() {
        varStack = stack<Variable>();
        funStack = stack<BytecodeFunction *>();
        contextStack.push(map<uint16_t, InterpreterCtx *>());
        _pos = stack<uint32_t>();
    }

    InterpreterCode::~InterpreterCode() {}


    Status *InterpreterCode::execute(vector<Var *> &vars) {
        BytecodeFunction *top = (BytecodeFunction *) functionByName("<top>");
        funStack.push(top);
        _pos.push(0);
        try {
            while (cur_instr() != BC_STOP) {
                runInstr();
            }
        } catch (...) {
            return Status::Error("error");
        }
        funStack.pop();
        _pos.pop();
        return Status::Ok();
    }


    void InterpreterCode::runInstr() {
        Instruction insn = bytecode()->getInsn(pos());
//        cout << bytecodeName(insn) << endl;
        next_command();

        switch (insn) {
            case BC_ILOAD:
            case BC_DLOAD:
            case BC_SLOAD:
                push_next(insn);
                break;
            case BC_IPRINT:
            case BC_DPRINT:
            case BC_SPRINT:
                print();
                break;
            case BC_LOADCTXIVAR:
            case BC_LOADCTXDVAR:
            case BC_LOADCTXSVAR: {
                auto con = get_int();
                next_int();
                auto idx = get_int();
                next_int();
                varStack.push(context(con)->get(idx));
                break;
            }
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR:
            case BC_STORECTXSVAR: {
                auto ctx = get_int();
                next_int();
                auto idx = get_int();
                next_int();
                context(ctx)->set(idx, varStack.top());
                varStack.pop();
                break;
            }
            case BC_IADD: {
                auto x = top_int();
                auto y = top_int();
                push_int(x + y);
                break;
            }
            case BC_ISUB: {
                auto x = top_int();
                auto y = top_int();
                push_int(y - x);
                break;
            }
            case BC_IMUL: {
                auto x = top_int();
                auto y = top_int();
                push_int(x * y);
                break;
            }
            case BC_IDIV: {
                auto x = top_int();
                auto y = top_int();
                push_int(y / x);
                break;
            }
            case BC_IMOD: {
                auto x = top_int();
                auto y = top_int();
                push_int(x % y);
                break;
            }
            case BC_IAAND: {
                auto x = top_int();
                auto y = top_int();
                push_int(x & y);
                break;
            }
            case BC_IAOR: {
                auto x = top_int();
                auto y = top_int();
                push_int(x | y);
                break;
            }
            case BC_IAXOR: {
                auto x = top_int();
                auto y = top_int();
                push_int(x ^ y);
                break;
            }
            case BC_DADD: {
                auto x = top_double();
                auto y = top_double();
                push_double(x + y);
                break;
            }
            case BC_DSUB: {
                auto x = top_double();
                auto y = top_double();
                push_double(y - x);
                break;
            }
            case BC_DMUL: {
                auto x = top_double();
                auto y = top_double();
                push_double(x * y);
                break;
            }
            case BC_DDIV: {
                auto x = top_double();
                auto y = top_double();
                push_double(y / x);
                break;
            }
            case BC_INEG: {
                auto x = top_int();
                push_int(-x);
                break;
            }
            case BC_DNEG: {
                auto x = top_double();
                push_double(-x);
                break;
            }
            case BC_S2I: {
                auto x = varStack.top().get_string();
                varStack.pop();
                push_int(stol(x));
                break;
            }
            case BC_I2D: {
                auto x = top_int();
                push_double(x);
                break;
            }
            case BC_D2I: {
                auto x = top_double();
                push_int(x);
                break;
            }
            case BC_SWAP: {
                auto x = varStack.top();
                varStack.pop();
                auto y = varStack.top();
                varStack.pop();
                varStack.push(x);
                varStack.push(y);
                break;
            }
            case BC_DCMP: {
                auto x = top_double();
                auto y = top_double();
                if (std::abs(x - y) < 0.0001)
                    push_int(0);
                else if (x < y)
                    push_int(-1);
                else
                    push_int(1);
                break;
            }
            case BC_ILOAD0:
                push_int(0);
                break;
            case BC_ILOAD1:
                push_int(1);
                break;
            case BC_ILOADM1:
                push_int(-1);
                break;
            case BC_JA:
                // jump_to(get_int()); // тест "extra/stack2" не проходит так
                jump_to(static_cast<uint32_t>(bytecode()->getInt16(pos())));
                break;
            case BC_IFICMPE: {
                auto x = top_int();
                auto y = top_int();
                if (x == y)
                    jump_to(get_int());
                else
                    next_int();
                break;
            }
            case BC_IFICMPNE: {
                auto x = top_int();
                auto y = top_int();
                if (x != y) {
                    jump_to(get_int());
                } else {
                    next_int();
                }
                break;
            }
            case BC_IFICMPG: {
                auto x = top_int();
                auto y = top_int();
                if (x > y) {
                    jump_to(get_int());
                } else {
                    next_int();
                }
                break;
            }
            case BC_IFICMPGE: {
                auto x = top_int();
                auto y = top_int();
                if (x >= y) {
                    jump_to(get_int());
                } else {
                    next_int();
                }
                break;
            }
            case BC_IFICMPL: {
                auto x = top_int();
                auto y = top_int();
                if (x < y) {
                    jump_to(get_int());
                } else {
                    next_int();
                }
                break;
            }
            case BC_IFICMPLE: {
                auto x = top_int();
                auto y = top_int();
                if (x <= y) {
                    jump_to(get_int());
                } else {
                    next_int();
                }
                break;
            }
            case BC_LOADIVAR0: {
                auto x = varStack.top();
                varStack.push(x);
                break;
            }
            case BC_CALL: {
                auto f = (BytecodeFunction *) functionById(bytecode()->getUInt16(pos()));
                funStack.push(f);
                _pos.push(0);
                map<uint16_t, InterpreterCtx *> tmp;
                for (auto ctx : contextStack.top()) {
                    if (f != ctx.second->func)
                        tmp.insert(make_pair(ctx.first, ctx.second));
                    else {
                        auto copyContext = new InterpreterCtx(ctx.second->func);
                        copyContext->vars = map<uint16_t, Variable>(ctx.second->vars);
                        tmp.insert(make_pair(ctx.first, copyContext));
                    }
                }
                contextStack.push(tmp);
                break;
            }
            case BC_RETURN: {
                _pos.pop();
                for (auto ctx : contextStack.top())
                    if (ctx.second->func == funStack.top())
                        delete ctx.second;
                funStack.pop();
                contextStack.pop();
                next_int();
                break;
            }
            default:
                break;
        }
    }

    uint16_t InterpreterCode::get_int() { return bytecode()->getUInt16(pos()); }

    InterpreterCtx *InterpreterCode::context(uint16_t ctx) {
        return contextStack.top().at(ctx);
    }

    void InterpreterCode::print() {
        Variable var = varStack.top();
        switch (var.get_type()) {
            case VT_INT:
                cout << var.get_int();
                break;
            case VT_DOUBLE:
                cout << var.get_double();
                break;
            case VT_STRING:
                cout << var.get_string();
                break;
            default:
                break;
        }
        varStack.pop();
    }


    void InterpreterCode::push_next(Instruction inst) {
        switch (inst) {
            case BC_ILOAD:
                push_int(bytecode()->getInt64(pos()));
                jump_to(8);
                break;
            case BC_DLOAD:
                push_double(bytecode()->getDouble(pos()));
                jump_to(8);
                break;
            case BC_SLOAD:
                push_str(constantById(bytecode()->getUInt16(pos())));
                next_int();
                break;
            default:
                break;
        }
    }

    void InterpreterCode::add_ctx(VisitorCtx *context, BytecodeFunction *f) {
        contextStack.top().insert(make_pair(context->address, new InterpreterCtx(f)));
    }

    void InterpreterCode::push_int(int64_t value) {
        Variable var = Variable(VT_INT);
        var.set_int(value);
        varStack.push(var);
    }


    void InterpreterCode::push_double(double value) {
        Variable var = Variable(VT_DOUBLE);
        var.set_double(value);
        varStack.push(var);
    }

    void InterpreterCode::push_str(const string &value) {
        Variable var = Variable(VT_STRING);
        var.set_string(value.c_str());
        varStack.push(var);
    }

    void InterpreterCode::next_command() {
        auto newVal = _pos.top() += 1;
        _pos.pop();
        _pos.push(newVal);
    }

    void InterpreterCode::next_int() {
        auto newVal = _pos.top() += 2;
        _pos.pop();
        _pos.push(newVal);
    }

    void InterpreterCode::jump_to(uint32_t place) {
        auto newVal = _pos.top() += place;
        _pos.pop();
        _pos.push(newVal);
    }

    double InterpreterCode::top_double() {
        auto x = varStack.top().get_double();
        varStack.pop();
        return x;
    }

    int64_t InterpreterCode::top_int() {
        auto x = varStack.top().get_int();
        varStack.pop();
        return x;
    }

    Instruction InterpreterCode::cur_instr() {
        return bytecode()->getInsn(pos());
    }

    Bytecode *InterpreterCode::bytecode() {
        return funStack.top()->bytecode();
    }

    uint32_t InterpreterCode::pos() {
        return _pos.top();
    }

}