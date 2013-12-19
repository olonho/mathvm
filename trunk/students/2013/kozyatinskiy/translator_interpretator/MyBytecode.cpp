#include "MyBytecode.h"

#include "MyInstruction.h"

MyBytecode::MyBytecode(void){}

MyBytecode::~MyBytecode(void)
{
	for (size_t i = 0; i < instructions_.size(); ++i)
		delete instructions_[i];
}

void MyBytecode::addInstruction(MyInstruction* instruction)
{
	instructions_.push_back(instruction);
}

void MyBytecode::dump(ostream& os)
{
	for (size_t i = 0; i < instructions_.size(); ++i)
		os << instructions_[i]->text() << endl;
}

void MyBytecode::removeLabels()
{
	map<int, int> labelToNum;
	size_t labelCount = 0;
	vector<MyInstruction*> withoutLabels;
	vector<MyInstruction*> labels;
	for (size_t i = 0; i < instructions_.size(); ++i)
	{
		MyLabel* lb = dynamic_cast<MyLabel*>(instructions_[i]);
		if (lb)
		{
			labelToNum[lb->id()] = i - labelCount;
			labelCount++;
			labels.push_back(lb);
		}
		else
			withoutLabels.push_back(instructions_[i]);
	}
	instructions_.clear();
	//instructions_.swap(withoutLabels);
	for(size_t i = 0; i < withoutLabels.size(); ++i)
	{
		Jump* j = dynamic_cast<Jump*>(withoutLabels[i]);
		if (j && j->labelId() != -1)
			j->replaceToOffset(labelToNum[j->labelId()] - i);

		instructions_.push_back(withoutLabels[i]);
	}
	for(size_t i = 0; i < labels.size(); ++i)
		delete labels[i];
}

MyInstruction* MyBytecode::take(int num) 
{ 
	MyInstruction* tmp = instructions_[num];
	instructions_[num] = 0;
	return tmp; 
}

MyInstruction* MyBytecode::get(int num) const{ return instructions_[num]; }

size_t MyBytecode::instructionCount() const{ return instructions_.size(); }
