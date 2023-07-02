#pragma once

#include <map>
#include "Parser.hpp"

#define MAX_HISTORY 20
#define map std::map

class Shell : public Visitor {
    private:
        void visit(Commands* commands);
        void visit(In* in);
        void visit(Out* out);
        void visit(BasicCommand* bc);
        void visit(SubPipe* sp);
        void visit(Pipe* pipe);

        Parser parser;
        
        bool cont;

        map<string, string> aliases;
        
        bool completed;
        size_t start;
        size_t end;
        size_t count;
        string history[MAX_HISTORY];
        void parse_run(string input, string hist = "");

        string read();
        void add_history(string c);
        bool check_custom(BasicCommand* bc);

        pid_t shell_pid;


    public:
        Shell();
        ~Shell();

        void execute();

};