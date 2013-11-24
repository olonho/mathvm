#include <fstream>
#include <sstream>
#include <stdexcept>

#include "bytecode_translator.h"

using namespace mathvm;

class interpreter_exception : public std::logic_error {
   uint32_t _position;
public:
   interpreter_exception(std::string const& reason, uint32_t position):
      std::logic_error(reason), _position(position) { }

   uint32_t position() const { return _position; }

   static interpreter_exception type_error(VarType target_type, VarType t, uint32_t position) {
      return interpreter_exception(std::string("Type error: wanted ") +
            typeToName(target_type) + " got " + typeToName(t), position);
   }

   static interpreter_exception type_error(VarType target_type, VarType t1, VarType t2,
         uint32_t position) {
      return interpreter_exception(std::string("Type error: wanted ") +
            typeToName(target_type) + " got " + typeToName(t1) + ", " +
         typeToName(t2), position);
   }

   static void assert_type(VarType target_type, VarType t, uint32_t position) {
      if(t != target_type)
         throw interpreter_exception::type_error(target_type, t, position);
   }

   static void assert_type(VarType target_type, VarType t1, VarType t2, uint32_t position) {
      if(t1 != target_type && t2 != target_type)
         throw interpreter_exception::type_error(target_type, t1, t2, position);
   }
};

size_t bc_length(Instruction insn) {
   size_t res;
   bytecodeName(insn, &res);
   return res;
}

typedef vector<Var> stack_t;

typedef vector<vector<Var> > varmap_t;

ostream& operator<<(ostream& ost, Var const& var) {
   switch (var.type()) {
      case VT_DOUBLE: return ost << var.getDoubleValue();
      case VT_INT: return ost << var.getIntValue();
      case VT_STRING: return ost << var.getStringValue();
      default: return ost << "UNKNOWN_TYPE";
   }
}

ostream& operator<<(ostream& ost, vector<Var> const& varvec) {
   if(varvec.empty()) return ost;
   ost << varvec[0];
   for(size_t i = 1; i < varvec.size(); ++i)
      ost << ", " << varvec[i];
   return ost;
}

ostream& operator<<(ostream& ost, varmap_t const& varmap) {
   if(varmap.empty()) return ost;
   for(size_t i = 0; i < varmap.size(); ++i)
      ost << std::endl << i << ": " << varmap[i];
   return ost;
}

template<class T> T fun_add(T v1, T v2) { return v1 + v2; }
template<class T> T fun_sub(T v1, T v2) { return v1 - v2; }
template<class T> T fun_mul(T v1, T v2) { return v1 * v2; }
template<class T> T fun_div(T v1, T v2) { return v1 / v2; }
int64_t fun_mod(int64_t v1, int64_t v2) { return v1 % v2; }
template<class T> T fun_neg(T v1) { return -v1; }
int64_t fun_or(int64_t v1, int64_t v2) { return v1 | v2; }
int64_t fun_and(int64_t v1, int64_t v2) { return v1 & v2; }
int64_t fun_xor(int64_t v1, int64_t v2) { return v1 ^ v2; }
bool fun_ne(int64_t v1, int64_t v2) { return v1 != v2; }
bool fun_eq(int64_t v1, int64_t v2) { return v1 == v2; }
bool fun_gt(int64_t v1, int64_t v2) { return v1 > v2; }
bool fun_ge(int64_t v1, int64_t v2) { return v1 >= v2; }
bool fun_lt(int64_t v1, int64_t v2) { return v1 < v2; }
bool fun_le(int64_t v1, int64_t v2) { return v1 <= v2; }

void run_double_load(stack_t& stack, double value) {
   Var var(VT_DOUBLE, "");
   var.setDoubleValue(value);
   stack.push_back(var);
}

void run_int_load(stack_t& stack, int64_t value) {
   Var var(VT_INT, "");
   var.setIntValue(value);
   stack.push_back(var);
}

