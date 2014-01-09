/*
 * FreeVarsVisitor.h
 *
 *  Created on: Jan 9, 2014
 *      Author: sam
 */

#ifndef FREEVARSVISITOR_H_
#define FREEVARSVISITOR_H_

namespace mathvm {

class FreeVarsVisitor: public AstVisitor {
public:
	FreeVarsVisitor(AstFunction* v) :
			global(true), functions(new std::map<std::string, FunInfo*>()) {
		init(v);
		for (bool repeat = true; repeat;) {
			repeat = false;
			for (std::map<const AstFunction*, FunInfo>::iterator it =
					infos.begin(); it != infos.end(); ++it) {
				size_t size = it->second.freeVars.size();
				for (std::set<std::pair<FunInfo*, Scope*> >::iterator it1 =
						it->second.callees.begin();
						it1 != it->second.callees.end(); ++it1) {
					for (std::set<const AstVar*>::iterator it2 =
							it1->first->freeVars.begin();
							it2 != it1->first->freeVars.end(); ++it2) {
						scope = it1->second;
						funScope = it->first->node()->body()->scope()->parent();
						if (!lookupVariable(*it2)) {
							it->second.freeVars.insert(*it2);
						}
					}
				}
				if (it->second.freeVars.size() != size) {
					repeat = true;
				}
			}
		}
		for (std::map<const AstFunction*, FunInfo>::iterator it = infos.begin();
				it != infos.end(); ++it) {
			it->second.bcfun->setFreeVarsNumber(it->second.freeVars.size());
		}
	}
	~FreeVarsVisitor() {
		if (global)
			delete functions;
	}
	void visitBinaryOpNode(BinaryOpNode* node) {
		node->visitChildren(this);
	}
	void visitUnaryOpNode(UnaryOpNode* node) {
		node->visitChildren(this);
	}
	void visitReturnNode(ReturnNode* node) {
		node->visitChildren(this);
	}
	void visitWhileNode(WhileNode* node) {
		node->visitChildren(this);
	}
	void visitPrintNode(PrintNode* node) {
		node->visitChildren(this);
	}
	void visitLoadNode(LoadNode* node) {
		addFreeVar(node);
	}
	void visitCallNode(CallNode* node) {
		node->visitChildren(this);
		std::map<std::string, FunInfo*>::iterator it = (*functions).find(
				node->name());
		if (it == functions->end()) {
			std::cerr << "Error: undeclared function " << node->name().c_str() << std::endl;
		}
		info->callees.insert(std::make_pair(it->second, scope));
		callNodes[node] = it->second;
	}
	void visitIfNode(IfNode* node) {
		node->ifExpr()->visit(this);
		uint32_t loc1 = info->bcfun->localsNumber();
		node->thenBlock()->visit(this);
		if (node->elseBlock()) {
			uint32_t loc2 = info->bcfun->localsNumber();
			info->bcfun->setLocalsNumber(loc1);
			node->elseBlock()->visit(this);
			if (loc2 > info->bcfun->localsNumber()) {
				info->bcfun->setLocalsNumber(loc2);
			}
		}
	}
	void visitStoreNode(StoreNode* node) {
		node->visitChildren(this);
		addFreeVar(node);
	}
	void visitForNode(ForNode* node) {
		node->visitChildren(this);
		addFreeVar(node);
	}
	void visitBlockNode(BlockNode* node) {
		Scope* scope1 = scope;
		scope = node->scope();
		std::vector<std::map<std::string, FunInfo*>::iterator> f;
		for (Scope::VarIterator it(node->scope()); it.hasNext();) {
			info->bcfun->setLocalsNumber(info->bcfun->localsNumber() + 1);
			AstVar* var = it.next();
			if (global) {
				globals.insert(var);
			}
		}
		global = false;
		uint32_t loc1 = info->bcfun->localsNumber(), loc_max = 0;
		for (Scope::FunctionIterator it(node->scope()); it.hasNext();) {
			AstFunction* fun = it.next();
			std::pair<std::map<std::string, FunInfo*>::iterator, bool> p =
					functions->insert(make_pair(fun->name(), &infos[fun]));
			if (p.second) {
				f.push_back(p.first);
			}
		}
		for (Scope::FunctionIterator it(node->scope()); it.hasNext();) {
			FreeVarsVisitor fv(it.next(), functions);
			for (std::set<const AstVar*>::iterator it1 =
					fv.info->freeVars.begin(); it1 != fv.info->freeVars.end();
					++it1) {
				if (!lookupVariable(node->scope(), *it1)) {
					info->freeVars.insert(*it1);
				}
			}
			info->callees.insert(fv.info->callees.begin(),
					fv.info->callees.end());
		}
		for (uint32_t i = 0; i < node->nodes(); ++i) {
			info->bcfun->setLocalsNumber(0);
			node->nodeAt(i)->visit(this);
			if (info->bcfun->localsNumber() > loc_max) {
				loc_max = info->bcfun->localsNumber();
			}
		}
		info->bcfun->setLocalsNumber(loc1 + loc_max);
		for (uint32_t i = 0; i < f.size(); ++i) {
			functions->erase(f[i]);
		}
		scope = scope1;
	}
private:
	FunInfo* info;
	Scope* scope;
	Scope* funScope;
	template<class T> void addFreeVar(const T* t) {
		if (!lookupVariable(t->var()) && !globals.count(t->var())) {
			info->freeVars.insert(t->var());
		}
	}
	bool lookupVariable(const AstVar* var) {
		for (Scope* s = scope; s; s = s->parent()) {
			if (s->lookupVariable(var->name()) == var
					&& (!s->parent()
							|| s->parent()->lookupVariable(var->name()) != var)) {
				return true;
			}
			if (s == funScope) {
				break;
			}
		}
		return false;
	}
	bool lookupVariable(Scope* s, const AstVar* var) {
		for (; s; s = s->parent()) {
			if (s->lookupVariable(var->name()) == var
					&& (!s->parent()
							|| s->parent()->lookupVariable(var->name()) != var)) {
				return true;
			}
		}
		return false;
	}
	bool global;
	std::map<std::string, FunInfo*>* functions;
	FreeVarsVisitor(AstFunction* v, std::map<std::string, FunInfo*>* f) :
			global(false), functions(f) {
		init(v);
	}
	void init(AstFunction* v) {
		info = &infos[v];
		info->bcfun = new BCFunction(v);
		scope = v->node()->body()->scope();
		funScope = scope->parent();
		v->node()->body()->visit(this);
	}
};

} /* namespace mathvm */

#endif /* FREEVARSVISITOR_H_ */
