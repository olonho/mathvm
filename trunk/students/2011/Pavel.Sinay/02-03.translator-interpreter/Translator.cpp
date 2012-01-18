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
	//m_bytecode.addInsn(BC_ILOAD);
	//m_bytecode.addInt64(10);
	//m_bytecode.addInsn(BC_IPRINT);

}

PSTranslator::~PSTranslator() {
	// TODO Auto-generated destructor stub
}

Status* PSTranslator::translate(const std::string& program, Code* *code) {
	mathvm::Parser parser;
	mathvm::Status* status = parser.parseProgram(program.c_str());
	if (status == NULL) {
		VisitorSourcePrinter printer(std::cout);
		std::cout << "--------------Printing--------------" << std::endl;
		parser.top()->node()->visit(&printer);
		std::cout << "--------------Translating--------------" << std::endl;
		try {
			parser.top()->node()->visit(this);

			//m_bytecode.dump(std::cout);
			//			ExecStack stack;
			//			//stack.pushDouble(12);
			//			stack.pushInt(123);
			//			uint_fast64_t i = stack.popInt();
			//			double d = stack.popDouble();
			//			// stack.popDouble();
			//			std::cout << i << " " << d << std::endl;
		} catch (MVException const& e) {
			std::cerr << "EXCEPTION: " << e.what() << std::endl;
		}
		*code = new PSCode();
		(*((PSCode**) code))->setByteCode(m_bytecode);
		return new Status;
	} else {
		if (status->isError()) {
			uint32_t position = status->getPosition();
			uint32_t line = 0, offset = 0;
			mathvm::positionToLineOffset(program.c_str(), position, line,
					offset);
			printf("Cannot translate expression: expression at %d,%d; "
				"error '%s'\n", line, offset, status->getError().c_str());
		}
		return status;
	}
}

void PSTranslator::visitIfNode(mathvm::IfNode *node) {
}

void PSTranslator::visitPrintNode(mathvm::PrintNode *node) {
	for (unsigned int i = 0; i < node->operands(); ++i) {
		node->operandAt(i)->visit(this);
		switch (m_last_result) {
		case RT_int:
			m_bytecode.addInsn(BC_IPRINT);
			break;
		case RT_double:
			m_bytecode.addInsn(BC_DPRINT);
			break;
		case RT_string:
			m_bytecode.addInsn(BC_SPRINT);
		}
	}
}

void PSTranslator::visitLoadNode(mathvm::LoadNode *node) {
}

void PSTranslator::visitForNode(mathvm::ForNode *node) {
}

void PSTranslator::visitUnaryOpNode(mathvm::UnaryOpNode *node) {
}

void PSTranslator::visitFunctionNode(mathvm::FunctionNode *node) {
	node->visitChildren(this);
	//node->body()->visit(this);
}

void PSTranslator::visitWhileNode(mathvm::WhileNode *node) {
}

void PSTranslator::visitBinaryOpNode(mathvm::BinaryOpNode *node) {
}

void PSTranslator::visitBlockNode(mathvm::BlockNode *node) {
	m_var_table.openPage();
	mathvm::Scope::VarIterator it(node->scope());
	while (it.hasNext()) {
		mathvm::AstVar *var = it.next();
		m_var_table.addVar(var->name());
	}

	mathvm::Scope::FunctionIterator f_it(node->scope());
	while (f_it.hasNext()) {
		f_it.next()->node()->visit(this);
	}

	node->visitChildren(this);
	m_var_table.dump();
	m_var_table.closePage();
}

void PSTranslator::visitIntLiteralNode(mathvm::IntLiteralNode *node) {
	m_bytecode.addInsn(BC_ILOAD);
	m_bytecode.addInt64(node->literal());
	m_last_result = RT_int;
}

void PSTranslator::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
	m_bytecode.addInsn(BC_SLOAD);
	m_bytecode.addInt16(node->literal().size());
	std::string::const_iterator it = node->literal().begin();
	for (; it != node->literal().end(); ++it) {
		m_bytecode.addByte(*it);
	}
	m_last_result = RT_string;
}

void PSTranslator::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
	m_bytecode.addInsn(BC_DLOAD);
	m_bytecode.addDouble(node->literal());
	m_last_result = RT_double;
}

void PSTranslator::visitStoreNode(mathvm::StoreNode *node) {
}

void PSTranslator::visitCallNode(mathvm::CallNode *node) {
}

void PSTranslator::visitReturnNode(mathvm::ReturnNode* node) {
}
