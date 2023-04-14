#include <iostream>
#include "Shell.hpp"

using namespace std;

// string get_str(string word) {
//     // assert balanced non escaped quotes
//     // replace enviroment vars
//     // expand wildchars where needed
//     // return string seperated with spaces
// }

int main() {

    // Scanner s = Scanner();

    // s.scan("my_exec<a\n");
    // s.print();

    // s.scan("my_exec<a | exec2 ; ls -a;\n");
    // s.print();

    // s.scan("my_exec \"hello \\ \\* \\\" there|mate\"\n");
    // s.scan(R"(hello < a " \\" ")");
    // s.print();

    // s.scan("my_exec<a\n");
    // s.print();

    // s.scan("my_exec<a\n");
    // s.print();

    // Parser parser = Parser();

    // Node* node = parser.parse("exe > file1 aa \\\"./src/*.cpp\\\" < file2 >> file3 arg '-\\ \" * mate' & | grep -l | cat   ; main giorgos nikolaou $USER\"$USER*\";main ./src/*.cpp\n");
    // // Node* node = parser.parse("exe > file1 aa \\\"a*\\\"*.txt < file2 \n");
    
    // node->print();
    // delete node;

    Shell sh = Shell();

    while(sh.execute());

    return 0;

}