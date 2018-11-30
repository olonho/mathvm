//
// Created by alexvangogen on 11.11.18.
//

#include "program_state.h"

namespace mathvm {
	program_state::program_state(mathvm::Bytecode *_bytecode, uint32_t scope_id)
		: bytecode(_bytecode), bptr(0), topmost_scope_id(scope_id) {
	}

	program_state::~program_state() {
	}

	void program_state::push(mathvm::elem_t &element) {
		program_stack.push_back(element);
	}

	elem_t program_state::top() const {
		return { *(program_stack.rbegin()) };
	}

	void program_state::pop() {
		program_stack.pop_back();
	}

	bool program_state::empty() {
		return program_stack.size() == 0;
	}

	uint32_t program_state::get_scope_id() {
		return topmost_scope_id;
	}

	void program_state::save(std::vector< std::vector<elem_t> > values) {
//		vars_values.clear();
//		std::vector<elem_t> v;
//		for (uint32_t i = 0; i < values.size(); ++i) {
//			vars_values.push_back(v);
//			for (uint32_t j = 0; j < values[i].size(); ++j)
//				vars_values.back().push_back(values[i][j]);
//		}
		vars_values = std::vector< std::vector<elem_t> >(values);
	}

	void program_state::restore(std::vector< std::vector<elem_t> >* target, uint32_t from_scope) {
//		target.clear();
//		std::vector<elem_t> v;
//		for (uint32_t i = 0; i < target->size(); ++i) {
//			for (uint32_t j = 0; j < (*target)[i].size(); ++j)
//				std::cout << i << '@' << j << '@' << (*target)[i][j].i << ' ';
//		}
////		std::cout << " ====> " << topmost_scope_id << " ====> ";
//		std::cout << '\n';
		for (uint32_t scope_id = from_scope; scope_id < target->size(); ++scope_id)
			(*target)[scope_id] = std::vector<elem_t>(vars_values[scope_id]);
//		for (uint32_t i = 0; i < target->size(); ++i) {
//			for (uint32_t j = 0; j < (*target)[i].size(); ++j)
//				std::cout << i << '@' << j << '@' << (*target)[i][j].i << ' ';
//		}
//		std::cout << std::endl;
//		*target = std::vector< std::vector<elem_t> >(vars_values);
	}

	// 1 byte
	Instruction program_state::get_insn() {
		Instruction insn = bytecode->getInsn(bptr);
		++bptr;
		return insn;
	}

	double program_state::get_double() {
		double value = bytecode->getDouble(bptr);
		bptr += sizeof(double);
		return value;
	}

	int16_t program_state::get_int16() {
		int16_t value = bytecode->getInt16(bptr);
		bptr += 2;
		return value;
	}

	uint16_t program_state::get_uint16() {
		uint16_t value = bytecode->getUInt16(bptr);
		bptr += 2;
		return value;
	}

	int32_t program_state::get_int32() {
		int32_t value = bytecode->getTyped<int32_t>(bptr);
		bptr += 4;
		return value;
	}

	int64_t program_state::get_int64() {
		int64_t value = bytecode->getInt64(bptr);
		bptr += 8;
		return value;
	}

	bool program_state::bytecode_ended() {
		return bptr >= bytecode->length();
	}

	void program_state::move_bptr(int16_t offset) {
		bptr += offset;
	}
}