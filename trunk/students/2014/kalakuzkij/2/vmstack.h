#ifndef VMSTACK_H
#define VMSTACK_H
#include <cstdio>
#include <stdint.h>
#include <algorithm>
using std::swap;
#include <cassert>
#include <cstring>

//debug stuff
#include <vector>
#include "mathvm.h"
#include <iostream>

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
            memcpy(newmem, this->memory, size);
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


    //todo delete it
    // DEBUG stuf
    void print(const std::vector<mathvm::VarType>& types){
        int k = 0;
        for (uint i = 0; i < types.size(); ++i){
            switch (types[i]){
            case mathvm::VT_INT:{
                int64_t v = at<int64_t>(k);
                std::cout << "i:" <<v <<";";
                k+= sizeof(int64_t);

                break;
            }
            case mathvm::VT_STRING:{
                int64_t v = at<int16_t>(k);
                std::cout << "s:" <<v <<";";
                k+= sizeof(int16_t);
                break;
            }
            case mathvm::VT_DOUBLE:{
                int64_t v = at<double>(k);
                std::cout << "d:" <<v <<";";
                k+= sizeof(double);
                break;
            }
            default:
                break;
            }

        }
        std::cout << std::endl;
    }
private:
    template<class T>
    T at(int offset){
        union {
            T val;
            uint8_t bits[sizeof(T)];
        } u;
        assert(int(pos - offset)- int(sizeof(T)) >= 0);

        for (uint32_t i=0; i<sizeof(u.bits); i++) {
            u.bits[i] = memory[pos - offset - sizeof(T) + i];
        }
        return u.val;
    }
    size_t size;
    char* memory;
    size_t pos;
};

#endif // VMSTACK_H
