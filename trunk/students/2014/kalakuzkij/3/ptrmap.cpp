#include "ptrmap.h"
using std::map;
#include <cassert>

ptrMap::ptrMap():_map(), _next_id(0)
{

}
const char * ptrMap::get(int id){
    if (_map.find(id) != _map.end()){
        return _map[id];
    }
    assert(0);
    return 0;
}

unsigned int ptrMap::add(const char * ptr){
    if (_bmap.find(ptr) != _bmap.end()){
        return _bmap[ptr];
    }
    _map[_next_id] = ptr;
    _bmap[ptr] = _next_id;
    return _next_id++;
}
