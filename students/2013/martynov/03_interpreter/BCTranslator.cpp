/*
 * BCTranslator.cpp
 *
 *  Created on: Dec 8, 2013
 *      Author: sam
 */

#include <cstdio>
#include <cstdlib>
#include <map>

#include "BCFunction.h"
#include "BCTranslator.h"
#include "BCTypes.h"

struct FunInfo {
	std::set<const mathvm::AstVar*> freeVars;
	std::set<std::pair<FunInfo*, mathvm::Scope*> > callees;
	mathvm::BCFunction* bcfun;
};

std::map<const mathvm::CallNode*, FunInfo*> callNodes;
std::map<const mathvm::AstFunction*, FunInfo> infos;
std::set<const mathvm::AstVar*> globals;

#include "BCLabel.h"
#include "FreeVarsVisitor.h"

namespace mathvm {

//BCTranslator::BCTranslator() {
//	// TODO Auto-generated constructor stub
//}

//BCTranslator::~BCTranslator() {
//	// TODO Auto-generated destructor stub
//}

Status* BCTranslator::translate(AstFunction* node) {
	try {
		FreeVarsVisitor v(node);
		BCFunction* main = infos[node].bcfun;
		prog->addFunction(main);
		ip = main->bytecode();
		node->node()->body()->visit(this);
		ip->add(BC_STOP);

		uint16_t i = 0;
		for (std::set<const AstVar*>::iterator it = globals.begin();
				it != globals.end(); ++it) {
			prog->globalVarsMap()[(*it)->name()] = i++;
		}
	} catch (ErrorInfoHolder* error) {
		return new Status(error->getMessage(), error->getPosition());
	}
	return 0;
}

void BCTranslator::visitBinaryOpNode(BinaryOpNode* node) {
	if (node->kind() == tOR || node->kind() == tAND) {
		checkTypeInt(node->left());
		ip->add(BC_ILOAD0);
		BCLabel label1(ip);
		{
			BCLabel label(ip);
			ip->addBranch(node->kind() == tOR ? BC_IFICMPE : BC_IFICMPNE,
					label());
			ip->add(node->kind() == tOR ? BC_ILOAD1 : BC_ILOAD0);
			ip->addBranch(BC_JA, label1());
		}
		node->right()->visit(this);
		if (cType != VT_INT) {
			std::cerr << "Expression of type int is expected, but "
					<< typeToName(cType) << " is given" << std::endl;
		}
		return;
	}

	node->right()->visit(this);
	VarType right = cType;
	cType = VT_INVALID;
	node->left()->visit(this);
	VarType left = cType;
	switch (node->kind()) {
	case tEQ:
	case tNEQ:
	case tGT:
	case tGE:
	case tLT:
	case tLE:
	case tADD:
	case tSUB:
	case tMUL:
	case tDIV:
		if (left != VT_INT && left != VT_DOUBLE) {
			std::cerr << "Expression of type int or double is expected, but "
					<< typeToName(left) << " is given" << std::endl;
		}
		if (right != VT_INT && right != VT_DOUBLE) {
			std::cerr << "Expression of type int or double is expected, but "
					<< typeToName(right) << " is given" << std::endl;
		}
		if (left != right) {
			if (left == VT_INT) {
				cType = VT_DOUBLE;
				ip->add(BC_I2D);
			} else {
				ip->add(BC_SWAP);
				ip->add(BC_I2D);
				if (node->kind() != tADD && node->kind() != tMUL) {
					ip->add(BC_SWAP);
				}
			}
		}
		break;
	default:
		std::cerr << "Error: unexpected token " << tokenOp(node->kind())
				<< std::cerr;
		break;
	}
	if (cType == VT_DOUBLE) {
		switch (node->kind()) {
		case tEQ:
		case tNEQ:
		case tGT:
		case tGE:
		case tLT:
		case tLE:
			ip->add(BC_DCMP);
			cType = VT_INT;
			break;
		default:
			break;
		}
		switch (node->kind()) {
		case tNEQ:
			ip->add(BC_ILOAD0);
			triple(BC_IFICMPNE);
			break;
		case tEQ:
			ip->add(BC_ILOAD0);
			triple(BC_IFICMPE);
			break;
		case tGT:
			ip->add(BC_ILOADM1);
			triple(BC_IFICMPE);
			break;
		case tGE:
			ip->add(BC_ILOAD1);
			triple(BC_IFICMPNE);
			break;
		case tLT:
			ip->add(BC_ILOAD1);
			triple(BC_IFICMPE);
			break;
		case tLE:
			ip->add(BC_ILOADM1);
			triple(BC_IFICMPNE);
			break;
		case tADD:
			ip->add(BC_DADD);
			break;
		case tSUB:
			ip->add(BC_DSUB);
			break;
		case tMUL:
			ip->add(BC_DMUL);
			break;
		case tDIV:
			ip->add(BC_DDIV);
			break;
		default:
			break;
		}
	} else
		switch (node->kind()) {
		case tEQ:
			triple(BC_IFICMPE);
			break;
		case tNEQ:
			triple(BC_IFICMPNE);
			break;
		case tGT:
			triple(BC_IFICMPG);
			break;
		case tGE:
			triple(BC_IFICMPGE);
			break;
		case tLT:
			triple(BC_IFICMPL);
			break;
		case tLE:
			triple(BC_IFICMPLE);
			break;
		case tADD:
			ip->add(BC_IADD);
			break;
		case tSUB:
			ip->add(BC_ISUB);
			break;
		case tMUL:
			ip->add(BC_IMUL);
			break;
		case tDIV:
			ip->add(BC_IDIV);
			break;
		default:
			break;
		}
}

void BCTranslator::visitUnaryOpNode(UnaryOpNode* node) {
	node->operand()->visit(this);
	switch (node->kind()) {
	case tSUB:
		switch (cType) {
		case VT_INT:
			ip->add(BC_INEG);
			break;
		case VT_DOUBLE:
			ip->add(BC_DNEG);
			break;
		default:
			std::cerr << "Expression of type int or double is expected, but "
					<< typeToName(cType) << " is given" << std::endl;
			break;
		}
		break;
	case tNOT: {
		if (cType != VT_INT) {
			std::cerr << "Expression of type int is expected, but "
					<< typeToName(cType) << " is given" << std::endl;
		}
		ip->add(BC_ILOAD0);
		triple(BC_IFICMPE);
		break;
	}
	default:
		std::cerr << "Internal Error: unknown operator" << std::endl;
		break;
	}
}

void BCTranslator::visitStringLiteralNode(StringLiteralNode* node) {
	ip->add(BC_SLOAD);
	ip->addInt16(prog->makeStringConstant(node->literal()));
	cType = VT_STRING;
}

void BCTranslator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
	if (node->literal() == -1) {
		ip->add(BC_DLOADM1);
	} else if (node->literal() == 0) {
		ip->add(BC_DLOAD0);
	} else if (node->literal() == 1) {
		ip->add(BC_DLOAD1);
	} else {
		ip->add(BC_DLOAD);
		if (sizeof(double) == 8) {
			double l = node->literal();
			put(&l, sizeof(l));
		} else if (sizeof(long double) == 8) {
			long double l = node->literal();
			put(&l, sizeof(l));
		} else if (sizeof(float) == 8) {
			float l = node->literal();
			put(&l, sizeof(l));
		} else
			std::cerr << "Error: " << std::endl;
	}
	cType = VT_DOUBLE;
}

