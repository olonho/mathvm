#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include "mathvm.h"

class Interpreter: public mathvm::Code {
    typedef uint16_t VarInt;
    typedef std::map<std::string, VarInt> VarMap;
    VarMap _varMap;
public:
    mathvm::Status* execute(std::vector<mathvm::Var*>& vars);
    VarMap& varMap() { return _varMap; }
};

#endif
