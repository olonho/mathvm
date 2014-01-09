#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <string>
#include <map>
#include <vector>
#include <memory>

#include "logger.hpp"
#include "ast.h"
#include "mathvm.h"
#include "translationerror.hpp"
#include "codeimpl.hpp"

namespace mathvm {

typedef std::pair<uint16_t, uint16_t> var_t;
typedef std::map<std::string, uint16_t> idmap_t;
static const uint16_t MAXID = 0xffff;

inline bool checkVart(var_t t) {
    return t.first < MAXID && t.second < MAXID;
}

class Context {
public:
    explicit Context(CodeImpl* code);
    Context(Context* parent, uint16_t id);
    ~Context();

    uint16_t id() const {
        return m_id;
    }

    CodeImpl* codeImpl() const {
        return m_code;
    }

    Context* addChildContext();

    uint16_t introduceVar(const std::string& name, VarType type);
    uint16_t introduceStringConst(const std::string& c);
    uint16_t addFunction(BytecodeFunction* func);

    var_t varId(const std::string& varname);
    Var* var(uint16_t scope, uint16_t var);

    uint16_t functionId(const std::string& name);
    BytecodeFunction* function(const std::string& name);


private:
    uint16_t m_id;

    CodeImpl* m_code;
    Context* m_parent;
    std::vector<Context*>* m_ancestor_contexts;

    idmap_t m_functions_map;
    idmap_t m_variables_map;
    std::vector<Var*> m_variables;

};

} //namespace

#endif // CONTEXT_HPP