void BCTranslator::visitIntLiteralNode(IntLiteralNode* node) {
	int64_t l = node->literal();
	switch (l) {
	case -1:
		ip->add(mathvm::BC_ILOADM1);
		break;
	case 0:
		ip->add(mathvm::BC_ILOAD0);
		break;
	case 1:
		ip->add(mathvm::BC_ILOAD1);
		break;
	default:
		ip->add(mathvm::BC_ILOAD);
		put(&l, sizeof(l));
		break;
	}
	cType = VT_INT;
}

void BCTranslator::visitLoadNode(LoadNode* node) {
	putVar(BC_LOADIVAR, node->var(), node);
	cType = node->var()->type();
}

void BCTranslator::visitStoreNode(StoreNode* node) {
	AstNode* right = node->value();
	LoadNode l(0, node->var());
	BinaryOpNode tnode(0, node->op() == tINCRSET ? tADD : tSUB, &l, right);
	switch (node->op()) {
	case tASSIGN:
		break;
	case tINCRSET:
	case tDECRSET:
		right = &tnode;
		break;
	default:
		std::cerr << "Internal Error: unknown operator" << std::endl;
		break;
	}
	right->visit(this);
	if (node->var()->type() != cType) {
		std::cerr << "Expression of type " << typeToName(node->var()->type())
				<< " is expected, but " << typeToName(cType) << " is given"
				<< std::endl;
	}
	putVar(BC_STOREIVAR, node->var(), node);
	cType = VT_INVALID;
}

