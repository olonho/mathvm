#include "bytecode_interpreter.h"
#include "program_state.h"
#include <sstream>
#include <stack>

namespace mathvm {

	Status* bytecode_interpreter_impl::translate(const string& program, Code* *code) {
		Parser parser;
		Status* status = parser.parseProgram(program);
		if (status->isOk()) {
			bytecode_translator visitor;
			visitor.traverse_scope(parser.top()->owner());
			*code = visitor.get_code();
			bytecode_interpreter interpreter(visitor);
			interpreter.run();
		}
		return status;
	}

	bytecode_interpreter::bytecode_interpreter(mathvm::bytecode_translator &translator) {
		program_state state(translator.get_bytecode(), 0);
		nested_functions_states.push_back(state);
		context = translator.get_code();
		vars = translator.get_vars();
		vars_values = translator.get_vars_values();
		var0 = translator.get_var0();
		var1 = translator.get_var1();
		var2 = translator.get_var2();
		var3 = translator.get_var3();
	}

	bytecode_interpreter::~bytecode_interpreter() {
	}

	void bytecode_interpreter::visit_DLOAD() {
		elem_t elem;
		elem.d = nested_functions_states.back().get_double();
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_ILOAD() {
		elem_t elem;
		elem.i = nested_functions_states.back().get_int64();
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_SLOAD() {
		uint16_t string_id = nested_functions_states.back().get_uint16();
		elem_t elem;
		elem.s = context->constantById(string_id).c_str();
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_DLOAD0() {
		elem_t elem;
		elem.d = 0.0;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_ILOAD0() {
		elem_t elem;
		elem.i = 0;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_SLOAD0() {
		elem_t elem;
		elem.s = "";
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_DLOAD1() {
		elem_t elem;
		elem.d = 1.0;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_ILOAD1() {
		elem_t elem;
		elem.i = 1;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_DLOADM1() {
		elem_t elem;
		elem.d = -1.0;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_ILOADM1() {
		elem_t elem;
		elem.i = -1;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_DADD() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.d = upper.d + lower.d;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_IADD() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = upper.i + lower.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_DSUB() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.d = upper.d - lower.d;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_ISUB() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = upper.i - lower.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_DMUL() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.d = upper.d * lower.d;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_IMUL() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = upper.i * lower.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_DDIV() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.d = upper.d / lower.d;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_IDIV() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = upper.i / lower.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_IMOD() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = upper.i % lower.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_DNEG() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.d = -upper.d;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_INEG() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = -upper.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_IAOR() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = upper.i | lower.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_IAAND() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = upper.i & lower.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_IAXOR() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = upper.i ^ lower.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_IPRINT() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		std::cout << upper.i;
	}

	void bytecode_interpreter::visit_DPRINT() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		std::cout << upper.d;
	}

	void bytecode_interpreter::visit_SPRINT() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		std::cout << upper.s;
	}

	void bytecode_interpreter::visit_I2D() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.d = (double)elem.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_D2I() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t elem;
		elem.i = (int64_t)elem.d;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_S2I() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		stringstream s2i(upper.s);
		elem_t elem;
		s2i >> elem.i;
		nested_functions_states.back().push(elem);
	}

	void bytecode_interpreter::visit_POP() {
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_SWAP() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		nested_functions_states.back().push(upper);
		nested_functions_states.back().push(lower);
	}

	void bytecode_interpreter::visit_LOADDVAR0() {
		nested_functions_states.back().push(var0);
	}

	void bytecode_interpreter::visit_LOADDVAR1() {
		nested_functions_states.back().push(var1);
	}

	void bytecode_interpreter::visit_LOADDVAR2() {
		nested_functions_states.back().push(var2);
	}

	void bytecode_interpreter::visit_LOADDVAR3() {
		nested_functions_states.back().push(var3);
	}

	void bytecode_interpreter::visit_LOADIVAR0() {
		nested_functions_states.back().push(var0);
	}

	void bytecode_interpreter::visit_LOADIVAR1() {
		nested_functions_states.back().push(var1);
	}

	void bytecode_interpreter::visit_LOADIVAR2() {
		nested_functions_states.back().push(var2);
	}

	void bytecode_interpreter::visit_LOADIVAR3() {
		nested_functions_states.back().push(var3);
	}

	void bytecode_interpreter::visit_LOADSVAR0() {
		nested_functions_states.back().push(var0);
	}

	void bytecode_interpreter::visit_LOADSVAR1() {
		nested_functions_states.back().push(var1);
	}

	void bytecode_interpreter::visit_LOADSVAR2() {
		nested_functions_states.back().push(var2);
	}

	void bytecode_interpreter::visit_LOADSVAR3() {
		nested_functions_states.back().push(var3);
	}

	void bytecode_interpreter::visit_STOREDVAR0() {
		var0 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STOREDVAR1() {
		var1 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STOREDVAR2() {
		var2 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STOREDVAR3() {
		var3 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STOREIVAR0() {
		var0 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STOREIVAR1() {
		var1 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STOREIVAR2() {
		var2 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STOREIVAR3() {
		var3 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STORESVAR0() {
		var0 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STORESVAR1() {
		var1 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STORESVAR2() {
		var2 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_STORESVAR3() {
		var3 = nested_functions_states.back().top();
		nested_functions_states.back().pop();
	}

	void bytecode_interpreter::visit_LOADCTXDVAR() {
		uint16_t scope_id = nested_functions_states.back().get_uint16();
		uint16_t var_id = nested_functions_states.back().get_uint16();
		elem_t var_elem = vars_values[scope_id][var_id];
		nested_functions_states.back().push(var_elem);
	}

	void bytecode_interpreter::visit_LOADCTXIVAR() {
		uint16_t scope_id = nested_functions_states.back().get_uint16();
		uint16_t var_id = nested_functions_states.back().get_uint16();
		elem_t var_elem = vars_values[scope_id][var_id];
		nested_functions_states.back().push(var_elem);
	}

	void bytecode_interpreter::visit_LOADCTXSVAR() {
		uint16_t scope_id = nested_functions_states.back().get_uint16();
		uint16_t var_id = nested_functions_states.back().get_uint16();
		elem_t var_elem = vars_values[scope_id][var_id];
		nested_functions_states.back().push(var_elem);
	}

	void bytecode_interpreter::visit_STORECTXDVAR() {
		uint16_t scope_id = nested_functions_states.back().get_uint16();
		uint16_t var_id = nested_functions_states.back().get_uint16();
		elem_t new_var_elem = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		vars_values[scope_id][var_id] = new_var_elem;
	}

	void bytecode_interpreter::visit_STORECTXIVAR() {
		uint16_t scope_id = nested_functions_states.back().get_uint16();
		uint16_t var_id = nested_functions_states.back().get_uint16();
		elem_t new_var_elem = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		vars_values[scope_id][var_id] = new_var_elem;
	}

	void bytecode_interpreter::visit_STORECTXSVAR() {
		uint16_t scope_id = nested_functions_states.back().get_uint16();
		uint16_t var_id = nested_functions_states.back().get_uint16();
		elem_t new_var_elem = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		vars_values[scope_id][var_id] = new_var_elem;
	}

	void bytecode_interpreter::visit_DCMP() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t cmp_elem;
		if (upper.d == lower.d)
			cmp_elem.i = 0;
		else if (upper.d < lower.d)
			cmp_elem.i = -1;
		else
			cmp_elem.i = 1;
		nested_functions_states.back().push(cmp_elem);
	}

	void bytecode_interpreter::visit_ICMP() {
		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t cmp_elem;
		if (upper.i == lower.i)
			cmp_elem.i = 0;
		else if (upper.i < lower.i)
			cmp_elem.i = -1;
		else
			cmp_elem.i = 1;
		nested_functions_states.back().push(cmp_elem);
	}

	void bytecode_interpreter::visit_JA() {
		int16_t offset = nested_functions_states.back().get_int16();
		nested_functions_states.back().move_bptr(offset - 2);
	}

	void bytecode_interpreter::visit_IFICMPNE() {
		int16_t offset = nested_functions_states.back().get_int16();

		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		if (upper.i != lower.i) {
			nested_functions_states.back().move_bptr(offset - 2);
		}
	}

	void bytecode_interpreter::visit_IFICMPE() {
		int16_t offset = nested_functions_states.back().get_int16();

		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		if (upper.i == lower.i) {
			nested_functions_states.back().move_bptr(offset - 2);
		}
	}

	void bytecode_interpreter::visit_IFICMPL() {
		int16_t offset = nested_functions_states.back().get_int16();

		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		if (upper.i < lower.i) {
			nested_functions_states.back().move_bptr(offset - 2);
		}
	}

	void bytecode_interpreter::visit_IFICMPLE() {
		int16_t offset = nested_functions_states.back().get_int16();

		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		if (upper.i <= lower.i) {
			nested_functions_states.back().move_bptr(offset - 2);
		}
	}

	void bytecode_interpreter::visit_IFICMPG() {
		int16_t offset = nested_functions_states.back().get_int16();

		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		if (upper.i > lower.i) {
			nested_functions_states.back().move_bptr(offset - 2);
		}
	}

	void bytecode_interpreter::visit_IFICMPGE() {
		int16_t offset = nested_functions_states.back().get_int16();

		elem_t upper = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		elem_t lower = nested_functions_states.back().top();
		nested_functions_states.back().pop();
		if (upper.i >= lower.i) {
			nested_functions_states.back().move_bptr(offset - 2);
		}
	}

	void bytecode_interpreter::visit_CALL() {
//		double start = getTimeInSeconds();
//		std::cout << nested_functions_states.size() << std::endl;
		uint16_t func_id = nested_functions_states.back().get_uint16();
		TranslatedFunctionWrapper* called_func = static_cast<TranslatedFunctionWrapper*>(context->functionById(func_id));
		program_state called_function_state(called_func->get_bytecode(), called_func->get_body_scope_id());

		std::stack<elem_t> passed_params;
		for (uint32_t param_id = 0; param_id < called_func->parametersNumber(); ++param_id) {
			elem_t elem = nested_functions_states.back().top();
			nested_functions_states.back().pop();
			passed_params.push(elem);
		}
		while (!passed_params.empty()) {
			called_function_state.push(passed_params.top());
			passed_params.pop();
		}
		nested_functions_states.back().save(vars_values);
		nested_functions_states.push_back(called_function_state);
//		std::cout << "time: " << getTimeInSeconds() - start << std::endl;
	}

	void bytecode_interpreter::visit_RETURN() {
		uint16_t function_scope_id = nested_functions_states.back().get_scope_id();
//		function_scope_id;
		if (!nested_functions_states.back().empty()) {
			elem_t elem = nested_functions_states.back().top();
			nested_functions_states.back().pop();
			nested_functions_states.pop_back();
			nested_functions_states.back().push(elem);
		} else {
			nested_functions_states.pop_back();
		}
		if (nested_functions_states.size() > 1)
			nested_functions_states.back().restore(&vars_values, function_scope_id);
	}

	void bytecode_interpreter::run() {
		while (!nested_functions_states.back().bytecode_ended()) {
//		stringstream ss;
//		ss << nested_functions_states.back().get_insn();
//		throw std::invalid_argument(ss.str());
			switch (nested_functions_states.back().get_insn()) {
				case BC_DLOAD:
					visit_DLOAD();
					break;
				case BC_ILOAD:
					visit_ILOAD();
					break;
				case BC_SLOAD:
					visit_SLOAD();
					break;
				case BC_DLOAD0:
					visit_DLOAD0();
					break;
				case BC_ILOAD0:
					visit_ILOAD0();
					break;
				case BC_SLOAD0:
					visit_SLOAD0();
					break;
				case BC_DLOAD1:
					visit_DLOAD1();
					break;
				case BC_ILOAD1:
					visit_ILOAD1();
					break;
				case BC_DLOADM1:
					visit_DLOADM1();
					break;
				case BC_ILOADM1:
					visit_ILOADM1();
					break;
				case BC_DADD:
					visit_DADD();
					break;
				case BC_IADD:
					visit_IADD();
					break;
				case BC_DSUB:
					visit_DSUB();
					break;
				case BC_ISUB:
					visit_ISUB();
					break;
				case BC_DMUL:
					visit_DMUL();
					break;
				case BC_IMUL:
					visit_IMUL();
					break;
				case BC_DDIV:
					visit_DDIV();
					break;
				case BC_IDIV:
					visit_IDIV();
					break;
				case BC_IMOD:
					visit_IMOD();
					break;
				case BC_DNEG:
					visit_DNEG();
					break;
				case BC_INEG:
					visit_INEG();
					break;
				case BC_IAOR:
					visit_IAOR();
					break;
				case BC_IAAND:
					visit_IAAND();
					break;
				case BC_IAXOR:
					visit_IAXOR();
					break;
				case BC_DPRINT:
					visit_DPRINT();
					break;
				case BC_IPRINT:
					visit_IPRINT();
					break;
				case BC_SPRINT:
					visit_SPRINT();
					break;
				case BC_I2D:
					visit_I2D();
					break;
				case BC_D2I:
					visit_D2I();
					break;
				case BC_S2I:
					visit_S2I();
					break;
				case BC_SWAP:
					visit_SWAP();
					break;
				case BC_POP:
					visit_POP();
					break;
				case BC_LOADDVAR0:
					visit_LOADDVAR0();
					break;
				case BC_LOADDVAR1:
					visit_LOADDVAR1();
					break;
				case BC_LOADDVAR2:
					visit_LOADDVAR2();
					break;
				case BC_LOADDVAR3:
					visit_LOADDVAR3();
					break;
				case BC_LOADIVAR0:
					visit_LOADIVAR0();
					break;
				case BC_LOADIVAR1:
					visit_LOADIVAR1();
					break;
				case BC_LOADIVAR2:
					visit_LOADIVAR2();
					break;
				case BC_LOADIVAR3:
					visit_LOADIVAR3();
					break;
				case BC_LOADSVAR0:
					visit_LOADSVAR0();
					break;
				case BC_LOADSVAR1:
					visit_LOADSVAR1();
					break;
				case BC_LOADSVAR2:
					visit_LOADSVAR2();
					break;
				case BC_LOADSVAR3:
					visit_LOADSVAR3();
					break;
				case BC_STOREDVAR0:
					visit_STOREDVAR0();
					break;
				case BC_STOREDVAR1:
					visit_STOREDVAR1();
					break;
				case BC_STOREDVAR2:
					visit_STOREDVAR2();
					break;
				case BC_STOREDVAR3:
					visit_STOREDVAR3();
					break;
				case BC_STOREIVAR0:
					visit_STOREIVAR0();
					break;
				case BC_STOREIVAR1:
					visit_STOREIVAR1();
					break;
				case BC_STOREIVAR2:
					visit_STOREIVAR2();
					break;
				case BC_STOREIVAR3:
					visit_STOREIVAR3();
					break;
				case BC_STORESVAR0:
					visit_STORESVAR0();
					break;
				case BC_STORESVAR1:
					visit_STORESVAR1();
					break;
				case BC_STORESVAR2:
					visit_STORESVAR2();
					break;
				case BC_STORESVAR3:
					visit_STORESVAR3();
					break;
				case BC_LOADCTXDVAR:
					visit_LOADCTXDVAR();
					break;
				case BC_LOADCTXIVAR:
					visit_LOADCTXIVAR();
					break;
				case BC_LOADCTXSVAR:
					visit_LOADCTXSVAR();
					break;
				case BC_STORECTXDVAR:
					visit_STORECTXDVAR();
					break;
				case BC_STORECTXIVAR:
					visit_STORECTXIVAR();
					break;
				case BC_STORECTXSVAR:
					visit_STORECTXSVAR();
					break;
				case BC_DCMP:
					visit_DCMP();
					break;
				case BC_ICMP:
					visit_ICMP();
					break;
				case BC_JA:
					visit_JA();
					break;
				case BC_IFICMPNE:
					visit_IFICMPNE();
					break;
				case BC_IFICMPE:
					visit_IFICMPE();
					break;
				case BC_IFICMPL:
					visit_IFICMPL();
					break;
				case BC_IFICMPLE:
					visit_IFICMPLE();
					break;
				case BC_IFICMPG:
					visit_IFICMPG();
					break;
				case BC_IFICMPGE:
					visit_IFICMPGE();
					break;
				case BC_CALL:
					visit_CALL();
					break;
				case BC_RETURN:
					visit_RETURN();
					break;
				default:
					break;
			}
		}
	}

} // namespace mathvm