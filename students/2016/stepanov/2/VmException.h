#ifndef VM_VMEXCEPTION_H
#define VM_VMEXCEPTION_H

#include <stdexcept>

namespace mathvm {

    class VmException : public std::logic_error {
    private:
        uint32_t position;
    public:
        uint32_t getPosition() const {
            return position;
        }

        VmException(const char *what, uint32_t position) : std::logic_error(what), position(position) { }
        VmException(const std::string &what, uint32_t position) : std::logic_error(what), position(position) { }
    };
}


#endif //VM_VMEXCEPTION_H
