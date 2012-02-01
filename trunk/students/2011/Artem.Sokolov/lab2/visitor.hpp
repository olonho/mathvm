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

//	std::map<std::string, uint16_t> function_map;
//	std::vector<mathvm::BytecodeFunction *> functions;
};

class Visitor: public mathvm::AstVisitor {
	mathvm::Code* code;
	mathvm::Bytecode* bytecode;

	mathvm::VarType top_type;
	mathvm::VarType return_type;

	std::vector<MyScope> scopes;

	bool at_top;

	uint16_t add_variable(mathvm::AstVar *variable) {
		uint16_t id = scopes.back().variables.size();
		scopes.back().variables.push_back(variable);
		scopes.back().variable_map[variable->name()] = id;
		return id;
	}

//	uint16_t add_function(mathvm::BytecodeFunction* function) {
//		uint16_t id = scopes.back().functions.size();
//		scopes.back().functions.push_back(function);
//		scopes.back().function_map[function->name()] = id;
//
//		code->addFunction(function);
//
//		return id;
//	}

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

	void add_var_id_to_bytecode(const std::string &name) {
		for (short scope = scopes.size() - 1; scope >= 0; --scope) {
			std::map<std::string, uint16_t>::iterator it = scopes[scope].variable_map.find(name);
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
	Visitor(mathvm::Code* code):
		code(code)
		, bytecode(((mathvm::BytecodeFunction *)(code->functionByName(mathvm::AstFunction::top_name)))->bytecode())
		, top_type(mathvm::VT_INVALID)
		, return_type(mathvm::VT_VOID)
		, at_top(true) {}

	virtual void visitBinaryOpNode(mathvm::BinaryOpNode* node) {
		mathvm::VarType right_type;
		mathvm::VarType left_type;

		if (node->kind() == mathvm::tMOD) {
			node->right()->visit(this);
			right_type = top_type;
			node->left()->visit(this);
			left_type = top_type;
			assert(right_type == mathvm::VT_INT && left_type == mathvm::VT_INT);
			bytecode->addInsn(mathvm::BC_IMOD);
		} else if (node->kind() == mathvm::tADD || node->kind() == mathvm::tSUB || node->kind() == mathvm::tMUL || node->kind() == mathvm::tDIV) {
			node->right()->visit(this);
			right_type = top_type;
			node->left()->visit(this);
			left_type = top_type;

			mathvm::VarType node_type = mathvm::VT_DOUBLE;
			if (right_type == mathvm::VT_INT && left_type == mathvm::VT_INT) {
				node_type = mathvm::VT_INT;
			}

			if (left_type != node_type) {
				match_type(node_type);
			}
			if (right_type != node_type) {
				bytecode->addInsn(mathvm::BC_SWAP);
				match_type(node_type);
				bytecode->addInsn(mathvm::BC_SWAP);
			}

			switch (node->kind()) {
			case mathvm::tADD:
				if (node_type == mathvm::VT_INT) {
					bytecode->addInsn(mathvm::BC_IADD);
				} else {
					bytecode->addInsn(mathvm::BC_DADD);
				}
				break;
			case mathvm::tSUB:
				if (node_type == mathvm::VT_INT) {
					bytecode->addInsn(mathvm::BC_ISUB);
				} else {
					bytecode->addInsn(mathvm::BC_DSUB);
				}
				break;
			case mathvm::tMUL:
				if (node_type == mathvm::VT_INT) {
					bytecode->addInsn(mathvm::BC_IMUL);
				} else {
					bytecode->addInsn(mathvm::BC_DMUL);
				}
				break;
			case mathvm::tDIV:
				if (node_type == mathvm::VT_INT) {
					bytecode->addInsn(mathvm::BC_IDIV);
				} else {
					bytecode->addInsn(mathvm::BC_DDIV);
				}
				break;
			default:
				assert(false);
				throw TranslationException("Unknown operator.");
				break;
			}
		} else if (node->kind() == mathvm::tOR) {
			mathvm::Label return_true(bytecode);
			mathvm::Label return_false(bytecode);
			node->left()->visit(this);
			bytecode->addInsn(mathvm::BC_ILOAD0);
			bytecode->addBranch(mathvm::BC_IFICMPNE, return_true);
			node->right()->visit(this);
			bytecode->addInsn(mathvm::BC_ILOAD0);
			bytecode->addBranch(mathvm::BC_IFICMPNE, return_true);
			bytecode->addInsn(mathvm::BC_ILOAD0);
			bytecode->addBranch(mathvm::BC_JA, return_false);
			bytecode->bind(return_true);
			bytecode->addInsn(mathvm::BC_ILOAD1);
			bytecode->bind(return_false);
		} else if (node->kind() == mathvm::tAND) {
			mathvm::Label return_false(bytecode);
			mathvm::Label return_true(bytecode);
			node->left()->visit(this);
			bytecode->addInsn(mathvm::BC_ILOAD0);
			bytecode->addBranch(mathvm::BC_IFICMPE, return_false);
			node->right()->visit(this);
			bytecode->addInsn(mathvm::BC_ILOAD0);
			bytecode->addBranch(mathvm::BC_IFICMPE, return_false);
			bytecode->addInsn(mathvm::BC_ILOAD1);
			bytecode->addBranch(mathvm::BC_JA, return_true);
			bytecode->bind(return_false);
			bytecode->addInsn(mathvm::BC_ILOAD0);
			bytecode->bind(return_true);
		} else if (node->kind() == mathvm::tEQ || node->kind() == mathvm::tNEQ || node->kind() == mathvm::tGT || node->kind() == mathvm::tGE || node->kind() == mathvm::tLT || node->kind() == mathvm::tLE) {
			node->right()->visit(this);
			right_type = top_type;
			node->left()->visit(this);
			left_type = top_type;

			assert(right_type == mathvm::VT_INT && left_type == mathvm::VT_INT);

			mathvm::Label return_true(bytecode);
			mathvm::Label return_false(bytecode);

			switch (node->kind()) {
			case mathvm::tEQ:
				bytecode->addBranch(mathvm::BC_IFICMPE, return_true);
				break;
			case mathvm::tNEQ:
				bytecode->addBranch(mathvm::BC_IFICMPNE, return_true);
				break;
			case mathvm::tGT:
				bytecode->addBranch(mathvm::BC_IFICMPG, return_true);
				break;
			case mathvm::tGE:
				bytecode->addBranch(mathvm::BC_IFICMPGE, return_true);
				break;
			case mathvm::tLT:
				bytecode->addBranch(mathvm::BC_IFICMPL, return_true);
				break;
			case mathvm::tLE:
				bytecode->addBranch(mathvm::BC_IFICMPLE, return_true);
				break;
			default:
				std::cerr << "busy doing nothing" << std::endl;
				break;
			}
			bytecode->addInsn(mathvm::BC_ILOAD0);
			bytecode->addBranch(mathvm::BC_JA, return_false);
			bytecode->bind(return_true);
			bytecode->addInsn(mathvm::BC_ILOAD1);
			bytecode->bind(return_false);
		}
	}

	virtual void visitUnaryOpNode(mathvm::UnaryOpNode* node) {
		node->operand()->visit(this);
		mathvm::Label return_true(bytecode);;
		mathvm::Label return_false(bytecode);
		switch (node->kind()) {
		case mathvm::tSUB:
			switch(top_type) {
			case mathvm::VT_INT :
				bytecode->addInsn(mathvm::BC_INEG);
				break;
			case mathvm::VT_DOUBLE :
				bytecode->addInsn(mathvm::BC_DNEG);
				break;
			default:
				assert(false);
				throw TranslationException("Unary minus is only applicable to int or double.");
				break;
			}
			break;
		case mathvm::tNOT:
			assert(top_type == mathvm::VT_INT);
			bytecode->addInsn(mathvm::BC_ILOAD0);
			bytecode->addBranch(mathvm::BC_IFICMPE, return_true);
			bytecode->addInsn(mathvm::BC_ILOAD0);
			bytecode->addBranch(mathvm::BC_JA, return_false);
			bytecode->bind(return_true);
			bytecode->add(mathvm::BC_ILOAD1);
			bytecode->bind(return_false);
			break;
		default:
			assert(false);
			throw TranslationException("Unknown unary operation.");
			break;
		}
	}

	virtual void visitStringLiteralNode(mathvm::StringLiteralNode* node) {
		bytecode->addInsn(mathvm::BC_SLOAD);
		bytecode->addUInt16(code->makeStringConstant(node->literal()));
		top_type = mathvm::VT_STRING;
	}

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

	virtual void visitLoadNode(mathvm::LoadNode* node) {
		switch (node->var()->type()) {
		case mathvm::VT_INT:
			bytecode->addInsn(mathvm::BC_LOADCTXIVAR);
			top_type = mathvm::VT_INT;
			break;
		case mathvm::VT_DOUBLE:
			bytecode->addInsn(mathvm::BC_LOADCTXDVAR);
			top_type = mathvm::VT_DOUBLE;
			break;
		case mathvm::VT_STRING:
			bytecode->addInsn(mathvm::BC_LOADCTXSVAR);
			top_type = mathvm::VT_STRING;
			break;
		default:
			assert(false);
			throw TranslationException("Tried to load invalid or void.");
			break;
		}
		add_var_id_to_bytecode(node->var());
	}

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

	virtual void visitForNode(mathvm::ForNode* node) {
		mathvm::Label condition(bytecode);
		mathvm::Label end(bytecode);
		mathvm::BinaryOpNode* in_expression = node->inExpr()->asBinaryOpNode();
		if (in_expression == NULL) {
			assert(false);
			throw TranslationException("In expression is empty.");
		}
		in_expression->left()->visit(this);
		bytecode->addInsn(mathvm::BC_STORECTXIVAR);
		add_var_id_to_bytecode(node->var()->name());

		bytecode->bind(condition);
		in_expression->right()->visit(this);
		bytecode->addInsn(mathvm::BC_LOADCTXIVAR);
		add_var_id_to_bytecode(node->var()->name());
		bytecode->addBranch(mathvm::BC_IFICMPG, end);

		node->body()->visit(this);

		bytecode->addInsn(mathvm::BC_ILOAD1);
		bytecode->addInsn(mathvm::BC_LOADCTXIVAR);
		add_var_id_to_bytecode(node->var()->name());
		bytecode->addInsn(mathvm::BC_IADD);
		bytecode->addInsn(mathvm::BC_STORECTXIVAR);
		add_var_id_to_bytecode(node->var()->name());

		bytecode->addBranch(mathvm::BC_JA, condition);
		bytecode->bind(end);
	}

	virtual void visitWhileNode(mathvm::WhileNode* node) {
		mathvm::Label condition(bytecode);
		mathvm::Label end(bytecode);

		bytecode->bind(condition);
		node->whileExpr()->visit(this);

		if (top_type != mathvm::VT_INT) {
			assert(false);
			throw TranslationException("While expression is invalid.");
		}

		bytecode->addInsn(mathvm::BC_ILOAD0);
		bytecode->addBranch(mathvm::BC_IFICMPE, end);

		node->loopBlock()->visit(this);
		bytecode->addBranch(mathvm::BC_JA, condition);

		bytecode->bind(end);
	}

	virtual void visitIfNode(mathvm::IfNode* node) {
		mathvm::Label its_false(bytecode);
		mathvm::Label end(bytecode);

		node->ifExpr()->visit(this);

		if (top_type != mathvm::VT_INT) {
			assert(false);
			throw TranslationException("If expression is invalid.");
		}

		bytecode->addInsn(mathvm::BC_ILOAD0);
		bytecode->addBranch(mathvm::BC_IFICMPE, its_false);

		node->thenBlock()->visit(this);
		bytecode->addBranch(mathvm::BC_JA, end);

		bytecode->bind(its_false);
		if (node->elseBlock()) {
			node->elseBlock()->visit(this);
		}

		bytecode->bind(end);
	}

	virtual void visitBlockNode(mathvm::BlockNode* node) {
		if (at_top) {
			scopes.push_back(MyScope());
			at_top = false;
		}

		mathvm::Scope::VarIterator variable_iterator(node->scope());
		while (variable_iterator.hasNext()) {
			mathvm::AstVar *ast_variable = variable_iterator.next();
			add_variable(ast_variable);
		}

		mathvm::Scope::FunctionIterator function_iterator(node->scope());
		while (function_iterator.hasNext()) {
			mathvm::AstFunction *ast_function = function_iterator.next();
			mathvm::BytecodeFunction* bytecode_function = new mathvm::BytecodeFunction(ast_function);
//			add_function(bytecode_function);
			code->addFunction(bytecode_function);

			mathvm::Bytecode *outer_bytecode = bytecode;
			mathvm::VarType outer_return_type = return_type;
			return_type = ast_function->returnType();
			bytecode = bytecode_function->bytecode();

			scopes.push_back(MyScope());

			for (uint16_t i = 0; i < ast_function->parametersNumber(); ++i) {
				mathvm::AstVar *ast_variable = new mathvm::AstVar(ast_function->parameterName(i), ast_function->parameterType(i), ast_function->scope());
				add_variable(ast_variable);
				switch (ast_function->parameterType(i - 1)) {
					case mathvm::VT_INT:
						bytecode->addInsn(mathvm::BC_STORECTXIVAR);
						break;
					case mathvm::VT_DOUBLE:
						bytecode->addInsn(mathvm::BC_STORECTXDVAR);
						break;
					case mathvm::VT_STRING :
						bytecode->addInsn(mathvm::BC_STORECTXSVAR);
						break;
					default :
						assert(false);
						break;
				}
				add_var_id_to_bytecode(ast_variable);
			}

			ast_function->node()->visitChildren(this);

			bytecode = outer_bytecode;
			return_type = outer_return_type;

			scopes.pop_back();
		}

		node->visitChildren(this);

		//scopes.pop_back();
	}

	virtual void visitFunctionNode(mathvm::FunctionNode* node) {
		node->visitChildren(this);
	}

	virtual void visitReturnNode(mathvm::ReturnNode* node) {
		if (node->returnExpr()) {
			node->returnExpr()->visit(this);
			match_type(return_type);
		}

		bytecode->addInsn(mathvm::BC_RETURN);
	}

	virtual void visitCallNode(mathvm::CallNode* node) {
		mathvm::TranslatedFunction* function = code->functionByName(node->name());

		for (int i = node->parametersNumber() - 1; i >= 0; --i) {
			node->parameterAt(i)->visit(this);
			if (top_type != function->signature()[i + 1].first) {
				match_type(function->signature()[i + 1].first);
			}
		}

		bytecode->addInsn(mathvm::BC_CALL);
		bytecode->addUInt16(function->id());

		top_type= function->signature()[0].first;
	}

	virtual void visitPrintNode(mathvm::PrintNode* node) {
		for (uint16_t i = 0; i < node->operands(); ++i) {
			node->operandAt(i)->visit(this);
			switch(top_type) {
			case mathvm::VT_INT:
				bytecode->addInsn(mathvm::BC_IPRINT);
				break;
			case mathvm::VT_STRING:
				bytecode->addInsn(mathvm::BC_SPRINT);
				break;
			case mathvm::VT_DOUBLE:
				bytecode->addInsn(mathvm::BC_DPRINT);
				break;
			default:
				assert(false);
				throw TranslationException("Can't print this type.");
				break;
			}
		}
	}
};

#endif // _VISITOR_HPP_
