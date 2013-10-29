#include "mathvm.h"

#include <map>
using std::map;
#include <string>
using std::string;
#include <stdexcept>

namespace mathvm {

struct FunctionData {
    FunctionData(uint16_t stack_size)
        : stack_size(stack_size) {}

    uint16_t stack_size;
};

class CodeImpl : public Code {
    map<uint16_t, FunctionData> funsData;
public:
    CodeImpl() {}
    virtual ~CodeImpl() {}

    /**
     * Execute this code with passed parameters, and update vars
     * in array with new values from topmost scope, if code says so.
     */
    Status * execute(vector<Var*> & vars) {
        return NULL;
    }

    void addFunctionData(uint16_t id, FunctionData data) {
        if ( !funsData.insert(make_pair(id, data)).second ) {
            throw logic_error("Duplicated FunctionData: " + id);
        }
    }

    FunctionData getFunctionData(uint16_t id) {
        map<uint16_t, FunctionData>::const_iterator it;
        it = funsData.find(id);

        if ( it != funsData.end() ) {
            return it->second;
        }

        throw logic_error("FunctionData not found: " + id);
    }
};

}