#ifndef VARSCONTEXT_H
#define VARSCONTEXT_H

#include <inttypes.h>
#include <map>
#include <memory>

using std::map;
using std::pair;
using std::make_pair;
using std::shared_ptr;

template<typename K, typename V>
class VarsContext {
protected:
    map<K, shared_ptr<V> > _varsMap;
    uint16_t _id;
    uint16_t _parentId;
    bool _isTopmost;

public:
    typedef pair<bool, shared_ptr<V> > MaybeV;

    explicit VarsContext(uint16_t id)
        : _id(id)
        , _parentId(0)
        , _isTopmost(true)
    {}

    explicit VarsContext(uint16_t id, uint16_t parentId)
        : _id(id)
        , _parentId(parentId)
        , _isTopmost(false)
    {}

    explicit VarsContext(VarsContext const& other)
        : _varsMap(other._varsMap)
        , _id(other._id)
        , _parentId(other._parentId)
        , _isTopmost(other._isTopmost)
    {}

    VarsContext& operator=(VarsContext const& other) {
        if(this != &other) {
            VarsContext(other).swap(*this);
        }
        return *this;
    }

    void swap(VarsContext &other) {
        std::swap(_varsMap, other._varsMap);
        std::swap(_id, other._id);
        std::swap(_parentId, other._parentId);
        std::swap(_isTopmost, other._isTopmost);
    }

    uint16_t id() { return _id; }
    uint16_t parentId() { return _parentId; }
    bool isTopmost() { return _isTopmost; }
    uint32_t varsNumber() const { return _varsMap.size(); }

    MaybeV get(K const& key) const {
        auto it = _varsMap.find(key);
        if(it == _varsMap.end()) {
            return make_pair(false, shared_ptr<V>());
        }
        return make_pair(true, it->second);
    }
};

union StackVal {
    int64_t vInt;
    double vDouble;

    StackVal(int64_t vi): vInt(vi) {}
    StackVal(double vd): vDouble(vd) {}
};

class InterpContext: public VarsContext<uint16_t, StackVal> {
    shared_ptr<StackVal> _svPtr;
public:
    using VarsContext<uint16_t, StackVal>::VarsContext;

    void update(uint16_t varId, int64_t val) {
        StackVal* sv = new StackVal(val);
        _varsMap[varId] = shared_ptr<StackVal>(sv);
//        _svPtr = shared_ptr<StackVal>(new StackVal(val));
    }

    void update(uint16_t varId, double val) {
        StackVal* sv = new StackVal(val);
        _varsMap[varId] = shared_ptr<StackVal>(sv);
//        _svPtr = shared_ptr<StackVal>(new StackVal(val));
    }

//    MaybeV get(uint16_t const& key) const {
//        return MaybeV(true, _svPtr);
//    }
};


#endif // VARSCONTEXT_H
