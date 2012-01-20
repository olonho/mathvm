/*
 * Translator_.cpp
 *
 *  Created on: 17.01.2012
 *      Author: Pavel Sinay
 */

#include "ExecStack.h"
#include "MVException.h"
#include "Translator.h"
#include "VisitorSourcePrinter.h"
#include "Code.h"
#include "../../../../include/mathvm.h"
#include "parser.h"
#include <stdio.h>
using namespace mathvm;

PSTranslator::PSTranslator() {
}

PSTranslator::~PSTranslator() {
}

Status* PSTranslator::translate(const std::string& program, Code* *code) {
	mathvm::Parser parser;
	mathvm::Status* status = parser.parseProgram(program.c_str());
	if (status == NULL) {
		VisitorSourcePrinter printer(std::cerr);
		std::cerr << "--------------Printing--------------" << std::endl;
		parser.top()->node()->visit(&printer);
		std::cerr << "--------------Translating--------------" << std::endl;

		*code = new PSCode();
		m_code = *code;
		try {
			parser.top()->node()->visit(this);
			m_bytecode.dump(std::cerr);
		} catch (MVException const& e) {
			return new Status(e.what(), e.getPosition());
		}

		(*((PSCode**) code))->setByteCode(m_bytecode);
		return new Status;
	} else {
		return status;
	}
}

void PSTranslator::visitFunctionNode(mathvm::FunctionNode *node) {
	m_var_table.openPage();
	m_func_table.openPage();
	if (node->name() != "<top>") {
		Label function_begin(&m_bytecode);
		Label function_end(&m_bytecode);

		m_bytecode.addBranch(BC_JA, function_end);
		m_bytecode.bind(function_begin);

		m_func_table.addFunc(
				FuncInfo(node->name(), node->returnType(), function_begin.bci()));

		for (uint32_t i = 0; i != node->parametersNumber(); ++i) {
			Var var(node->parameterType(i), node->parameterName(i));
			m_var_table.addVar(var);
			allocVar(var);
			switch (var.type()) {
			case VT_INT:
				m_bytecode.addInsn(BC_STOREIVAR);
				break;
			case VT_DOUBLE:
				m_bytecode.addInsn(BC_STOREDVAR);
				break;
			case VT_STRING:
				m_bytecode.addInsn(BC_STORESVAR);
				break;
			default:
				throw MVException("Invalid variable type", node->position());
			}
			m_bytecode.addInt16(m_var_table.getVarAddr(var.name()));
		}

		node->body()->visit(this);
//		if (node->returnType() == VT_VOID) {
//			m_bytecode.addInsn(BC_RETURN);
//		}
		m_bytecode.bind(function_end);
	} else {
		node->body()->visit(this);
	}
	//m_func_table.dump();
	m_var_table.closePage();
	m_func_table.closePage();
	m_last_result = VT_VOID;
}

void PSTranslator::visitBlockNode(mathvm::BlockNode *node) {
	mathvm::Scope::VarIterator it(node->scope());
	while (it.hasNext()) {
		mathvm::AstVar *var_it = it.next();
		Var var(var_it->type(), var_it->name());
		m_var_table.addVar(var);
		allocVar(var);
	}

	mathvm::Scope::FunctionIterator f_it(node->scope());
	while (f_it.hasNext()) {
		f_it.next()->node()->visit(this);
	}

	node->visitChildren(this);
	m_last_result = VT_VOID;
}

void PSTranslator::visitCallNode(mathvm::CallNode *node) {
	//m_bytecode.addInsn(BC_CALLNATIVE);
	FuncInfo func = m_func_table.getFuncByName(node->name());

	for (int i = node->parametersNumber() - 1; i >= 0; --i) {
		node->parameterAt(i)->visit(this);
	}

	m_bytecode.addInsn(BC_CALL);
	m_bytecode.addInt16(func.addr);
	m_last_result = func.result_type;
}

void PSTranslator::visitReturnNode(mathvm::ReturnNode* node) {
	//std::cerr << " >>> return node" << std::endl;

	node->visitChildren(this);
	m_bytecode.addInsn(BC_RETURN);
}