void run_string_load(stack_t& stack, string const& value) {
   Var var(VT_STRING, "");
   var.setStringValue(value.c_str());
   stack.push_back(var);
}

void run_double_binop(stack_t& stack, double (*func)(double, double), uint32_t position) {
   Var v1 = stack.back();
   stack.pop_back();
   Var v2 = stack.back();
   stack.pop_back();
   interpreter_exception::assert_type(VT_DOUBLE, v1.type(), v2.type(), position);
   Var var(VT_DOUBLE, "");
   var.setDoubleValue(func(v2.getDoubleValue(), v1.getDoubleValue()));
   stack.push_back(var);
}

void run_int_binop(stack_t& stack, int64_t (*func)(int64_t, int64_t), uint32_t position) {
   Var v1 = stack.back();
   stack.pop_back();
   Var v2 = stack.back();
   stack.pop_back();
   interpreter_exception::assert_type(VT_INT, v1.type(), v2.type(), position);
   Var var(VT_INT, "");
   var.setIntValue(func(v2.getIntValue(), v1.getIntValue()));
   stack.push_back(var);
}

void run_double_unop(stack_t& stack, double (*func)(double), uint32_t position) {
   Var v = stack.back();
   stack.pop_back();
   interpreter_exception::assert_type(VT_DOUBLE, v.type(), position);
   Var var(VT_DOUBLE, "");
   var.setDoubleValue(func(v.getDoubleValue()));
   stack.push_back(var);
}

void run_int_unop(stack_t& stack, int64_t (*func)(int64_t), uint32_t position) {
   Var v = stack.back();
   stack.pop_back();
   interpreter_exception::assert_type(VT_INT, v.type(), position);
   Var var(VT_INT, "");
   var.setIntValue(func(v.getIntValue()));
   stack.push_back(var);
}

void run_loadvar(stack_t& stack, varmap_t& varmap, uint16_t id) {
   stack.push_back(varmap[id].back());
}

void run_loadvar(stack_t& stack, varmap_t& varmap, uint16_t ctx_id, uint16_t id) {
   // For "premature optimization" reasons I lazily copy stack frames.
   uint64_t ctx = varmap[ctx_id].back().getIntValue();
   vector<Var>& varvec = varmap[id];
   // Might have been in translator but this way is more "efficient"
   if(varvec.empty()) varvec.push_back(stack.back());
   while(varvec.size() <= ctx) varvec.push_back(varvec.back());
   stack.push_back(varmap[id][ctx]);
}

void run_storevar(stack_t& stack, varmap_t& varmap, uint16_t id) {
   while(varmap.size() <= id) varmap.push_back(vector<Var>());
   vector<Var>& varvec = varmap[id];
   if(varvec.empty()) varvec.push_back(stack.back());
   else varvec.back() = stack.back();
   stack.pop_back();
}

void run_storevar(stack_t& stack, varmap_t& varmap, uint16_t ctx_id, uint16_t id) {
   uint64_t ctx = varmap[ctx_id].back().getIntValue();
   while(varmap.size() <= id) varmap.push_back(vector<Var>());
   vector<Var>& varvec = varmap[id];
   if(varvec.empty()) varvec.push_back(Var(VT_INT, ""));
   while(varvec.size() <= ctx) varvec.push_back(varvec.back());
   varvec[ctx] = stack.back();
   stack.pop_back();
}

bool run_if(stack_t& stack, bool (*func)(int64_t, int64_t), int32_t position) {
   Var v1 = stack.back();
   stack.pop_back();
   Var v2 = stack.back();
   stack.pop_back();
   interpreter_exception::assert_type(VT_INT, v1.type(), v2.type(), position);
   return func(v2.getIntValue(), v1.getIntValue());
}

void run_native(Code const& code, stack_t& stack, uint16_t id) {
   // TODO: do
}

