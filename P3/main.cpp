/*
 * prog2.cpp
 *
 *  Created on: Feb 20, 2017
 *      Author: gerardryan
 */
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
using namespace std;

#include "ParseNode.h"

int currentLine = 0;
int globalErrorCount = 0;

int
main(int argc, char *argv[])
{
    ifstream file;
    bool use_stdin = true;
    
    for( int i=1; i<argc; i++ ) {
        string arg = argv[i];
        
        if( use_stdin == false ) {
            cout << "Too many file names" << endl;
            return 1;
        }
        use_stdin = false;
        
        file.open(arg);
        if( file.is_open() == false ) {
            cout << "Could not open " << arg << endl;
            return 1;
        }
    }
    
    istream& in = use_stdin ? cin : file;

    ParseNode *program = Prog(in);
    
    if( program == 0 || globalErrorCount > 0 ) {
        cout << "Program failed!" << endl;
        return 1;
    }
    
//    ParseNode *right = program->rightNode();
//    cout << "\n" << right << "\n";
//    right = program->rightNode();
//    cout << "\n" << right;

    
    return 0;
}