void PSTranslator::visitIfNode(mathvm::IfNode *node) {
	Label label_then(&m_bytecode);
	Label label_else(&m_bytecode);
	Label label_end(&m_bytecode);
	node->ifExpr()->visit(this);

	if (m_last_result != VT_INT) {
		throw MVException("logical statement is not int", node->position());
	}
	m_bytecode.addInsn(BC_ILOAD0);
	m_bytecode.addBranch(BC_IFICMPNE, label_then);
	if (node->elseBlock()) {
		node->elseBlock()->visitChildren(this);
		m_bytecode.addBranch(BC_JA, label_end);
	} else {
		m_bytecode.addBranch(BC_JA, label_end);
	}
	m_bytecode.bind(label_then);
	node->thenBlock()->visitChildren(this);
	m_bytecode.bind(label_end);

	m_last_result = VT_VOID;
}

void PSTranslator::visitPrintNode(mathvm::PrintNode *node) {
	for (unsigned int i = 0; i < node->operands(); ++i) {
		node->operandAt(i)->visit(this);
		switch (m_last_result) {
		case VT_INT:
			m_bytecode.addInsn(BC_IPRINT);
			break;
		case VT_DOUBLE:
			m_bytecode.addInsn(BC_DPRINT);
			break;
		case VT_STRING:
			m_bytecode.addInsn(BC_SPRINT);
			break;
		default:
			throw MVException("Invalid variable type", node->position());
		}
	}
	m_last_result = VT_VOID;
}

void PSTranslator::visitLoadNode(mathvm::LoadNode *node) {
	switch (node->var()->type()) {
	case VT_INT:
		m_bytecode.addInsn(BC_LOADIVAR);
		m_last_result = VT_INT;
		break;
	case VT_DOUBLE:
		m_bytecode.addInsn(BC_LOADDVAR);
		m_last_result = VT_DOUBLE;
		break;
	case VT_STRING:
		m_bytecode.addInsn(BC_LOADSVAR);
		m_last_result = VT_STRING;
		break;
	default:
		throw MVException("Invalid variable type", node->position());
	}

	m_bytecode.addInt16(m_var_table.getVarAddr(node->var()->name()));
}

void PSTranslator::visitForNode(mathvm::ForNode *node) {
	if (node->var()->type() != VT_INT) {
		throw MVException("For variable must be integer");
	}

	Label begin(&m_bytecode);
	Label end(&m_bytecode);
	uint16_t var_addr = m_var_table.getVarAddr(node->var()->name());

	node->inExpr()->visit(this);
	m_bytecode.addInsn(BC_STOREIVAR);
	m_bytecode.addInt16(var_addr);
	m_bytecode.addInsn(BC_POP);

	m_bytecode.bind(begin);
	node->inExpr()->visit(this);
	m_bytecode.addInsn(BC_POP);
	m_bytecode.addInsn(BC_LOADIVAR);
	m_bytecode.addInt16(var_addr);
	m_bytecode.addBranch(BC_IFICMPG, end);
	node->body()->visit(this);

	m_bytecode.addInsn(BC_LOADIVAR);
	m_bytecode.addInt16(var_addr);
	m_bytecode.addInsn(BC_ILOAD1);
	m_bytecode.addInsn(BC_IADD);
	m_bytecode.addInsn(BC_STOREIVAR);
	m_bytecode.addInt16(var_addr);
	m_bytecode.addBranch(BC_JA, begin);
	m_bytecode.bind(end);

	m_last_result = VT_VOID;
}

void PSTranslator::visitWhileNode(mathvm::WhileNode *node) {
	Label begin(&m_bytecode);
	Label end(&m_bytecode);

	m_bytecode.bind(begin);
	node->whileExpr()->visit(this);
	//TODO: cast
	if (m_last_result != VT_INT) {
		throw MVException("While expression must be integer or logical",
				node->position());
	}

	m_bytecode.addInsn(BC_ILOAD0);
	m_bytecode.addBranch(BC_IFICMPE, end);

	node->loopBlock()->visit(this);
	m_bytecode.addBranch(BC_JA, begin);
	m_bytecode.bind(end);

	m_last_result = VT_VOID;
}

