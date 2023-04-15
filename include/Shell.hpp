#pragma once

#include "Parser.hpp"
#include <map>

#define MAX_HISTORY 20
#define map std::map

class Shell : public Visitor {
    private:
        void visit(Commands* commands);
        void visit(In* in);
        void visit(Out* out);
        void visit(BasicCommand* bc);
        void visit(Pipe* pipe);

        Parser parser;
        
        bool cont;

        map<string, Node*> aliases;
        
        bool completed;
        size_t last;
        string history[MAX_HISTORY];
        void parse_run(string input);

        pid_t running;

        string read();

    public:
        Shell();
        ~Shell();

        void execute();

};