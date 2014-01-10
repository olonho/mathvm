/* 
 * File:   MvmInterpreter.h
 * Author: stasstels
 *
 * Created on January 10, 2014, 3:19 AM
 */

#ifndef MVMINTERPRETER_H
#define	MVMINTERPRETER_H
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <functional>

#include "mathvm.h"
#include "ast.h"

namespace mathvm {
    using namespace std;

    class MvmBytecode : public Code {
    public:

        MvmBytecode() : bytecode(new Bytecode()) {
        }

        ~MvmBytecode() {
            delete bytecode;
        }

        virtual Status* execute(vector<Var*>& vars) {
            try {
                execute();
            } catch (const exception& e) {
                return new Status(e.what());
            }
            return 0;
        }

        virtual void disassemble(ostream& out = cout, FunctionFilter* filter = 0) {
            Code::disassemble(out, filter);
            bytecode -> dump(out);
        }

        Bytecode* getBytecode() {
            return bytecode;
        }
    private:
        typedef vector<Var> VarStack;
        typedef vector<Bytecode*> BytecodeStack;
        typedef vector<u_int32_t> InsStack;
        typedef vector< vector<Var> > Env;

        Bytecode* bytecode;
        VarStack vStk;
        BytecodeStack bcStk;
        InsStack iStk;
        Env env;

        void loadDouble(double val) {
            Var d(VT_DOUBLE, "");
            d.setDoubleValue(val);
            vStk.push_back(d);
        };

        void loadInt(int64_t val) {
            Var i(VT_INT, "");
            i.setIntValue(val);
            vStk.push_back(i);
        };

        void loadString(const string& val) {
            Var s(VT_STRING, "");
            s.setStringValue(val.c_str());
            vStk.push_back(s);
        };

        void loadVar(u_int16_t id) {
            vStk.push_back(env[id].back());
        };

        void loadVar(u_int16_t scopeId, u_int16_t id) {
            u_int64_t scope = env[scopeId].back().getIntValue();
            vector<Var>& varValues = env[id];
            if (varValues.empty()) {
                varValues.push_back(vStk.back());
            }
            while (varValues.size() <= scope) {
                varValues.push_back(varValues.back());
            }
            vStk.push_back(env[id][scope]);
        };

        void storeVar(u_int16_t id) {
            if (env.size() <= id) {
                env.resize(id + 1);
            }
            vector<Var>& varValues = env[id];
            if (varValues.empty()) {
                varValues.push_back(vStk.back());
            } else {
                varValues.back() = vStk.back();
            }
            vStk.pop_back();
        }

        void storeVar(u_int16_t scopeId, u_int16_t id) {
            u_int64_t scope = env[scopeId].back().getIntValue();
            if (env.size() <= id) {
                env.resize(id + 1);
            }
            vector<Var>& varValues = env[id];
            if (varValues.empty()) {
                varValues.push_back(Var(VT_INT, ""));
            }
            if (varValues.size() <= scopeId) {
                varValues.resize(scopeId + 1, varValues.back());
            }
            varValues[scope] = vStk.back();
            vStk.pop_back();
        }

        template <class Pred>
        bool branch(Pred pred, int32_t pos) {
            Var v1(pop(vStk));
            Var v2(pop(vStk));
            checkType(v1.type(), VT_INT);
            checkType(v2.type(), VT_INT);
            return pred(v2.getIntValue(), v1.getIntValue());
        }

        template <class binOp>
        void dBinary(binOp f, u_int32_t pos) {
            Var v1(pop(vStk));
            Var v2(pop(vStk));
            checkType(v1.type(), VT_DOUBLE);
            checkType(v2.type(), VT_DOUBLE);
            Var res(VT_DOUBLE, "");
            res.setDoubleValue(f(v2.getDoubleValue(), v1.getDoubleValue()));
            vStk.push_back(res);
        }

        template <class binOp>
        void iBinary(binOp f, u_int32_t pos) {
            Var v1(pop(vStk));
            Var v2(pop(vStk));
            checkType(v1.type(), VT_INT);
            checkType(v2.type(), VT_INT);
            Var res(VT_INT, "");
            res.setIntValue(f(v2.getIntValue(), v1.getIntValue()));
            vStk.push_back(res);
        }