void PSTranslator::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
	node->visitChildren(this);

	//TODO: cast
	switch (node->kind()) {

	case tNOT: {
		Label set0(&m_bytecode);
		Label end(&m_bytecode);

		m_bytecode.addInsn(BC_ILOAD0);
		m_bytecode.addBranch(BC_IFICMPNE, set0);
		m_bytecode.addInsn(BC_ILOAD1);
		m_bytecode.addBranch(BC_JA, end);
		m_bytecode.bind(set0);
		m_bytecode.addInsn(BC_ILOAD0);
		m_bytecode.bind(end);
		m_last_result = VT_INT;
		break;
	}
	case tSUB: {
		switch (m_last_result) {
		case VT_INT: {
			m_bytecode.addInsn(BC_INEG);
			m_last_result = VT_INT;
			break;
		}
		case VT_DOUBLE: {
			m_bytecode.addInsn(BC_DNEG);
			m_last_result = VT_DOUBLE;
			break;
		}
		case VT_STRING: {
			throw MVException("No operations with strings!", node->position());
			default:
			throw MVException("Invalid variable type", node->position());
		}
		}
	}
		break;
	default: {
		throw MVException("Invalid unary operation", node->position());
	}
	}
}

void PSTranslator::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
	node->right()->visit(this);
	VarType right_var_type = m_last_result;
	node->left()->visit(this);
	VarType left_var_type = m_last_result;

	const TokenKind op = node->kind();
	if (right_var_type == VT_STRING || left_var_type == VT_STRING) {
		throw MVException("No operations with strings!", node->position());
	} else if (right_var_type != left_var_type && (op == tADD || op == tSUB
			|| op == tMUL || op == tDIV)) {
		//cast, cast...
		if (right_var_type == VT_DOUBLE) {
			castIntToDouble();
			left_var_type = m_last_result;
		} else {
			castDoubleToInt();
			left_var_type = m_last_result;
		}
	}

	if (op == tADD || op == tSUB || op == tMUL || op == tDIV || op == tMOD) {
		switch (op) {
		case tADD:
			if (m_last_result == VT_INT) {
				m_bytecode.addInsn(BC_IADD);
			} else {
				m_bytecode.addInsn(BC_DADD);
			}
			break;
		case tSUB:
			if (m_last_result == VT_INT) {
				m_bytecode.addInsn(BC_ISUB);
			} else {
				m_bytecode.addInsn(BC_DSUB);
			}
			break;
		case tMUL:
			if (m_last_result == VT_INT) {
				m_bytecode.addInsn(BC_IMUL);
			} else {
				m_bytecode.addInsn(BC_DMUL);
			}
			break;
		case tDIV:
			if (m_last_result == VT_INT) {
				m_bytecode.addInsn(BC_IDIV);
			} else {
				m_bytecode.addInsn(BC_DDIV);
			}
			break;
		case tMOD:
			m_bytecode.addInsn(BC_IMOD);
			break;

		default:
			std::cerr << "busy doing nothing" << std::endl;
		}
	} else if (op == tEQ || op == tNEQ || op == tGT || op == tGE || op == tLT
			|| op == tLE) {
		Label set1(&m_bytecode);
		Label end(&m_bytecode);

		switch (op) {
		case tEQ:
			m_bytecode.addBranch(BC_IFICMPE, set1);
			break;

		case tNEQ:
			m_bytecode.addBranch(BC_IFICMPNE, set1);
			break;

		case tGT:
			m_bytecode.addBranch(BC_IFICMPG, set1);
			break;

		case tGE:
			m_bytecode.addBranch(BC_IFICMPGE, set1);
			break;

		case tLT:
			m_bytecode.addBranch(BC_IFICMPL, set1);
			break;

		case tLE:
			m_bytecode.addBranch(BC_IFICMPLE, set1);
			break;

		default:
			std::cerr << "busy doing nothing" << std::endl;
		}
		m_bytecode.addInsn(BC_ILOAD0);
		m_bytecode.addBranch(BC_JA, end);
		m_bytecode.bind(set1);
		m_bytecode.addInsn(BC_ILOAD1);
		m_bytecode.bind(end);
	} else if (op == tOR || op == tAND) {
		switch (op) {
		case tOR:
			m_bytecode.addInsn(BC_IADD);
			break;

		case tAND:
			m_bytecode.addInsn(BC_IMUL);
			break;

		default:
			std::cerr << "busy doing nothing" << std::endl;
		}
		Label set1(&m_bytecode);
		Label end(&m_bytecode);
		m_bytecode.addInsn(BC_ILOAD0);
		m_bytecode.addBranch(BC_IFICMPNE, set1);
		m_bytecode.addInsn(BC_ILOAD0);
		m_bytecode.addBranch(BC_JA, end);
		m_bytecode.bind(set1);
		m_bytecode.addInsn(BC_ILOAD1);
		m_bytecode.bind(end);
	} else if (op == tRANGE) {
		//do nothing?...
	}
}

