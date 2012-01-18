#include "FuncStackFrame.h"


FuncStackFrame::FuncStackFrame(uint64_t stackSize) : _stackSize(stackSize), _isEmpty(true)
{
	_stack = (FuncFrame *)(new char[_stackSize]);
	_topStack = _stack;
	
}


FuncStackFrame::~FuncStackFrame(void)
{
	delete[] (char*)_stack;
}

FuncFrame* FuncStackFrame::addFrame(mathvm::TranslatedFunction *func) {
	if (_isEmpty) {
		_topStack->_id = func->id();
		_topStack->_insnPtr = 0;
		_topStack->_prevCall = 0;
		_topStack->_prevFunc = 0;
		_topStack->_countVars = func->localsNumber();
		_topStack->_countParams = func->parametersNumber();
		_topStack->_vars = (char*)&(_topStack->_vars) + sizeof(_topStack->_vars); //_vars points to the next cell
		_isEmpty = false;
	} else {
		FuncFrame *newFrame = (FuncFrame *)(_topStack + _topStack->size());
		newFrame->_id = func->id();
		newFrame->_insnPtr = 0;
		newFrame->_prevCall = _topStack;
		(_topStack->_id == newFrame->_id) ? newFrame->_prevFunc = _topStack->_prevFunc : 
			newFrame->_prevFunc = _topStack; 
		newFrame->_countVars = func->localsNumber();
		newFrame->_countParams = func->parametersNumber();
		newFrame->_vars = (char*)&(newFrame->_vars) + sizeof(newFrame->_vars); 
		_topStack = newFrame;
	}
	return _topStack;
}

FuncFrame* FuncStackFrame::firstFrameWithId(uint16_t id) {
	FuncFrame *searchFrame = _topStack;
	while (searchFrame->_id != id && searchFrame != 0) {
		searchFrame = searchFrame->_prevFunc;
	}
	if (searchFrame == 0) throw ExecutionException("Error: can not find the variable in the context");
	return searchFrame;
}

StackValue FuncStackFrame::getVar(uint16_t ctx, uint16_t id) {
	return firstFrameWithId(ctx)->getVar(id);
}

StackValue FuncStackFrame::getVar(uint16_t id) {
	return _topStack->getVar(id);
}

void FuncStackFrame::updateVar(uint16_t ctx, uint16_t id, StackValue val) {
	firstFrameWithId(ctx)->setVar(id, val);
}
	
void FuncStackFrame::updateVar(uint16_t id, StackValue val) {
	_topStack->setVar(id, val);
}
	
void FuncStackFrame::removeTop() {
	if (_topStack->_prevCall) {
		_topStack = _topStack->_prevCall;
	} else {
		_isEmpty = true;
		_topStack = _stack;
	}
}

bool FuncStackFrame::isEmpty() {
	return _isEmpty;
}

FuncFrame* FuncStackFrame::getCurFrame() {
	return _topStack;
}
