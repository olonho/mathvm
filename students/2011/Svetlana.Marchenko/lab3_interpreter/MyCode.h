#pragma once

#include "mathvm.h"

class MyCode :
  public mathvm::Code
{
public:
  MyCode(void);
  ~MyCode(void);
  mathvm::Status* execute(std::vector<mathvm::Var*>& vars);
};

