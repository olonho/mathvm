#ifndef INTERPRETATOR_H
#define INTERPRETATOR_H
#include "mathvm.h"
using namespace mathvm;
class InterpretCode : public Code
{
public:    
    virtual Status * execute(vector<Var *> & vars)
    {

        return Status::Ok();
    }

};

#endif // INTERPRETATOR_H
