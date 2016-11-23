#ifndef INTERPRETABLE_CODE_H
#define INTERPRETABLE_CODE_H

#include "mathvm.h"

namespace mathvm
{

class InterpretableCode : public Code
{
public:
    Status* execute(vector<Var*>& vars) override;
};

}   // namespace mathvm

#endif  // INTERPRETABLE_CODE_H
