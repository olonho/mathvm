#include <iostream>
#include <string>
#include <cstdio>

using namespace std;


int main(int argc, char* argv[]) {
    for(int i = 0; i < argc; ++i)
        cout << argv[i] << endl;
    return 0;
} 
