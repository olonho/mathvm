#include "context.hpp"

namespace mathvm {

Context::Context(CodeImpl *code) :
    m_id(0)
  , m_code(code)
  , m_parent(NULL)
{
    m_ancestor_contexts = new std::vector<Context*>();
    m_ancestor_contexts->push_back(this);
}

Context::Context(Context *parent, uint16_t id) :
    m_id(id)
  , m_parent(parent)
{
    m_code = parent->m_code;
    m_ancestor_contexts = parent->m_ancestor_contexts;
}

Context::~Context() {
    for (size_t i = 0; i < m_variables.size(); ++i)
        delete m_variables[i];

    if (NULL != m_parent) {
        for (size_t i = 0; i < m_ancestor_contexts->size(); ++i)
            delete m_ancestor_contexts->at(i);
        delete m_ancestor_contexts;
    }
}

Context* Context::addChildContext() {
    uint16_t id = m_ancestor_contexts->size();
    if (id >= MAXID)
        throw TranslationError("Scope limit reached");
    Context* c = new Context(this, id);
    m_ancestor_contexts->push_back(c);
    return c;
}

uint16_t Context::introduceVar(const string &name, VarType type) {
    if (m_variables.size() >= MAXID)
        throw TranslationError("Couldn't introduce variable: ID limit reached");
    if (m_variables_map.find(name) != m_variables_map.end())
        throw TranslationError("Couldn't introduce variable with same name");
    uint16_t id = m_variables.size();
    m_variables_map[name] = id;
    m_variables.push_back(new Var(type, name));
    return id;
}

uint16_t Context::introduceStringConst(const string &c) {
    return m_code->makeStringConstant(c);
}

uint16_t Context::addFunction(BytecodeFunction *func) {
    uint16_t id = m_code->addFunction(func);
    if (id >= MAXID)
        throw TranslationError("Couldn't add function: ID limit reached");
    if (m_functions_map.find(func->name()) != m_functions_map.end())
        throw TranslationError("Couldn't add function with same name");
    m_functions_map[func->name()] = id;
    func->setScopeId(id);
    return id;
}

var_t Context::varId(const string &varname) {
    idmap_t::const_iterator it = m_variables_map.find(varname);
    if (it == m_variables_map.end()) {
        if (NULL == m_parent)
            throw TranslationError("Variable not found");

        return m_parent->varId(varname);
    }
    return std::make_pair(m_id, (*it).second);
}

Var* Context::var(uint16_t scope, uint16_t var) {
    if (scope >= m_ancestor_contexts->size())
        throw TranslationError("Couldn't find var: wrong scope");
    Context* c = m_ancestor_contexts->at(scope);
    if (var >= c->m_variables.size())
        throw TranslationError("Couldn't find var: no such var in scope");
    return c->m_variables[var];
}

uint16_t Context::functionId(const string &name) {
    return function(name)->id();
}

BytecodeFunction* Context::function(const string &name) {
    idmap_t::const_iterator it = m_functions_map.find(name);
    if (it == m_functions_map.end()) {
        if (NULL == m_parent)
            throw TranslationError("Couldn't find function");

        return m_parent->function(name);
    }
    return (BytecodeFunction*)m_code->functionById((*it).second);
}

}
