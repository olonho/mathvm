#ifndef _TRANSLATOR_H_
#define _TRANSLATOR_H_

#include <map>
#include <vector>

#include "ast.h"
#include "MvmCode.h"

class Translator: public mathvm::AstVisitor {
    mathvm::Bytecode* code;
    MvmCode* prog;
    typedef uint16_t VarInt;
    std::map<std::string, std::vector<VarInt> > vars;
    VarInt currentVar;
    bool overflow;
    mathvm::VarType currentType, resultType;
    
    void addVar(mathvm::AstNode* node, const std::string& name);
    void delVar(const std::string& name);
    void put(const void* buf, unsigned int size);
    void putVar(mathvm::Instruction ins, const mathvm::AstVar* var, mathvm::AstNode* node = 0);
    void checkTypeInt(mathvm::AstNode* expr);
    void triple(mathvm::Instruction i);
public:
    Translator(MvmCode* p);
    mathvm::Status translate(mathvm::AstFunction* fun);
#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

#endif
