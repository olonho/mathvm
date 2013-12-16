#include "errors.h"

error invalidBytecodeError(uint16_t fid, uint32_t ip) {
    std::stringstream msg;
    msg << "Error in function with id: " 
        << fid
        << "\nInstruction bytecode instruction in position: "
        << ip << std::endl;

    return error(msg.str());
}

error stackUnderFlowError(uint16_t fid, uint32_t ip) {
    std::stringstream msg;
    msg << "Error in function with id: "
        << fid 
        << "\nPop from empty stack. At position: "
        << ip << std::endl;

    return error(msg.str());
}

error notImplementedError(uint16_t fid, uint32_t ip) {
    std::stringstream msg;
    msg << "Error in function with id: "
        << fid
        << "\nOperation not implemented. At position: "
        << ip << std::endl;
    return error(msg.str());
}