void BCTranslator::visitForNode(ForNode* node) {
	if (node->var()->type() != VT_INT) {
		std::cerr << "Expression of type int is expected, but "
				<< typeToName(node->var()->type()) << " is given" << std::endl;
	}
	BinaryOpNode* in = dynamic_cast<BinaryOpNode*>(node->inExpr());
	if (!in || in->kind() != tRANGE) {
		node->inExpr()->visit(this);
		std::cerr << "Expression of type range is expected, but "
				<< typeToName(cType) << " is given" << std::endl;
	}
	checkTypeInt(in->left());
	putVar(BC_STOREIVAR, node->var(), node);
	BCLabel start(ip);
	BCLabel end(ip);
	ip->bind(start());
	checkTypeInt(in->right());
	putVar(BC_LOADIVAR, node->var(), node);
	ip->addBranch(BC_IFICMPG, end());
	node->body()->visit(this);
	cType = VT_INVALID;
	putVar(BC_LOADIVAR, node->var(), node);
	ip->add(BC_ILOAD1);
	ip->add(BC_IADD);
	putVar(BC_STOREIVAR, node->var(), node);
	ip->addBranch(BC_JA, start());
}

void BCTranslator::visitWhileNode(WhileNode* node) {
	BCLabel start(ip);
	BCLabel end(ip);
	ip->bind(start());
	IntLiteralNode* in = dynamic_cast<IntLiteralNode*>(node->whileExpr());
	if (in) {
		if (in->literal() == 0) {
			return;
		}
	} else {
		checkTypeInt(node->whileExpr());
		ip->add(BC_ILOAD0);
		ip->addBranch(BC_IFICMPE, end());
	}
	node->loopBlock()->visit(this);
	cType = VT_INVALID;
	ip->addBranch(BC_JA, start());
}

void BCTranslator::visitIfNode(IfNode* node) {
	checkTypeInt(node->ifExpr());
	ip->add(BC_ILOAD0);
	BCLabel label(ip);
	ip->addBranch(BC_IFICMPE, label());
	node->thenBlock()->visit(this);
	cType = VT_INVALID;
	if (node->elseBlock()) {
		BCLabel label1(ip);
		ip->addBranch(BC_JA, label1());
		ip->bind(label());
		node->elseBlock()->visit(this);
		cType = VT_INVALID;
	}
}

void BCTranslator::visitBlockNode(BlockNode* node) {
	for (Scope::VarIterator it(node->scope()); it.hasNext();) {
		addVar(node, it.next()->name());
	}
	for (Scope::FunctionIterator it(node->scope()); it.hasNext();) {
		prog->addFunction(infos[it.next()].bcfun);
	}
	for (Scope::FunctionIterator it(node->scope()); it.hasNext();) {
		AstFunction* fun = it.next();
		FunInfo* info = &infos[fun];
		BCTranslator trans(prog);
		trans.ip = info->bcfun->bytecode();
		trans.tType = fun->returnType();
		trans.currentVar = 0;
		if (trans.tType != VT_VOID) {
			++trans.currentVar;
		}
		for (std::set<const AstVar*>::iterator it = globals.begin();
				it != globals.end(); ++it) {
			trans.vars[(*it)->name()].push_back(vars[(*it)->name()].front());
		}
		for (std::set<const AstVar*>::iterator it = info->freeVars.begin();
				it != info->freeVars.end(); ++it) {
			trans.addVar(node, (*it)->name());
		}
		for (uint32_t i = 0; i < fun->parametersNumber(); ++i) {
			trans.addVar(node, fun->parameterName(i));
		}
		fun->node()->body()->visit(&trans);
	}
	for (uint32_t i = 0; i < node->nodes(); ++i) {
		CallNode* callNode = dynamic_cast<CallNode*>(node->nodeAt(i));
		node->nodeAt(i)->visit(this);
		if (callNode && callNodes[callNode]->bcfun->returnType() != VT_VOID) {
			ip->add(BC_POP);
		}
	}
	for (Scope::VarIterator it(node->scope()); it.hasNext();) {
		delVar(it.next()->name());
	}
}

void BCTranslator::visitFunctionNode(FunctionNode* node) {
	std::cerr << "Error" << std::endl;
}

void BCTranslator::visitReturnNode(ReturnNode* node) {
	if (node->returnExpr()) {
		if (tType == VT_VOID) {
			std::cerr << "Error: returning value is expected" << std::endl;
		}
		node->returnExpr()->visit(this);
		if (tType != cType) {
			if (cType == VT_INT && tType == VT_DOUBLE) {
				ip->add(BC_I2D);
			} else {
				std::cerr << "Expression of type " << typeToName(tType)
						<< " is expected, but " << typeToName(cType)
						<< " is given" << std::endl;
			}
		}
	} else {
		if (tType != VT_VOID) {
			std::cerr << "Error: returning value is unexpected" << std::endl;
		}
	}
	cType = VT_INVALID;
	if (tType != VT_VOID) {
		ip->add(BC_STOREIVAR0);
	}
	ip->add(BC_RETURN);
}

