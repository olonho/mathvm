#ifndef VMSTACK_H
#define VMSTACK_H
#include <cstdio>
#include <stdint.h>
#include <algorithm>
using std::swap;
#include <cassert>

class vmStack
{
public:
    vmStack();
    ~vmStack();
    template<class T>
    void add(T var)
    {
        union {
            T val;
            uint8_t bits[sizeof(T)];
        } u;
        if (pos + sizeof(T) >= size){
            char* newmem = new char[size*2];
            swap(newmem, this->memory);
            delete[] newmem;
            size *= 2;
        }

        u.val = var;
        for (uint32_t i=0; i<sizeof(u.bits); i++) {
            memory[pos++] = u.bits[i];
        }
    }
    template<class T>
    T get(){
        union {
            T val;
            uint8_t bits[sizeof(T)];
        } u;
        assert(int(pos )- int(sizeof(T)) >= 0);

        for (uint32_t i=0; i<sizeof(u.bits); i++) {
            u.bits[i] = memory[pos - sizeof(T) + i];
        }
        pos -= sizeof(T);
        return u.val;
    }
private:
    size_t size;
    char* memory;
    size_t pos;
};

#endif // VMSTACK_H
