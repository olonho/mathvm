#ifndef MATHVM_PROGRAM_STATE_H
#define MATHVM_PROGRAM_STATE_H

#include "ast.h"
#include "../../../../vm/parser.h"
#include <vector>
#include <map>
#include "mathvm.h"
#include "bytecode_translator.h"

namespace mathvm {

	class program_state {

		Bytecode *bytecode;
		std::vector <elem_t> program_stack;
		uint32_t bptr;
		std::vector< std::vector<elem_t> > vars_values;

	public:
		program_state(Bytecode *_bytecode);

		~program_state();

		void push(elem_t& element);
		elem_t top() const;
		void pop();
		bool empty();

		void save(std::vector< std::vector<elem_t> > values);
		void restore(std::vector< std::vector<elem_t> >* target);

		/**
		 * Get values and move bytecode pointer
		 */
		Instruction get_insn();

		double get_double();

		int16_t get_int16();

		uint16_t get_uint16();

		int32_t get_int32();

		int64_t get_int64();

		bool bytecode_ended();

		void move_bptr(int16_t offset);
	};
}
#endif //MATHVM_PROGRAM_STATE_H
