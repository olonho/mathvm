#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <stack>
#include <map>

template <typename T>
static T poptop(std::stack<T>& container)
{
    assert(!container.empty());

    T res = container.top();
    container.pop();
    return res;
}

template <typename T>
class mapInc
{
    std::map<T, uint16_t> mp;
    uint16_t lastNum = 0;

public:
    uint16_t operator[](T idx) {
        auto it = mp.find(idx);
        if (it == mp.end())
            return mp[idx] = lastNum++;
        return it->second;
    }
};

#endif // UTILS_HPP__
