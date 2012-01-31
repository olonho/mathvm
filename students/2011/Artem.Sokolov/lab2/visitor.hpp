#ifndef _VISITOR_HPP_
#define _VISITOR_HPP_

#include <map>
#include "ast.h"
#include "mathvm.h"

class TranslationException {
	std::string message;
public:
	TranslationException(std::string const& message): message(message) {}
	std::string what() { return message; }
};

struct MyScope {
	std::map<std::string, uint16_t> variable_map;
	std::vector<mathvm::AstVar *> variables;

	std::map<std::string, uint16_t> function_map;
	std::vector<mathvm::BytecodeFunction *> functions;
};

class Visitor: public mathvm::AstVisitor {
	mathvm::Code* code;
	mathvm::Bytecode* bytecode;

	mathvm::VarType top_type;

	std::vector<MyScope> scopes;

	uint16_t add_variable(mathvm::AstVar *variable) {
		uint16_t id = scopes.back().variables.size();
		scopes.back().variables.push_back(variable);
		scopes.back().variable_map[variable->name()] = id;
		return id;
	}

	uint16_t add_function(mathvm::BytecodeFunction* function) {
		uint16_t id = scopes.back().functions.size();
		scopes.back().functions.push_back(function);
		scopes.back().function_map[function->name()] = id;

		code->addFunction(function);

		return id;
	}

	void add_var_id_to_bytecode(const mathvm::AstVar *variable) {
		for (short scope = scopes.size() - 1; scope >= 0; --scope) {
			std::map<std::string, uint16_t>::iterator it = scopes[scope].variable_map.find(variable->name());
			if (it == scopes[scope].variable_map.end()) {
				continue;
			} else {
				bytecode->addUInt16(scope);
				bytecode->addUInt16((*it).second);
				return;
			}
		}
		assert(false);
		throw TranslationException("Could not find the variable.");
	}

	void match_type(mathvm::VarType type) {
		if (top_type != type) {
			if (top_type == mathvm::VT_DOUBLE) {
				if (type == mathvm::VT_INT) {
					bytecode->addInsn(mathvm::BC_D2I);
				} else {
					assert(false);
					throw TranslationException("You can convert double only to int.");
				}
			} else if (top_type == mathvm::VT_INT) {
				if (type == mathvm::VT_DOUBLE) {
					bytecode->addInsn(mathvm::BC_I2D);
				} else {
					assert(false);
					throw TranslationException("You can convert int only to double.");
				}
			} else if (top_type == mathvm::VT_STRING) {
				if (type == mathvm::VT_DOUBLE) {
					bytecode->addInsn(mathvm::BC_S2I);
				} else {
					assert(false);
					throw TranslationException("You can convert string only to int.");
				}
			} else {
				assert(false);
				throw TranslationException("You can convert only int, double and string.");
			}
			top_type = type;
		}
	}

public:
	Visitor(mathvm::Code* code): code(code)
		, bytecode(((mathvm::BytecodeFunction *)(code->functionByName(mathvm::AstFunction::top_name)))->bytecode())
		, top_type(mathvm::VT_INVALID) {}

	virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node) {}

	virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node) {}

	virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node) {}

	virtual void visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
		bytecode->addInsn(mathvm::BC_DLOAD);
		bytecode->addDouble(node->literal());
		top_type = mathvm::VT_DOUBLE;
	}

	virtual void visitIntLiteralNode(mathvm::IntLiteralNode* node) {
		bytecode->addInsn(mathvm::BC_ILOAD);
		bytecode->addInt64(node->literal());
		top_type = mathvm::VT_INT;
	}

	virtual void visitLoadNode(mathvm::LoadNode* node) {}

	virtual void visitStoreNode(mathvm::StoreNode* node) {
		node->visitChildren(this);

		match_type(node->var()->type());

		switch (node->var()->type()) {
			case mathvm::VT_INT:
				switch (node->op()) {
				case mathvm::tINCRSET:
					bytecode->addInsn(mathvm::BC_LOADCTXIVAR);
					add_var_id_to_bytecode(node->var());
					bytecode->addInsn(mathvm::BC_IADD);
					break;
				case mathvm::tDECRSET:
					bytecode->addInsn(mathvm::BC_LOADCTXIVAR);
					add_var_id_to_bytecode(node->var());
					bytecode->addInsn(mathvm::BC_ISUB);
					break;
				default:
					break;
				}
				bytecode->addInsn(mathvm::BC_STORECTXIVAR);
				add_var_id_to_bytecode(node->var());
				break;
			case mathvm::VT_DOUBLE:
				switch (node->op()) {
				case mathvm::tINCRSET:
					bytecode->addInsn(mathvm::BC_LOADCTXDVAR);
					add_var_id_to_bytecode(node->var());
					bytecode->addInsn(mathvm::BC_DADD);
					break;
				case mathvm::tDECRSET:
					bytecode->addInsn(mathvm::BC_LOADCTXDVAR);
					add_var_id_to_bytecode(node->var());
					bytecode->addInsn(mathvm::BC_DSUB);
					break;
				default:
					break;
				}
				bytecode->addInsn(mathvm::BC_STORECTXDVAR);
				add_var_id_to_bytecode(node->var());
				break;
			case mathvm::VT_STRING:
				node->visitChildren(this);
				if (node->op() == mathvm::tASSIGN) {
					if (top_type == mathvm::VT_STRING) {
						bytecode->addInsn(mathvm::BC_STORECTXSVAR);
						add_var_id_to_bytecode(node->var());
					} else {
						assert(false);
						throw TranslationException("Tried to assign another value type to string variable.");
					}
				} else {
					assert(false);
					throw TranslationException("Strings are immutable.");
				}
				break;
			default:
				assert(false);
				throw TranslationException("Tried to assign to invalid or void.");
				break;
		}
	}

	virtual void visitForNode(mathvm::ForNode* node) {}

	virtual void visitWhileNode(mathvm::WhileNode* node) {}

	virtual void visitIfNode(mathvm::IfNode* node) {}

	virtual void visitBlockNode(mathvm::BlockNode* node) {
		scopes.push_back(MyScope());
		mathvm::Scope::FunctionIterator function_iterator(node->scope());
		while (function_iterator.hasNext()) {
			mathvm::AstFunction *ast_function = function_iterator.next();
			mathvm::BytecodeFunction* bytecode_function = new mathvm::BytecodeFunction(ast_function);
			add_function(bytecode_function);

			ast_function->node()->visitChildren(this);
		}

		mathvm::Scope::VarIterator variable_iterator(node->scope());
		while (variable_iterator.hasNext()) {
			mathvm::AstVar *ast_variable = variable_iterator.next();
			add_variable(ast_variable);
		}

		node->visitChildren(this);

		scopes.pop_back();
	}

	virtual void visitFunctionNode(mathvm::FunctionNode* node) {
		node->visitChildren(this);
	}

	virtual void visitReturnNode(mathvm::ReturnNode* node) {}

	virtual void visitCallNode(mathvm::CallNode* node) {}

	//virtual void visitNativeCallNode(mathvm::NativeCallNode* node) {}

	virtual void visitPrintNode(mathvm::PrintNode* node) {}
};

#endif // _VISITOR_HPP_
