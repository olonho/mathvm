#include "MyMemoryManager.h"


MyMemoryManager::MyMemoryManager(void)
{
}


MyMemoryManager::~MyMemoryManager(void)
{
	for (size_t i = 0; i < vars_.size(); ++i)
		for (size_t j = 0; j < vars_[i].size(); ++j)
			;//now memory leak delete vars_[i][j];
}
