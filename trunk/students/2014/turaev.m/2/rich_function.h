#include "mathvm.h"
#include "ast.h"

using namespace std;
using namespace mathvm;

class RichFunction : public BytecodeFunction {
    map<string, pair<VarType, uint16_t> > _local_variables;
    uint16_t _index; // function's index in the chain of nested functions
    RichFunction *_parent;  //TODO: use base class
    uint16_t _id;
public:
    RichFunction(AstFunction *function, RichFunction *parent, uint16_t index): BytecodeFunction(function), _index(index), _parent(parent), _id(0) {
        for (uint16_t i = 0; i < parametersNumber(); ++i) {
            _local_variables[parameterName(i)] = std::make_pair(parameterType(i), _id++);
        }
    }

    RichFunction *lookupParentFunction(const string &name) {
        return
            _local_variables.find(name) == _local_variables.end()
            ? _parent->lookupParentFunction(name)
            : this;
    }

    uint16_t getIndex() {
        return _index;
    }

    uint16_t getVariableId(const string &name) {
        return _local_variables[name].second;
    }

    void addLocalVariable(const string &name, VarType type) {
        _local_variables[name] = std::make_pair(type, _id++);
    }

    virtual ~RichFunction() {

    }
};