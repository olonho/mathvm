#ifndef CODEANALYZER_H
#define CODEANALYZER_H

#include "mathvm.h"
#include "ast.h"

#include <set>
#include <stack>
#include <stdexcept>

namespace mathvm {

// there are some bugs in std::pair destoying, so we use this workaround
struct VarUsage {
    VarUsage(const std::string &var, uint16_t funcId) : var(var), funcId(funcId) {}

    bool operator==(const VarUsage &other) const {
        return (var == other.var && funcId == other.funcId);
    }

    bool operator<(const VarUsage &other) const {
        if(var != other.var) return var < other.var;
        return funcId < other.funcId;
    }

    std::string var;
    uint16_t funcId;
};

typedef std::vector<VarUsage> VarUsageList;
typedef std::map<VarUsage, uint32_t> VarAddressMap;

struct FunctionDescription {
    FunctionDescription() {}
    FunctionDescription(uint16_t id, const std::string &name) :
        id(id), name(name), varsToStore(0),
        hasIntPrintNode(false), canReuseMathVars(false) {}

    uint16_t id;
    std::string name;
    std::vector<std::string> params;
    std::vector<std::string> locals;
    std::vector<uint16_t> calls;

    std::vector<VarType> localTypes;

    uint32_t varsToStore;
    VarUsageList useVars;       //external vars used by this function
    VarUsageList closureVars;   //closure vars used in this function and child calls

    VarAddressMap varAddresses;

    //reusable print buffer for string literals and int variables
    //just for AsmJit performance
    bool hasIntPrintNode;

    //reusable buffer for int vars
    bool canReuseMathVars;
    std::set<BinaryOpNode*> nodesWithReuse;

    inline bool isNodeWithMathReuse(BinaryOpNode *node) {
        return nodesWithReuse.find(node) != nodesWithReuse.end();
    }

    void generateMetadata();
    uint32_t getAddress(const std::string &var) const;
    uint32_t getAddress(const std::string &var, uint16_t ownerId) const;
};

class CodeAnalyzer : public AstVisitor {
public:
    CodeAnalyzer() {}

    void analyze(AstFunction *top);

    FunctionDescription &getMetadata(size_t id) {
        return funcDescrs[id];
    }

    FunctionDescription *operator[](size_t id) {
        return &funcDescrs[id];
    }

    std::vector<FunctionDescription> &getAllMetadata() {
        return funcDescrs;
    }

private:
#define VISITOR_FUNCTION(type, name) void visit##type(type* node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    void enterScope(Scope *scope);
    void exitScope(Scope *scope);

    void useVar(const AstVar *var);
    inline bool hasVar(const std::vector<std::string> &list, const std::string &name) const;
    inline bool hasVar(const VarUsageList &list, const VarUsage &var) const;
    inline bool hasVar(const VarUsageList &list, const std::string &name, uint16_t userId) const;
    inline bool isLocalVar(const FunctionDescription &fd, const VarUsage &var) const;
    inline bool isLocalVar(const FunctionDescription &fd, const std::string &var) const;

    void findExtUsages(FunctionDescription &descr);

    bool canReuseVars(BinaryOpNode *node) const;

    //------------------------------------------------------------------

    template<typename T>
    class StackStorage {
        std::map<std::string, std::stack<T> > storage;

    public:
        StackStorage() {}

        void clear() {
            storage.clear();
        }

        void pushData(const std::string &name, const T &data) {
            storage[name].push(data);
        }

        T popData(const std::string &name) {
            T d = storage[name].top();
            storage[name].pop();
            return d;
        }

        T topData(const std::string &name) const {
            typename std::map<std::string, std::stack<T> >::const_iterator it = storage.find(name);
            if(it != storage.end()) return it->second.top();
            throw std::logic_error("Storage with name " + name + " not found");
        }
    };

    //------------------------------------------------------------------

    StackStorage<uint16_t> varUsage;
    std::vector<FunctionDescription> funcDescrs;
    std::map<std::string, uint16_t> funcIds;
    uint16_t currentFuncID;

    std::set<uint16_t> visitedFuncs;
    bool extUsageChanged;
    bool recursiveBinaryOp, inStoreNode;
};

class IfChecker : public AstVisitor {
public:
    IfChecker() {}

    bool hasReturn(BlockNode *node);

private:
    void visitReturnNode(ReturnNode *node);
    void visitForNode(ForNode *node);
    void visitWhileNode(WhileNode *node);
    void visitBlockNode(BlockNode *node);
    void visitIfNode(IfNode *node);

    bool returnFound;

};

}

#endif // CODEANALYZER_H
