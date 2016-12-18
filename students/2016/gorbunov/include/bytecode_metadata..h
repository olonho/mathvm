#ifndef PROJECT_BYTECODE_METADATA_H
#define PROJECT_BYTECODE_METADATA_H

#include <map>
#include <string>

struct BytecodeMetadata {
private:
    std::map<std::string, uint32_t> _topmost_var_ids;

public:
    void addTopmostVar(const std::string& name, uint32_t id) {
        _topmost_var_ids[name] = id;
    }

    uint32_t getTopmostVarId(const std::string& name) {
        return _topmost_var_ids[name];
    }
};

#endif //PROJECT_BYTECODE_METADATA_H
