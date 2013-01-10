/* 
 * File:   main.cpp
 * Author: yarik
 *
 * Created on October 1, 2012, 1:11 PM
 */

#include <string>

#include <fstream>
#include <streambuf>
#include <iostream>
#include <parser.h>

#include "Translator.h"
#include "Interpreter.h"



using std::cin;
using std::cout;
using std::endl;

using std::string;

using namespace mathvm;



/*
 * 
 */
int main(int argc, char** argv) {

	if (argc < 2) {
    	cout << "Enter source file" << endl;
    	return -1;
    }


	std::ifstream t(argv[1]);
	string code((std::istreambuf_iterator<char>(t)),
			         std::istreambuf_iterator<char>());
    Parser parser;
 	Status* status = parser.parseProgram(code);

	if (status)
	{
	    if (status->isError())
	    {
	         cout << "Parser.parseProgram: status is an error" << endl;
	         cout << status->getError() << endl;
	         delete status;
	         return -1;
	     }
	     else
	         cout << "Parse OK!" << endl;
	}
	else
	{
	    //cout << "Parser.parseProgram: returned null status" << endl;
	    //cout << "Suprisingly, It's OK!" << endl;
	}
	delete status;

    
    Code* bc = 0;


    TranslatorVisitor translator(&bc);

    translator.translate(parser.top());


    Interpreter executor(cout);


    executor.execute(bc);





    //delete bcode;


    return 0;
}

