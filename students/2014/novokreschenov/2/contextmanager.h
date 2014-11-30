#ifndef CONTEXTMANAGER_H
#define CONTEXTMANAGER_H

#include "mathvm.h"
#include "exceptions.h"

namespace mathvm {

class ContextVar
{
    union {
        double d;
        int64_t i;
        uint16_t id;
    } _value;

    VarType _type;

public:
    ContextVar();
    ContextVar(double d);
    ContextVar(int64_t i);
    ContextVar(uint16_t id);

    VarType type();

    double doubleValue();
    int64_t intValue();
    uint16_t stringIdValue();

    void setDouble(double d);
    void setInt(int64_t i);
    void setStringId(uint16_t id);
};

class InterpreterStack
{
    vector<ContextVar> _elements;

public:
    void pushElement(ContextVar element);
    void pushDouble(double d);
    void pushInt(int64_t i);
    void pushStringId(uint16_t id);

    ContextVar getElement();
    double getDouble();
    int64_t getInt();
    uint16_t getStringId();

    ContextVar popElement();
    double popDouble();
    int64_t popInt();
    uint16_t popStringId();

    void swapTwoUpperElements();
};

class ContextManager
{
    typedef std::map<uint16_t, ContextVar> Context;
    typedef std::vector<Context> RecursiveContextVec;
    typedef std::map<uint16_t, RecursiveContextVec> ContextMap;

    ContextMap _contextById;
    uint16_t _currentContextId;

    void checkContext(uint16_t contextId);
    void checkCtxVar(uint16_t contextId, uint16_t varId);

public:
    Context& getContext(uint16_t contextId);
    void addContext(uint16_t contextId);
    void removeContext(uint16_t contextId);

    void setCurrentContextId(uint16_t contextId);
    uint16_t getCurrentContextId();

    double loadDoubleFromCtxVar(uint16_t contextId, uint16_t varId);
    double loadDoubleFromVar(uint16_t varId);
    int64_t loadIntFromCtxVar(uint16_t contextId, uint16_t varId);
    int64_t loadIntFromVar(uint16_t varId);
    uint16_t loadStringIdFromCtxVar(uint16_t contextId, uint16_t varId);
    uint16_t loadStringIdFromVar(uint16_t varId);

    void storeDoubleToCtxVar(uint16_t contextId, uint16_t varId, double dvalue);
    void storeDoubleToVar(uint16_t varId, double dvalue);
    void storeIntToCtxVar(uint16_t contextId, uint16_t varId, int64_t ivalue);
    void storeIntToVar(uint16_t varId, int64_t ivalue);
    void storeStringIdToCtxVar(uint16_t contextId, uint16_t varId, uint16_t svalue);
    void storeStringIdToVar(uint16_t varId, uint16_t svalue);
};

}


#endif // CONTEXTMANAGER_H
