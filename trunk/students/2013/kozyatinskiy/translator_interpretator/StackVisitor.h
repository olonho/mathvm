#pragma once

#include <ast.h>
using namespace mathvm;

#include "CallStack.h"

#include <map>
using std::map;

#include <vector>
using std::vector;

#include <set>
using std::set;

#include <stdexcept>

/*
* Generate Call Stack for each call
* default strategy is push all local variables and pointers from previous call stack
* mark pointer and variables is used
* remove other variable and pointers from stack
* every call node - one processing
*/

#define FOR_VISIT_NODES(DO) \
	DO(BinaryOp) \
	DO(For) \
	DO(If)  \
	DO(NativeCall) \
	DO(Print) \
	DO(Return) \
	DO(UnaryOp) \
	DO(While)

class StackVisitor : AstVisitor
{
public:
	~StackVisitor(void)
	{
		freeStacks();
	}

	void callStartFunction(AstFunction* f)
	{
		capturedPerFunction_.clear();

		CallStack* fst = new CallStack();
		curStacks_.push_back(fst);
		allStacks_.push_back(curStack());
		
		visitFunctionNode(f->node());
		
		freeStacks();
	}

	const map<AstFunction*, set<pair<VarType, string> > >& result() const
	{
		return capturedPerFunction_;
	}

private:
	virtual void visitFunctionNode(FunctionNode* function)
	{
		curFunctions_.push_back(function);
		function->body()->visit(this);
		curFunctions_.pop_back();
	}

	virtual void visitBlockNode(BlockNode* block)
	{
		lastFunction_ = curFunction();
		curStacks_.push_back(
			curStack()->enterBlock(block->scope(), lastFunction_ ? lastFunction_->signature() : Signature())
			);
		lastFunction_ = 0;

		allStacks_.push_back(curStack());

		curScope_.push_back(block->scope());

		block->visitChildren(this);

		curScope_.pop_back();
		curStacks_.pop_back();
	}

	virtual void visitCallNode(CallNode* callNode)
	{
		if (already_.find(callNode) == already_.end())
		{
			already_.insert(callNode);
			curStack()->saveAndClearMarks();

			AstFunction* f = curScope_.back()->lookupFunction(callNode->name());
			if (!f) throw std::invalid_argument("undefined function");

			f->node()->visit(this);

			const vector<pair<VarType, string> >& captured = curStack()->extractUsed();
			copy(captured.begin(), captured.end(), 
				inserter(capturedPerFunction_[f], capturedPerFunction_[f].begin())); 
		
			curStack()->restoreMarks();
		}
	}

	virtual void visitLoadNode(LoadNode* loadNode)
	{
		SingleVar* var = curStack()->lookup(loadNode->var()->type(), loadNode->var()->name());
		if (!var) throw std::invalid_argument("undefined variable");
		var->markAsUsed();
	}

	virtual void visitStoreNode(StoreNode* storeNode)
	{
		SingleVar* var = curStack()->lookup(storeNode->var()->type(), storeNode->var()->name());
		if (!var) throw std::invalid_argument("undefined variable");
		var->markAsUsed();
		
		storeNode->visitChildren(this);
	}

#define PROCESS(type) \
	virtual void visit##type##Node( type##Node* t ) \
	{ \
		t->visitChildren(this); \
	}
	FOR_VISIT_NODES(PROCESS)
#undef PROCESS

private:
	CallStack* curStack() const
	{
		return curStacks_.back();
	}

	FunctionNode* curFunction() const
	{
		return curFunctions_.back();
	}

	void freeStacks()
	{
		for (size_t i = 0; i < allStacks_.size(); ++i)
			delete allStacks_[i];
		allStacks_.clear();
		curStacks_.clear();
		already_.clear();
		curScope_.clear();
		curFunctions_.clear();
		lastFunction_ = 0;
	}

	vector<CallStack*> allStacks_;
	vector<CallStack*> curStacks_;

	set<CallNode*> already_;
	map<AstFunction*, set<pair<VarType, string> > > capturedPerFunction_;
	map<CallNode*, vector<SingleVar*> > usedVars_;

	FunctionNode* lastFunction_;
	vector<Scope*> curScope_;
	vector<FunctionNode*> curFunctions_;
};
