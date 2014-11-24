#include "vmstack.h"
#include <cstring>


vmStack::vmStack():
    size(100),
    memory(new char[size]),
    pos(0)
{

}

vmStack::~vmStack(){
    delete[] memory;
}