void run(Code const& code, stack_t& stack, varmap_t& varmap, Bytecode const* bytecode) {
   std::vector<Bytecode const*> bc_stack;
   std::vector<uint32_t> ip_stack;
   bc_stack.push_back(bytecode);
   ip_stack.push_back(0);
   for(;!bc_stack.empty();) {
      uint32_t& ip = ip_stack.back();
      Bytecode const& bc = *bc_stack.back();
      Instruction insn = bc.getInsn(ip);
      loggerstr << "On " << ip << " executing " << insn << std::endl;
      switch(insn) {
         case BC_INVALID: throw interpreter_exception("INVALID instruction found", ip);
         case BC_DLOAD: run_double_load(stack, bc.getDouble(ip + 1)); break;
         case BC_ILOAD: run_int_load(stack, bc.getInt64(ip + 1)); break;
         case BC_SLOAD: run_string_load(stack, code.constantById(bc.getInt16(ip + 1)));
                        break;
         case BC_DLOAD0: run_double_load(stack, 0); break;
         case BC_ILOAD0: run_int_load(stack, 0); break;
         case BC_SLOAD0: run_string_load(stack, ""); break;
         case BC_DLOAD1: run_double_load(stack, 1); break;
         case BC_ILOAD1: run_int_load(stack, 1); break;
         case BC_DLOADM1: run_double_load(stack, -1); break;
         case BC_ILOADM1: run_int_load(stack, -1); break;
         case BC_DADD: run_double_binop(stack, fun_add, ip); break;
         case BC_IADD: run_int_binop(stack, fun_add, ip); break;
         case BC_DSUB: run_double_binop(stack, fun_sub, ip); break;
         case BC_ISUB: run_int_binop(stack, fun_sub, ip); break;
         case BC_DMUL: run_double_binop(stack, fun_mul, ip); break;
         case BC_IMUL: run_int_binop(stack, fun_mul, ip); break;
         case BC_DDIV: run_double_binop(stack, fun_div, ip); break;
         case BC_IDIV: run_int_binop(stack, fun_div, ip); break;
         case BC_IMOD: run_int_binop(stack, fun_mod, ip); break;
         case BC_DNEG: run_double_unop(stack, fun_neg, ip); break;
         case BC_INEG: run_int_unop(stack, fun_neg, ip); break;
         case BC_IAOR: run_int_binop(stack, fun_or, ip); break;
         case BC_IAAND: run_int_binop(stack, fun_and, ip); break;
         case BC_IAXOR: run_int_binop(stack, fun_xor, ip); break;
         case BC_IPRINT: {
            Var v = stack.back();
            stack.pop_back();
            interpreter_exception::assert_type(VT_INT, v.type(), ip);
            std::cout << v.getIntValue();
            break;
         }
         case BC_DPRINT: {
            Var v = stack.back();
            stack.pop_back();
            interpreter_exception::assert_type(VT_DOUBLE, v.type(), ip);
            std::cout << v.getDoubleValue();
            break;
         }
         case BC_SPRINT: {
            Var v = stack.back();
            stack.pop_back();
            interpreter_exception::assert_type(VT_STRING, v.type(), ip);
            std::cout << v.getStringValue();
            break;
         }
         case BC_I2D: {
            Var v = stack.back();
            stack.pop_back();
            interpreter_exception::assert_type(VT_INT, v.type(), ip);
            Var r(VT_DOUBLE, "");
            r.setDoubleValue(v.getIntValue());
            stack.push_back(r);
            break;
         }
         case BC_D2I: {
            Var v = stack.back();
            stack.pop_back();
            interpreter_exception::assert_type(VT_DOUBLE, v.type(), ip);
            Var r(VT_INT, "");
            r.setIntValue(static_cast<int64_t>(v.getDoubleValue()));
            stack.push_back(r);
            break;
         }
         case BC_S2I: {
            Var v = stack.back();
            stack.pop_back();
            interpreter_exception::assert_type(VT_STRING, v.type(), ip);
            Var r(VT_INT, "");
            stringstream ss(v.getStringValue());
            int64_t rval;
            ss >> rval;
            r.setIntValue(rval);
            stack.push_back(r);
            break;
         }
         case BC_SWAP: {
            Var v1 = stack.back();
            stack.pop_back();
            Var v2 = stack.back();
            stack.pop_back();
            stack.push_back(v1);
            stack.push_back(v2);
            break;
         }
         case BC_POP: stack.pop_back(); break;
         case BC_LOADDVAR0: run_loadvar(stack, varmap, 0); break;
         case BC_LOADDVAR1: run_loadvar(stack, varmap, 1); break;
         case BC_LOADDVAR2: run_loadvar(stack, varmap, 2); break;
         case BC_LOADDVAR3: run_loadvar(stack, varmap, 3); break;
         case BC_LOADDVAR: run_loadvar(stack, varmap, bc.getInt16(ip + 1));
                           break;
         case BC_LOADCTXDVAR: run_loadvar(stack, varmap, bc.getInt16(ip + 1),
                                    bc.getInt16(ip + 3));
                              break;
         case BC_LOADIVAR0: run_loadvar(stack, varmap, 0); break;
         case BC_LOADIVAR1: run_loadvar(stack, varmap, 1); break;
         case BC_LOADIVAR2: run_loadvar(stack, varmap, 2); break;
         case BC_LOADIVAR3: run_loadvar(stack, varmap, 3); break;
         case BC_LOADIVAR: run_loadvar(stack, varmap, bc.getInt16(ip + 1));
                           break;
         case BC_LOADCTXIVAR: run_loadvar(stack, varmap, bc.getInt16(ip + 1),
                                    bc.getInt16(ip + 3));
                              break;
         case BC_LOADSVAR0: run_loadvar(stack, varmap, 0); break;
         case BC_LOADSVAR1: run_loadvar(stack, varmap, 1); break;
         case BC_LOADSVAR2: run_loadvar(stack, varmap, 2); break;
         case BC_LOADSVAR3: run_loadvar(stack, varmap, 3); break;
         case BC_LOADSVAR: run_loadvar(stack, varmap, bc.getInt16(ip + 1));
                           break;
         case BC_LOADCTXSVAR: run_loadvar(stack, varmap, bc.getInt16(ip + 1),
                                    bc.getInt16(ip + 3));
                              break;
         case BC_STOREDVAR0: run_storevar(stack, varmap, 0); break;
         case BC_STOREDVAR1: run_storevar(stack, varmap, 1); break;
         case BC_STOREDVAR2: run_storevar(stack, varmap,  2); break;
         case BC_STOREDVAR3: run_storevar(stack, varmap,  3); break;
         case BC_STOREDVAR: run_storevar(stack, varmap, bc.getInt16(ip + 1));
                           break;
         case BC_STORECTXDVAR: run_storevar(stack, varmap, bc.getInt16(ip + 1),
                                    bc.getInt16(ip + 3));
                              break;
         case BC_STOREIVAR0: run_storevar(stack, varmap, 0); break;
         case BC_STOREIVAR1: run_storevar(stack, varmap, 1); break;
         case BC_STOREIVAR2: run_storevar(stack, varmap, 2); break;
         case BC_STOREIVAR3: run_storevar(stack, varmap, 3); break;
         case BC_STOREIVAR: run_storevar(stack, varmap, bc.getInt16(ip + 1));
                           break;
         case BC_STORECTXIVAR: run_storevar(stack, varmap, bc.getInt16(ip + 1),
                                    bc.getInt16(ip + 3));
                              break;
         case BC_STORESVAR0: run_storevar(stack, varmap, 0); break;
         case BC_STORESVAR1: run_storevar(stack, varmap, 1); break;
         case BC_STORESVAR2: run_storevar(stack, varmap, 2); break;
         case BC_STORESVAR3: run_storevar(stack, varmap, 3); break;
         case BC_STORESVAR: run_storevar(stack, varmap, bc.getInt16(ip + 1));
                           break;
         case BC_STORECTXSVAR: run_storevar(stack, varmap, bc.getInt16(ip + 1),
                                    bc.getInt16(ip + 3));
                              break;
         // TODO: is order correct?
         case BC_DCMP: {
            Var v1 = stack.back();
            stack.pop_back();
            Var v2 = stack.back();
            stack.pop_back();
            interpreter_exception::assert_type(VT_DOUBLE, v1.type(), v2.type(), ip);
            Var var(VT_INT, "");
            double d1 = v1.getDoubleValue();
            double d2 = v2.getDoubleValue();
            var.setIntValue(d1 < d2 ? -1 : (d1 == d2) ? 0 : 1);
            stack.push_back(var);
            break;
         }
         case BC_ICMP: {
            Var v1 = stack.back();
            stack.pop_back();
            Var v2 = stack.back();
            stack.pop_back();
            interpreter_exception::assert_type(VT_INT, v1.type(), v2.type(), ip);
            Var var(VT_INT, "");
            int64_t i1 = v1.getIntValue();
            int64_t i2 = v2.getIntValue();
            var.setIntValue(i1 < i2 ? -1 : (i1 == i2) ? 0 : 1);
            stack.push_back(var);
            break;
         }
         case BC_JA: {
            ip += bc.getInt16(ip + 1) + 1;
            continue;
         }
         case BC_IFICMPNE: {
            if(!run_if(stack, fun_ne, ip)) break;
            ip += bc.getInt16(ip + 1) + 1;
            continue;
         }
         case BC_IFICMPE: {
            if(!run_if(stack, fun_eq, ip)) break;
            ip += bc.getInt16(ip + 1) + 1;
            continue;
         }
         case BC_IFICMPG: {
            if(!run_if(stack, fun_gt, ip)) break;
            ip += bc.getInt16(ip + 1) + 1;
            continue;
         }
         case BC_IFICMPGE: {
            if(!run_if(stack, fun_ge, ip)) break;
            ip += bc.getInt16(ip + 1) + 1;
            continue;
         }
         case BC_IFICMPL: {
            if(!run_if(stack, fun_lt, ip)) break;
            ip += bc.getInt16(ip + 1) + 1;
            continue;
         }
         case BC_IFICMPLE: {
            if(!run_if(stack, fun_le, ip)) break;
            ip += bc.getInt16(ip + 1) + 1;
            continue;
         }
         case BC_DUMP: {
            Var v1 = stack.back();
            std::cerr << "DUMP: TOS(" << typeToName(v1.type()) << ") = ";
            if(v1.type() == VT_INT) std::cerr << v1.getIntValue();
            else if(v1.type() == VT_DOUBLE) std::cerr << v1.getDoubleValue();
            else std::cerr << v1.getStringValue();
            std::cerr << std::endl;
         }
         case BC_STOP: ip_stack.clear(); bc_stack.clear(); continue;
         case BC_CALL: {
            TranslatedFunction* f = code.functionById(bc.getInt16(ip + 1));
            bc_stack.push_back(static_cast<BytecodeFunction*>(f)->bytecode());
            ip_stack.push_back(0);
            loggerstr << "Calling function with varmap: " << varmap << std::endl;
            continue;
         }
         case BC_CALLNATIVE: run_native(code, stack, bc.getInt16(ip + 1)); break;
         case BC_RETURN: {
            ip_stack.pop_back();
            bc_stack.pop_back();
            if(!ip_stack.empty()) ip_stack.back() += bc_length(BC_CALL);
            continue;
         }
         case BC_BREAK: {
            std::cerr << "WARNING: BREAK instruction unsupported" << std::endl;
            break;
         }
         default: throw interpreter_exception("Unknown instruction found", ip);
      }
      ip += bc_length(insn);
   }
}

Status* BytecodeImpl::execute(vector<Var*>& vars) {
   stack_t stack;
   varmap_t varmap;
   // TODO: fill varmap with vars
   try {
      run(*this, stack, varmap, _bytecode);
   } catch(interpreter_exception const& e) {
      return new Status(e.what(), e.position());
   }
   return NULL;
}
