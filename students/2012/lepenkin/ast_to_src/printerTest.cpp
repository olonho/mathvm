/* 
 * File:   main.cpp
 * Author: yarik
 *
 * Created on October 1, 2012, 1:11 PM
 */

#include <iostream>
#include <string>
#include "PrinterVisitor.h"

#include <fstream>
#include <streambuf>



using std::cin;
using std::cout;
using std::endl;

using std::string;

/*
 * 
 */
int main(int argc, char** argv) {
    
    PrinterVisitor ast_printer;
    
    if (argc > 1) {
        std::ifstream t(argv[1]);
        string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
        ast_printer.print(str);
        
    } else 
    {
        cout << "Enter source file" << endl;
        return -1;
    }
    
    
    
    return 0;
}

