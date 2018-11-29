#ifndef MATHVM_BYTECODE_TRANSLATOR_H
#define MATHVM_BYTECODE_TRANSLATOR_H

#include "ast.h"
#include "../../../../vm/parser.h"
#include <vector>
#include <map>
#include <set>
#include "mathvm.h"
#include <stack>

namespace mathvm {

	class variable;
	class function;

	union elem_t {
		int64_t i;
		double d;
		const char* s;

		~elem_t() {}
	};

	class bytecode_translator_impl: public Translator {
	public:
		bytecode_translator_impl() = default;
		virtual Status* translate(const string& program, Code* *code);
	};

	class CCode : public Code {
		virtual Status* execute(std::vector<Var*>& vars) {
			return Status::Ok();
		}
	};

	class TranslatedFunctionWrapper : public TranslatedFunction {
		AstFunction* astFunction;
		Bytecode* bytecode;
		std::map<const AstVar*, uint32_t> params_map;
		std::set<const AstVar*> local_params;
		uint16_t body_scope_id;

	public:
		TranslatedFunctionWrapper(AstFunction* function)
			: TranslatedFunction(function), astFunction(function), bytecode(new Bytecode()) {}

//		~TranslatedFunctionWrapper() {
//			delete astFunction;
//			delete bytecode;
//		}

		virtual void disassemble(ostream& out) const {}
		AstFunction* get_function() const {
			return astFunction;
		}
		Bytecode* get_bytecode() const {
			return bytecode;
		}
		void set_bytecode(Bytecode* bc) {
			bytecode = bc;
		}
		void add_param(const AstVar* var, uint32_t id) {
			params_map[var] = id;
		}
		uint32_t get_id(const AstVar* var) {
			return params_map[var];
		}
		std::map<const AstVar*, uint32_t>::iterator get_param(const AstVar* var) {
			return params_map.find(var);
		}
		bool contains(const AstVar* var) {
			return get_param(var) != params_map.end();
		}
		uint16_t get_body_scope_id() {
			return body_scope_id;
		}
		void set_body_scope_id(uint16_t scope_id) {
			body_scope_id = scope_id;
		}
	};

	class bytecode_translator : public AstVisitor {

		Code* code = new CCode;
		Bytecode *function_bytecode = new Bytecode();
		Bytecode *bytecode = new Bytecode();
		VarType current_subtree_type = VT_INVALID;
		std::vector<Scope*> scopes;
		std::vector< std::vector<elem_t> > vars_values;
		std::vector<Bytecode*> nested_functions_bytecodes;
		std::stack<VarType> type_stack;
		elem_t var0, var1, var2, var3;
		std::map<const AstVar*, variable*> vars;
		std::map<const TranslatedFunctionWrapper*, function*> functions;
		std::map<uint16_t, Bytecode*> functions_bytecodes;
		uint16_t last_func_id = 0;
		TranslatedFunctionWrapper* top;
		TranslatedFunctionWrapper* current_function;
		std::vector<TranslatedFunctionWrapper*> functions_declarations_stack;

	public:

		~bytecode_translator() {
//			delete code;
			delete bytecode;
//			if (current_node != nullptr)
//				delete current_node;
//			delete current_function;
//			for (uint32_t i = 0; i < scopes.size(); ++i)
//				delete scopes[i];
//			for (uint32_t i = 0; i < nested_functions_bytecodes.size(); ++i)
//				delete nested_functions_bytecodes[nested_functions_bytecodes.size() - i - 1];
//			for (uint32_t i = 0; i < functions_declarations_stack.size(); ++i)
//				delete functions_declarations_stack[i];
		}

		virtual void visitBinaryOpNode(BinaryOpNode *node) override;

		virtual void visitUnaryOpNode(UnaryOpNode *node) override;

		virtual void visitStringLiteralNode(StringLiteralNode *node) override;

		virtual void visitIntLiteralNode(IntLiteralNode *node) override;

		virtual void visitDoubleLiteralNode(DoubleLiteralNode *node) override;

		virtual void visitLoadNode(LoadNode *node) override;

		virtual void visitStoreNode(StoreNode *node) override;

		virtual void visitBlockNode(BlockNode *node) override;

		virtual void visitNativeCallNode(NativeCallNode *node) override;

		virtual void visitForNode(ForNode *node) override;

		virtual void visitWhileNode(WhileNode *node) override;

		virtual void visitIfNode(IfNode *node) override;

		virtual void visitReturnNode(ReturnNode *node) override;

		virtual void visitFunctionNode(FunctionNode *node) override;

		virtual void visitCallNode(CallNode *node) override;

		virtual void visitPrintNode(PrintNode *node) override;

		void traverse_scope(Scope* scope);
		void print_bytecode() const;

		Code* get_code() const;
		Bytecode* get_bytecode() const;
		std::map<const AstVar*, variable*> get_vars() const;
		std::vector< std::vector<elem_t> > get_vars_values() const;
		elem_t get_var0() const;
		elem_t get_var1() const;
		elem_t get_var2() const;
		elem_t get_var3() const;

	private:
		void first_to_double();
		void first_to_int();
		void second_to_double();
		void require_int_or_double();
		void require_int();
		void check_binary_op_correctness(BinaryOpNode* op);
		void check_unary_op_correctness(UnaryOpNode* op);

		VarType translate_binop(TokenKind kind);
		VarType translate_unop(TokenKind kind);
		VarType translate_lazy_and(BinaryOpNode* node);
		VarType translate_lazy_or(BinaryOpNode* node);
		void translate_cmp(TokenKind cmp_kind, VarType cmp_type);

		void translate_function(TranslatedFunctionWrapper* wrapper);
		void translate_load(const AstVar* var);
		void translate_store(const AstVar* var);
		Instruction get_load_insn(const AstVar* var);
		Instruction get_store_insn(const AstVar* var);
		Instruction get_local_load_insn(const AstVar* var);
		Instruction get_local_store_insn(const AstVar* var);

		void save_bytecode(bool create_new_bytecode);
		void flush_bytecode_to_function(TranslatedFunctionWrapper* wrapper);

		VarType resolve_int_or_double_binary(VarType left, VarType right);
		VarType resolve_int_binary(VarType left, VarType right);
		VarType resolve_int_or_double_unary(VarType top_type);
		VarType resolve_int_unary(VarType top_type);

		int32_t try_find_local(const AstVar* param);
	};

	struct variable {
		uint16_t scope_id;
		uint16_t var_id;

		variable(uint16_t scope_id, uint16_t var_id)
			: scope_id(scope_id), var_id(var_id) {}
	};

	struct function {
		uint16_t scope_id;
		uint16_t func_id;

		function(uint16_t scope_id, uint16_t func_id)
			: scope_id(scope_id), func_id(func_id) {}
	};
}
#endif //MATHVM_BYTECODE_TRANSLATOR_H
