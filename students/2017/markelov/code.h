#ifndef STUDENTS_2017_MARKELOV_CODE_H_
#define STUDENTS_2017_MARKELOV_CODE_H_

#include "parser.h"
#include "mathvm.h"

#include <map>
#include <set>
#include <vector>
#include <iostream>

using namespace mathvm;
using namespace std;

struct CodeScope;

class ScopedCode: public Code {
    set<pair<uint16_t, uint16_t>> scopeEdges;
    map<uint16_t, vector<uint16_t>> graph;
    map<uint16_t, CodeScope*> scopeMap;
public:
    virtual ~ScopedCode();
    void addScopeEdge(uint16_t p, uint16_t sc) {
        graph[p].push_back(sc);
        scopeEdges.insert(make_pair(p, sc));
    }
    vector<uint16_t>& getEdges(uint16_t p) {
        return graph[p];
    }
    bool hasEdge(int16_t p, uint16_t sc) {
        return scopeEdges.count(make_pair(p, sc));
    }
    void addScope(uint16_t id, CodeScope * sc) {
        scopeMap[id] = sc;
    }
    CodeScope * getScope(uint16_t id) {
        if (!scopeMap.count(id))
            return nullptr;
        return scopeMap[id];
    }
};

struct CodeScope {
    uint16_t scopeid;
    BytecodeFunction * f;
    AstFunction * astf;
    CodeScope * p;
    uint32_t sbci, ebi;
    std::map<std::string, uint16_t> var_map;
    Scope * legacy_scope;
    bool isFunction;
    bool isGlobal = false;
    CodeScope * findPrevFuncScope() {
        CodeScope * fscope = this->p;
                while (fscope != nullptr) {
                    if (fscope->isFunction)
                        return fscope;
                    fscope = fscope->p;
                }
                return nullptr;
    }

/*============================ */
    CodeScope(
            ScopedCode * code,
            CodeScope * p,
            Scope * scope,
            uint16_t scopeid,
            AstFunction * astf =
            nullptr) :
            scopeid(scopeid), p(p), legacy_scope(scope) {
        if (astf != nullptr) {
            this->f = new BytecodeFunction(astf);
            this->f->setScopeId(scopeid);
            this->astf = astf;
            isFunction = true;
        } else {
            this->f = p->f;
            this->astf = p->astf;
            isFunction = false;
        }


        code->addScope(this->scopeid, this);

        CodeScope * fscope = findPrevFuncScope();
        if (fscope != nullptr)
            code->addScopeEdge(fscope->scopeid, this->scopeid);



        Scope::VarIterator it(scope);
        uint16_t index = 0;

        while (it.hasNext()) {
            AstVar * var = it.next();
            addVar(var->name(), index++);
        }
    }
    void addVar(const string &name, uint16_t id) {
        var_map[name] = id;
    }
    uint16_t varId(const string &name) {
        return var_map[name];
    }
    bool hasVar(const string &name) {
        return var_map.count(name);
    }
    std::tuple<uint16_t, uint16_t> outerVarId(const string &name) {
        CodeScope * current = this;
        while (current) {
            if (current->hasVar(name)) {
                return std::make_tuple(current->scopeid, current->varId(name));
            }
            current = current->p;
        }
        assert(false);
        abort();
    }

};


#endif