void BCTranslator::visitCallNode(CallNode* node) {
	FunInfo* info = callNodes[node];
	if (info->bcfun->parametersNumber() != node->parametersNumber()) {
		std::cerr << "Function expects " << info->bcfun->parametersNumber()
				<< " arguments but got " << node->parametersNumber()
				<< std::endl;
	}
	if (info->bcfun->returnType() != VT_VOID) {
		ip->add(BC_ILOAD0);
	}
	for (std::set<const AstVar*>::iterator it = info->freeVars.begin();
			it != info->freeVars.end(); ++it) {
		putVar(BC_LOADIVAR, *it);
	}
	for (uint32_t i = 0; i < info->bcfun->parametersNumber(); ++i) {
		node->parameterAt(i)->visit(this);
		if (cType != info->bcfun->parameterType(i)) {
			if (cType == VT_INT
					&& info->bcfun->parameterType(i) == VT_DOUBLE) {
				ip->add(BC_I2D);
			} else {
				std::cerr << "Expression of type "
						<< typeToName(info->bcfun->parameterType(i))
						<< " is expected, but " << typeToName(cType)
						<< " is given" << std::endl;
			}
		}
	}
	ip->add(BC_CALL);
	ip->addTyped(info->bcfun->id());
	cType = info->bcfun->returnType();
	for (std::set<const AstVar*>::reverse_iterator it = info->freeVars.rbegin();
			it != info->freeVars.rend(); ++it) {
		putVar(BC_STOREIVAR, *it);
	}
}

void BCTranslator::visitNativeCallNode(NativeCallNode* node) {
	std::cerr << "Internal Error: unexpected code" << std::endl;
}

void BCTranslator::visitPrintNode(PrintNode* node) {
	for (uint32_t i = 0; i < node->operands(); ++i) {
		node->operandAt(i)->visit(this);
		switch (cType) {
		case VT_INT:
			ip->add(BC_IPRINT);
			break;
		case VT_DOUBLE:
			ip->add(BC_DPRINT);
			break;
		case VT_STRING:
			ip->add(BC_SPRINT);
			break;
		case VT_VOID:
			std::cerr
					<< "Expression of type int or double or string is expected, but "
					<< typeToName(VT_VOID) << " is given" << std::endl;
			break;
		default:
			std::cerr << "Internal Error: unexpected type" << std::endl;
			break;
		}
		cType = VT_INVALID;
	}
	cType = VT_VOID;
}

void BCTranslator::addVar(AstNode* node, const std::string& name) {
	if (overflow) {
		std::cerr << "Stack overflow" << std::cerr;
	}
	vars[name].push_back(currentVar++);
	if (!currentVar) {
		overflow = true;
	}
}

void BCTranslator::delVar(const std::string& name) {
	vars[name].pop_back();
	overflow = false;
	--currentVar;
}

void BCTranslator::put(const void* vbuf, unsigned int size) {
	const uint8_t* buf = (const uint8_t*) vbuf;
	while (size--) {
		ip->add(*buf++);
	}
}

void BCTranslator::putVar(Instruction ins, const AstVar* var, AstNode* node) {
	const std::vector<VarInt>& v = vars[var->name()];
	if (v.size() == 1 && globals.count(var)) {
		switch (ins) {
		case mathvm::BC_LOADIVAR:
			ip->add(mathvm::BC_LOADDVAR);
			break;
		case mathvm::BC_STOREIVAR:
			ip->add(mathvm::BC_STOREDVAR);
			break;
		default:
			std::cerr << "Internal Error: unexpected symbol" << std::endl;
			break;
		}
	} else {
		ip->add(ins);
	}
	put(&v.back(), sizeof(VarInt));
}

void BCTranslator::checkTypeInt(AstNode* expr) {
	expr->visit(this);
	if (cType != VT_INT) {
		std::cerr << "Expression of type int is expected, but "
				<< typeToName(cType) << " is given" << std::endl;
	}
	cType = VT_INVALID;
}

void BCTranslator::triple(Instruction i) {
	BCLabel label1(ip);
	{
		BCLabel label(ip);
		ip->addBranch(i, label());
		ip->add(BC_ILOAD0);
		ip->addBranch(BC_JA, label1());
	}
	ip->add(BC_ILOAD1);
}

} /* namespace mathvm */
