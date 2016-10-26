#include <iostream>
#include "translator_impl.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cerr << "Usage: formatter [filename]";
        return 1;
    }
    try
    {
        cout << FormatDocument(argv[1]);
    }
    catch (runtime_error &e)
    {
        cerr << e.what();
        return 1;
    }

    return 0;
}