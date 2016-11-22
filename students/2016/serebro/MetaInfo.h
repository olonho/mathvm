//
// Created by andy on 11/22/16.
//

#ifndef PROJECT_METAINFO_H
#define PROJECT_METAINFO_H

#include <unordered_map>
#include <stack>
#include "mathvm.h"
#include "util.h"

namespace mathvm
{

struct BytecodeScope {
private:
    BytecodeScope(uint32_t bytecodeBeginIndex,
                  BytecodeScope* parent,
                        uint16_t depth, vector<VarType> &&types) :
            _bytecodeBeginIndex(bytecodeBeginIndex),
            parent(parent),
            depth(depth),
            localsAndParamsTypes(move(types))
    { }
public:
    static BytecodeScope* createToplevelScope(vector<VarType> &&types) {
        return new BytecodeScope(0, nullptr, 0, move(types));
    }

    BytecodeScope* addChildScope(uint32_t bytecodeBeginIdx, vector<VarType> &&types, bool isFunctionScope) {
        BytecodeScope* newScope = new BytecodeScope(bytecodeBeginIdx, this, depth + 1, move(types));
        if (!isFunctionScope) {
            children.push_back(newScope);
        }
        return newScope;
    }

    vector<vector<AnonymousVar>> paramsAndLocals;
    uint32_t _bytecodeBeginIndex;
    uint32_t _bytecodeEndIndex;
    vector<BytecodeScope*> children;
    BytecodeScope *parent;
    uint16_t depth;
    const vector<VarType> localsAndParamsTypes;

    size_t localsAndParamsNumber() const {
        return localsAndParamsTypes.size();
    }

    ~BytecodeScope() {
        for (auto child: children) {
            delete child;
        }
    }

    void enter() {
        if (localsAndParamsTypes.empty()) {
            return;
        }

        paramsAndLocals.push_back(vector<AnonymousVar>{});
        paramsAndLocals.back().reserve(localsAndParamsTypes.size());

        for (VarType t : localsAndParamsTypes) {
            paramsAndLocals.back().push_back(t);
        }
    }

    void setIntValue(uint16_t varId, int64_t value) {
        assert(varId < localsAndParamsTypes.size());
        paramsAndLocals.back()[varId].setIntValue(value);
    }

    void setStringValue(uint16_t varId, const char * value) {
        assert(varId < localsAndParamsTypes.size());
        paramsAndLocals.back()[varId].setStringValue(value);
    }

    void setDoubleValue(uint16_t varId, double value) {
        assert(varId < localsAndParamsTypes.size());
        paramsAndLocals.back()[varId].setDoubleValue(value);
    }

    void setValue(uint16_t varId, Var value) {
        assert(varId < localsAndParamsTypes.size());
        paramsAndLocals.back()[varId] = value;
    }

    int64_t getIntValue(uint16_t varId) {
        assert(varId < localsAndParamsTypes.size());
        return paramsAndLocals.back()[varId].getIntValue();
    }

    const char *getStringValue(uint16_t varId) {
        assert(varId < localsAndParamsTypes.size());
        return paramsAndLocals.back()[varId].getStringValue();
    }

    double getDoubleValue(uint16_t varId) {
        assert(varId < localsAndParamsTypes.size());
        return paramsAndLocals.back()[varId].getDoubleValue();
    }

    bool empty() const {
        return _bytecodeBeginIndex == _bytecodeEndIndex;
    }

    void exit() {
        if (localsAndParamsTypes.empty()) {
            return;
        }
        paramsAndLocals.pop_back();
    }

    bool contains(uint32_t codeIndex) const {
        return _bytecodeBeginIndex <= codeIndex && _bytecodeEndIndex > codeIndex;
    }
};

struct MetaInfo {
    unordered_map<uint16_t, BytecodeScope*> funcToScope;
    unordered_map<string, uint16_t> toplevelVarNameToId;
    ~MetaInfo() {
        delete funcToScope[0];
    }
};

}


#endif //PROJECT_METAINFO_H