void PSTranslator::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
	m_bytecode.addInsn(BC_ILOAD);
	m_bytecode.addInt64(node->literal());
	m_last_result = VT_INT;
}

void PSTranslator::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
	m_bytecode.addInsn(BC_SLOAD);
	m_bytecode.addInt16(m_code->makeStringConstant(node->literal()));
	m_last_result = VT_STRING;
}

void PSTranslator::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
	m_bytecode.addInsn(BC_DLOAD);
	m_bytecode.addDouble(node->literal());
	m_last_result = VT_DOUBLE;
}

void PSTranslator::visitStoreNode(mathvm::StoreNode *node) {
	node->value()->visit(this);
	uint16_t addr = m_var_table.getVarAddr(node->var()->name());
	const TokenKind op = node->op();
	switch (op) {
	case tASSIGN: {
		switch (node->var()->type()) {
		case VT_INT:
			m_bytecode.addInsn(BC_STOREIVAR);
			m_last_result = VT_INT;
			break;
		case VT_DOUBLE:
			m_bytecode.addInsn(BC_STOREDVAR);
			m_last_result = VT_DOUBLE;
			break;
		case VT_STRING:
			m_bytecode.addInsn(BC_STORESVAR);
			m_last_result = VT_STRING;
			break;
		default:
			throw MVException("Invalid result type", node->position());
		}
		break;
	}
	case tINCRSET:
		switch (node->var()->type()) {
		case VT_INT:
			m_bytecode.addInsn(BC_LOADIVAR);
			m_bytecode.addInt16(addr);
			m_bytecode.addInsn(BC_IADD);
			m_bytecode.addInsn(BC_STOREIVAR);
			m_last_result = VT_INT;
			break;
		case VT_DOUBLE:
			m_bytecode.addInsn(BC_LOADDVAR);
			m_bytecode.addInt16(addr);
			m_bytecode.addInsn(BC_DADD);
			m_bytecode.addInsn(BC_STOREDVAR);
			m_last_result = VT_DOUBLE;
			break;
		default:
			throw MVException("Invalid variable type", node->position());
		}
		break;

	case tDECRSET:
		switch (node->var()->type()) {
		case VT_INT:
			m_bytecode.addInsn(BC_LOADIVAR);
			m_bytecode.addInt16(addr);
			m_bytecode.addInsn(BC_ISUB);
			m_bytecode.addInsn(BC_STOREIVAR);
			m_last_result = VT_INT;
			break;
		case VT_DOUBLE:
			m_bytecode.addInsn(BC_LOADDVAR);
			m_bytecode.addInt16(addr);
			m_bytecode.addInsn(BC_DSUB);
			m_bytecode.addInsn(BC_STOREDVAR);
			m_last_result = VT_DOUBLE;
			break;
		default:
			throw MVException("Invalid variable type", node->position());
		}
		break;
	default:
		throw MVException("Invalid operation", node->position());
	}
	m_bytecode.addInt16(addr);
}

void PSTranslator::castIntToDouble() {
	m_bytecode.add(BC_I2D);
	m_last_result = VT_DOUBLE;
}

void PSTranslator::castDoubleToInt() {
	m_bytecode.add(BC_D2I);
	m_last_result = VT_INT;
}

void PSTranslator::allocVar(mathvm::Var &var) {
	switch (var.type()) {
	case VT_INT:
		m_bytecode.addInsn(BC_STORECTXIVAR);
		m_bytecode.addInt16(m_var_table.getVarAddr(var.name()));
		m_bytecode.addInt16(0);
		break;
	case VT_DOUBLE:
		m_bytecode.addInsn(BC_STORECTXDVAR);
		m_bytecode.addInt16(m_var_table.getVarAddr(var.name()));
		m_bytecode.addInt16(0);
		break;
	case VT_STRING:
		m_bytecode.addInsn(BC_STORECTXSVAR);
		m_bytecode.addInt16(m_var_table.getVarAddr(var.name()));
		m_bytecode.addInt16(0);
		break;
	default:
		throw MVException("Invalid variable type");
	}
}
