#ifndef PTRMAP_H
#define PTRMAP_H

#include <map>

class ptrMap
{
public:
    ptrMap();
    const char * get(int id);
    unsigned int add(const char *);
private:
    std::map<unsigned int, const char *> _map;
    std::map<const char *, unsigned int> _bmap;
    unsigned int _next_id;
};

#endif // PTRMAP_H
