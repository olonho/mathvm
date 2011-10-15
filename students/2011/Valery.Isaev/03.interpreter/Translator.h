#ifndef _TRANSLATOR_H_
#define _TRANSLATOR_H_

#include <map>
#include <vector>

#include "ast.h"
#include "Interpreter.h"

class Translator: public mathvm::AstVisitor {
    mathvm::Bytecode* code;
    mathvm::Code* prog;
    typedef uint16_t VarInt;
    std::map<std::string, std::vector<VarInt> > vars;
    VarInt currentVar;
    bool overflow;
    mathvm::VarType currentType, resultType;
    
    void put(const void* buf, unsigned int size);
    template<class T> void putVar(mathvm::Instruction ins, T* var);
    void checkTypeInt(mathvm::AstNode* expr);
    void triple(mathvm::Instruction i);
public:
    Translator(mathvm::Code* p);
    mathvm::Status translate(mathvm::AstFunction* node);
#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

#endif
