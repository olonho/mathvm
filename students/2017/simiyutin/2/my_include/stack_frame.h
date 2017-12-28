#pragma once
#include <map>
#include "identity.h"

struct stack_frame {
    stack_frame(uint32_t executionPoint, int64_t function_id) :
            executionPoint(executionPoint),
            function_id(function_id)
    {};

    std::map<int, int64_t> & getVarMap(identity<int64_t>) {
        return intVarMap;
    };

    std::map<int, double> & getVarMap(identity<double >) {
        return doubleVarMap;
    };

    //map from variable to string id
    std::map<int, uint16_t> & getVarMap(identity<std::string>) {
        return stringVarMap;
    };

    uint32_t executionPoint;
    int64_t function_id;
    int64_t stack_size = 0;

private:
    std::map<int, int64_t> intVarMap;
    std::map<int, double> doubleVarMap;
    std::map<int, uint16_t> stringVarMap;
};