        template <class unOp>
        void iUnary(unOp f, u_int32_t pos) {
            Var v(pop(vStk));
            checkType(v.type(), VT_INT);
            Var res(VT_INT, "");
            res.setIntValue(f(v.getIntValue()));
            vStk.push_back(res);
        }
        
        template <class unOp>
        void dUnary(unOp f, u_int32_t pos) {
            Var v(pop(vStk));
            checkType(v.type(), VT_DOUBLE);
            Var res(VT_DOUBLE, "");
            res.setDoubleValue(f(v.getDoubleValue()));
            vStk.push_back(res);
        }

        template <class T>
        static T pop(vector<T>& stk) {
            Var v(stk.back());
            stk.pop_back();
            return v;
        }

        void checkType(VarType a, VarType b) {
//            if (a != b) {
//                throw runtime_error("Type check failed!");
//            }
        }

        size_t getLength(Instruction i) {
            size_t length;
            bytecodeName(i, &length);
            return length;
        }

        void execute() {
            bcStk.push_back(bytecode);
            iStk.push_back(0);
            while (!bcStk.empty()) {
                u_int32_t& nextIns = iStk.back();
                Bytecode& bc = *bcStk.back();
                Instruction i = bc.getInsn(nextIns);
                switch (i) {
                    case BC_INVALID:
                        throw runtime_error("INVALID instruction");
                    case BC_DLOAD:
                        loadDouble(bc.getDouble(nextIns + 1));
                        break;
                    case BC_ILOAD:
                        loadInt(bc.getInt64(nextIns + 1));
                        break;
                    case BC_SLOAD:
                        loadString(constantById(bc.getInt16(nextIns + 1)));
                        break;
                    case BC_DLOAD0:
                        loadDouble(0.0);
                        break;
                    case BC_ILOAD0:
                        loadInt(0);
                        break;
                    case BC_SLOAD0:
                        loadString("");
                        break;
                    case BC_DLOAD1:
                        loadDouble(1.0);
                        break;
                    case BC_ILOAD1:
                        loadInt(1);
                        break;
                    case BC_DLOADM1:
                        loadDouble(-1.0);
                        break;
                    case BC_ILOADM1:
                        loadInt(-1);
                        break;
                    case BC_DADD:
                        dBinary(plus<double>(), nextIns);
                        break;
                    case BC_IADD:
                        iBinary(plus<int64_t>(), nextIns);
                        break;
                    case BC_DSUB:
                        dBinary(minus<double>(), nextIns);
                        break;
                    case BC_ISUB:
                        iBinary(minus<int64_t>(), nextIns);
                        break;
                    case BC_DMUL:
                        dBinary(multiplies<double>(), nextIns);
                        break;
                    case BC_IMUL:
                        iBinary(multiplies<int64_t>(), nextIns);
                        break;
                    case BC_DDIV:
                        dBinary(divides<double>(), nextIns);
                        break;
                    case BC_IDIV:
                        iBinary(divides<int64_t>(), nextIns);
                        break;
                    case BC_IMOD:
                        iBinary(modulus<int64_t>(), nextIns);
                        break;
                    case BC_DNEG:
                        dUnary(negate<int64_t>(), nextIns);
                        break;
                    case BC_INEG:
                        iUnary(negate<int64_t>(), nextIns);
                        break;
                    case BC_IAOR:
                        iBinary(logical_or<int64_t>(), nextIns);
                        break;
                    case BC_IAAND:
                        iBinary(logical_and<int64_t>(), nextIns);
                        break;
                    case BC_IAXOR:
                        iBinary(logical_or<int64_t>(), nextIns);
                        break;
                    case BC_IPRINT:
                    {
                        Var v(pop(vStk));
                        checkType(VT_INT, v.type());
                        cout << v.getIntValue();
                        break;
                    }
                    case BC_DPRINT:
                    {
                        Var v(pop(vStk));
                        ;
                        checkType(VT_DOUBLE, v.type());
                        cout << v.getDoubleValue();
                        break;
                    }
                    case BC_SPRINT:
                    {
                        Var v(pop(vStk));
                        checkType(VT_DOUBLE, v.type());
                        cout << v.getStringValue();
                        break;
                    }
                    case BC_I2D:
                    {
                        Var v(pop(vStk));
                        checkType(VT_INT, v.type());
                        Var d(VT_DOUBLE, "");
                        d.setDoubleValue(v.getIntValue());
                        vStk.push_back(d);
                        break;
                    }
                    case BC_D2I:
                    {
                        Var v(pop(vStk));
                        checkType(VT_DOUBLE, v.type());
                        Var i(VT_INT, "");
                        i.setIntValue(static_cast<int64_t> (v.getDoubleValue()));
                        vStk.push_back(i);
                        break;
                    }
                    case BC_S2I:
                    {
                        //Strange
                        Var v(pop(vStk));
                        checkType(VT_STRING, v.type());
                        Var i(VT_INT, "");
                        i.setIntValue(*v.getStringValue());
                        vStk.push_back(i);
                        break;
                    }
                    case BC_SWAP:
                    {
                        Var v1(pop(vStk));
                        Var v2(pop(vStk));
                        vStk.push_back(v1);
                        vStk.push_back(v2);
                        break;
                    }
                    case BC_POP:
                        vStk.pop_back();
                        break;
                    case BC_LOADDVAR0:
                        loadVar(0);
                        break;
                    case BC_LOADDVAR1:
                        loadVar(1);
                        break;
                    case BC_LOADDVAR2:
                        loadVar(2);
                        break;
                    case BC_LOADDVAR3:
                        loadVar(3);
                        break;
                    case BC_LOADDVAR:
                        loadVar(bc.getInt16(nextIns + 1));
                        break;
                    case BC_LOADCTXDVAR:
                        loadVar(bc.getInt16(nextIns + 1), bc.getInt16(nextIns + 3));
                        break;
                    case BC_LOADIVAR0:
                        loadVar(0);
                        break;
                    case BC_LOADIVAR1:
                        loadVar(1);
                        break;
                    case BC_LOADIVAR2:
                        loadVar(2);
                        break;
                    case BC_LOADIVAR3:
                        loadVar(3);
                        break;
                    case BC_LOADIVAR:
                        loadVar(bc.getInt16(nextIns + 1));
                        break;
                    case BC_LOADCTXIVAR:
                        loadVar(bc.getInt16(nextIns + 1), bc.getInt16(nextIns + 3));
                        break;
                    case BC_LOADSVAR0:
                        loadVar(0);
                        break;
                    case BC_LOADSVAR1:
                        loadVar(1);
                        break;
                    case BC_LOADSVAR2:
                        loadVar(2);
                        break;
                    case BC_LOADSVAR3:
                        loadVar(2);
                        break;
                    case BC_LOADSVAR:
                        loadVar(bc.getInt16(nextIns + 1));
                        break;
                    case BC_LOADCTXSVAR:
                        loadVar(bc.getInt16(nextIns + 1), bc.getInt16(nextIns + 3));
                        break;
                    case BC_STOREDVAR0:
                        storeVar(0);
                        break;
                    case BC_STOREDVAR1:
                        storeVar(1);
                        break;
                    case BC_STOREDVAR2:
                        storeVar(2);
                        break;
                    case BC_STOREDVAR3:
                        storeVar(3);
                        break;
                    case BC_STOREDVAR:
                        storeVar(bc.getInt16(nextIns + 1));
                        break;
                    case BC_STORECTXDVAR:
                        storeVar(bc.getInt16(nextIns + 1), bc.getInt16(nextIns + 3));
                        break;
                    case BC_STOREIVAR0:
                        storeVar(0);
                        break;
                    case BC_STOREIVAR1:
                        storeVar(1);
                        break;
                    case BC_STOREIVAR2:
                        storeVar(2);
                        break;
                    case BC_STOREIVAR3:
                        storeVar(3);
                        break;
                    case BC_STOREIVAR:
                        storeVar(bc.getInt16(nextIns + 1));
                        break;
                    case BC_STORECTXIVAR:
                        storeVar(bc.getInt16(nextIns + 1), bc.getInt16(nextIns + 3));
                        break;
                    case BC_STORESVAR0:
                        storeVar(0);
                        break;
                    case BC_STORESVAR1:
                        storeVar(1);
                        break;
                    case BC_STORESVAR2:
                        storeVar(2);
                        break;
                    case BC_STORESVAR3:
                        storeVar(3);
                        break;
                    case BC_STORESVAR:
                        storeVar(bc.getInt16(nextIns + 1));
                        break;
                    case BC_STORECTXSVAR:
                        storeVar(bc.getInt16(nextIns + 1), bc.getInt16(nextIns + 3));
                        break;
                    case BC_DCMP:
                    {
                        Var v1(pop(vStk));
                        Var v2(pop(vStk));
                        checkType(v1.type(), VT_DOUBLE);
                        checkType(v2.type(), VT_DOUBLE);
                        Var cmp(VT_INT, "");
                        double d1 = v1.getDoubleValue();
                        double d2 = v2.getDoubleValue();
                        cmp.setIntValue(d1 < d2 ? -1 : (d1 == d2) ? 0 : 1);
                        vStk.push_back(cmp);
                        break;
                    }
                    case BC_ICMP:
                    {
                        Var v1(pop(vStk));
                        Var v2(pop(vStk));
                        checkType(v1.type(), VT_INT);
                        checkType(v2.type(), VT_INT);
                        Var cmp(VT_INT, "");
                        int64_t i1 = v1.getIntValue();
                        int64_t i2 = v2.getIntValue();
                        cmp.setIntValue(i1 < i2 ? -1 : (i1 == i2) ? 0 : 1);
                        vStk.push_back(cmp);
                        break;
                    }
                    case BC_JA:
                    {
                        nextIns += bc.getInt16(nextIns + 1) + 1;
                        continue;
                    }
                    case BC_IFICMPNE:
                    {
                        if (!branch(not_equal_to<int64_t>(), nextIns)) {
                            break;
                        }
                        nextIns += bc.getInt16(nextIns + 1) + 1;
                        continue;
                    }
                    case BC_IFICMPE:
                    {
                        if (!branch(equal_to<int64_t>(), nextIns)) {
                            break;
                        }
                        nextIns += bc.getInt16(nextIns + 1) + 1;
                        continue;
                    }
                    case BC_IFICMPG:
                    {
                        if (!branch(greater<int64_t>(), nextIns)) {
                            break;
                        }
                        nextIns += bc.getInt16(nextIns + 1) + 1;
                        continue;
                    }
                    case BC_IFICMPGE:
                    {
                        if (!branch(greater_equal<int64_t>(), nextIns)) {
                            break;
                        }
                        nextIns += bc.getInt16(nextIns + 1) + 1;
                        continue;
                    }
                    case BC_IFICMPL:
                    {
                        if (!branch(less<int64_t>(), nextIns)) {
                            break;
                        }
                        nextIns += bc.getInt16(nextIns + 1) + 1;
                        continue;
                    }
                    case BC_IFICMPLE:
                    {
                        if (!branch(less_equal<int64_t>(), nextIns)) {
                            break;
                        }
                        nextIns += bc.getInt16(nextIns + 1) + 1;
                        continue;
                    }
                    case BC_DUMP:
                    {
                        cerr << "DUMP Not Implemented" << endl;
                    }
                    case BC_STOP:
                        bcStk.clear();
                        iStk.clear();
                        continue;
                    case BC_CALL:
                    {
                        TranslatedFunction* f = functionById(bc.getInt16(nextIns + 1));
                        bcStk.push_back(static_cast<BytecodeFunction*> (f) -> bytecode());
                        iStk.push_back(0);
                        continue;
                    }
                    case BC_CALLNATIVE:
                        break;
                    case BC_RETURN:
                    {
                        iStk.pop_back();
                        bcStk.pop_back();
                        if (!iStk.empty()) {
                            iStk.back() += getLength(BC_CALL);
                        }
                        continue;
                    }
                    case BC_BREAK:
                    {
                        break;
                    }
                    default: throw runtime_error("Unknown bytecode instruction");
                }
                nextIns += getLength(i);
            }
        }

    };




}

#endif	/* MVMINTERPRETER_H */

