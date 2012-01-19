#ifndef EXTENDEDBYTECODEFUNCTION_H
#define EXTENDEDBYTECODEFUNCTION_H

#include "Scopes.h"

class ExtendedBytecodeFunction: public mathvm::BytecodeFunction {
private:
    uint16_t myVariablesNum;
    std::vector<mathvm::VarType> myArgumentTypes;
public:
    ExtendedBytecodeFunction(FunctionScope const * functionScope): mathvm::BytecodeFunction(functionScope->getAstFunction()), myVariablesNum(functionScope->getTotalVariablesNum()) {
        mathvm::AstFunction* fun = functionScope->getAstFunction();
        for (uint32_t i = 0; i < fun->parametersNumber(); ++i) {
            myArgumentTypes.push_back(fun->parameterType(i));
        }
    }
    uint16_t GetVariablesNum() const {
        return myVariablesNum;
    }
    std::vector<mathvm::VarType> const & getArgumentTypes() const {
        return myArgumentTypes;
    }
};
#endif
