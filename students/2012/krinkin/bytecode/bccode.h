#ifndef __BC_CODE_H__
#define __BC_CODE_H__

#include "mathvm.h"
#include "ast.h"

#include <cassert>
#include <utility>
#include <string>
#include <stack>
#include <map>

using namespace mathvm;

#define LOCK_RESULT (1 << 1)
#define SAVE_RESULT (1 << 2)
#define UNLOCK_RESULT (1 << 3)

class BCCode : public Code
{
public:
    virtual Status* execute(vector<Var*>& vars);
    
private:
    std::stack<BytecodeFunction *> m_bytecode;
    std::stack<Scope *> m_scope;

    std::map<AstVar const * const, std::pair<uint16_t, uint16_t> > m_variables;
    std::map<AstFunction const * const, BytecodeFunction *> m_functions;
    
    std::map<std::pair<uint16_t, uint32_t>, uint8_t> m_annotations;

public:
    void annotate(uint16_t id, uint32_t bci, uint8_t flag)
    {
        std::pair<uint16_t, uint32_t> pos(id, bci);
        if (m_annotations.count(pos) == 0) m_annotations[pos] = flag;
        else m_annotations[pos] |= flag;
    }
    
    uint8_t annotation(uint16_t id, uint32_t bci)
    {
        std::pair<uint16_t, uint32_t> pos(id, bci);
        if (m_annotations.count(pos) == 0) return (uint8_t) 0;
        else return m_annotations[pos];
    }

    AstVar *lookup_variable(std::string const &name)
    {
        return scope()->lookupVariable(name);
    }
    
    std::pair<uint16_t, uint16_t> lookup_variable(AstVar const * const var)
    {
        std::map<AstVar const * const, std::pair<uint16_t, uint16_t> >::iterator it =
                                                                            m_variables.find(var);
        if (it == m_variables.end()) assert(0);
        return it->second;
    }

    AstFunction *lookup_function(std::string const &name)
    {
        return scope()->lookupFunction(name);
    }
    
    BytecodeFunction *lookup_function(AstFunction const * const fun)
    {
        std::map<AstFunction const * const, BytecodeFunction *>::iterator it =
                                                                            m_functions.find(fun);
        if (it == m_functions.end()) assert(0);
        return it->second;
    }

    Scope *scope() { return m_scope.top(); }
    void push_scope(Scope *scope) { m_scope.push(scope); };
    void pop_scope() { m_scope.pop(); }

    Bytecode *bytecode() { return m_bytecode.top()->bytecode(); }
    void push_bytecode(BytecodeFunction *fun) { m_bytecode.push(fun); }
    void pop_bytecode() { m_bytecode.pop(); }
    
    uint16_t current_id() const { return m_bytecode.top()->id(); }
    
    uint16_t declare_function(AstFunction *afun)
    {
        BytecodeFunction *bfun = new BytecodeFunction(afun);
        m_functions.insert(std::make_pair(afun, bfun));
        return addFunction(bfun);
    }
    
    std::pair<uint16_t, uint16_t> declare_variable(AstVar *var)
    {
        BytecodeFunction *bfun = m_bytecode.top();
        uint16_t scope_id = bfun->id();
        uint16_t local_id = bfun->localsNumber();
        std::pair<uint16_t, uint16_t> var_id(scope_id, local_id);
        m_variables.insert(std::make_pair(var, var_id));
        local_id += size(var->type());
        bfun->setLocalsNumber(local_id);
        return var_id;
    }
    
    uint16_t size(VarType type)
    {
        switch (type)
        {
        case VT_INT: return sizeof(int64_t);
        case VT_DOUBLE: return sizeof(double);
        case VT_STRING: return sizeof(char *);
        default: assert(0);
        }
    }
    
    size_t function_count()
    {
        Code::FunctionIterator it(this);
        size_t count = 0;
        while (it.next() != 0) ++count;
        return count;
    }

    size_t insn_size(Instruction insn) const
    {
        #define GET_SIZE(b, d, l) case BC_##b: return l;
        switch (insn)
        {
        FOR_BYTECODES(GET_SIZE)
        default: assert(0);
        }
        #undef GET_SIZE
    }
};

#endif /* __BC_CODE_H__ */
