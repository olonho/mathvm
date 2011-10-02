#ifndef _TRANSLATOR_H_
#define _TRANSLATOR_H_

#include <map>
#include <vector>

#include "ast.h"
#include "Bytecode.h"

class Translator: public mathvm::AstVisitor {
    Bytecode& code;
    typedef uint8_t VarInt;
    std::map<std::string, std::vector<VarInt> > vars;
    VarInt currentVar;
    bool overflow;
    mathvm::VarType currentType;
    
    void put(const void* buf, unsigned int size);
    void putVar(const std::string& var);
    void checkTypeInt(mathvm::AstNode* expr);
    void triple(mathvm::Instruction i);
//    void visitLogicNode(mathvm::AstNode* expr);
public:
    Translator(Bytecode& c);
    mathvm::Status translate(mathvm::AstNode* node);
#define VISITOR_FUNCTION(type, name) \
    void visit##type(mathvm::type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

#endif
