//
// Created by alexvangogen on 02.11.18.
//

#ifndef MATHVM_BYTECODE_INTERPRETER_H
#define MATHVM_BYTECODE_INTERPRETER_H

#include "ast.h"
#include "../../../../vm/parser.h"
#include <vector>
#include <map>
#include "mathvm.h"
#include "bytecode_translator.h"
#include "program_state.h"

namespace mathvm {

	class bytecode_interpreter_impl : public Translator {
	public:
		bytecode_interpreter_impl() = default;
		virtual Status* translate(const string& program, Code* *code);
	};

	class bytecode_interpreter {

		std::vector<program_state*> nested_functions_states;
		Code* context;
		std::map<const AstVar*, variable*> vars;
		std::vector< std::vector<elem_t> > vars_values;
		elem_t var0, var1, var2, var3;
		int i = 0;

	public:
		bytecode_interpreter(bytecode_translator& translator);
		~bytecode_interpreter();

		void run();

		#define VISIT_INSN(b, d, l) void visit_##b();
		FOR_BYTECODES(VISIT_INSN)
	};


} // namespace mathvm
#endif //MATHVM_BYTECODE_INTERPRETER_H
