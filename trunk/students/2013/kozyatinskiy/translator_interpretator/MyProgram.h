#pragma once

#include "MyBytecode.h"
#include "MyInstruction.h"

class MyProgram
{
public:
	MyProgram(void){}
	~MyProgram(void)
	{
		for(size_t i = 0; i < functions_.size(); ++i)
			delete functions_[i];
	}

	void addInstruction(int func, MyInstruction* i)
	{
		if (func == -1)
			main_.addInstruction(i);
		else
		{
			if (functions_.size() < func + 1)
			{
				functions_.resize(func + 1);
				functions_[func] = new MyBytecode();
			}
			functions_[func]->addInstruction(i);
		}
	}

	void setStrings(const vector<string>& strings)
	{
		strings_ = strings;
	}

	void dump(ostream& os)
	{
		os << "main:" << endl;
		main_.dump(os);
		for (size_t i = 0; i < functions_.size(); ++i)
		{
			os << "function " << i << ":" << endl;
			functions_[i]->dump(os);
		}
	}

	void removeAllLabels()
	{
		main_.removeLabels();
		for (size_t i = 0; i < functions_.size(); ++i)
			functions_[i]->removeLabels();
	}

	MyBytecode* link()
	{
		MyBytecode* linked_ = new MyBytecode();
		for (size_t i = 0; i < strings_.size(); ++i)
			linked_->addInstruction(new StringConstant(strings_[i]));

		map<int, int> funcToRow;
		size_t cur = main_.instructionCount() + strings_.size();
		for (size_t i = 0; i < functions_.size(); ++i)
		{
			funcToRow[i] = cur;
			cur += functions_[i]->instructionCount();
		}

		for (size_t i = 0; i < main_.instructionCount(); ++i)
			processInstructionToLink(main_.take(i), funcToRow, linked_);
		for (size_t i = 0; i < functions_.size(); ++i)
			for (size_t j = 0; j < functions_[i]->instructionCount(); ++j)
				processInstructionToLink(functions_[i]->take(j), funcToRow, linked_);
		return linked_;
	}



private:
	MyProgram(const MyProgram&){}

	MyBytecode          main_;
	vector<MyBytecode*> functions_;
	vector<string>      strings_;

	void processInstructionToLink(MyInstruction* i, map<int, int>& funcToRow, MyBytecode* bc)
	{
		CALL* c = dynamic_cast<CALL*>(i);
		if (c)
		{
			int pointer = funcToRow[c->funcId()];
			c->setPointer(pointer);
		}
		bc->addInstruction(i);
	}
};

