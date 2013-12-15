#include <string>
#include <sstream>
#include <stdexcept>

// Exception class for type errors etc...
class error : public std::exception {
    std::string _msg;

public:
    explicit error(const std::string& msg): _msg(msg) {}
    explicit error(const char* msg): _msg(msg) {}

    virtual ~error() throw () {}

    const char* what() const throw () {
        return _msg.c_str();
    }
};

error invalidBytecodeError(const std::string & funcName, uint32_t ip) {
    std::stringstream msg;
    msg << "Invalid bytecode instruction at function: " 
        << funcName
        << ". Instruction position: "
        << ip << std::endl;

    return error(msg.str());
}

error stackUnderFlowError(uint32_t ip) {
    std::stringstream msg;
    msg << "Pop from empty stack. At position: "
        << ip << std::endl;

    return error(msg.str());
}

