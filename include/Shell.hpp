#pragma once

#include "Parser.hpp"


class ShellVisitor : public Visitor {
    private:
        void run(vector<IO*> ios, char* const * argv);
    public:
        virtual void visit(Commands* commands);
        virtual void visit(In* in);
        virtual void visit(Out* out);
        virtual void visit(BasicCommand* bc);
        virtual void visit(Pipe* pipe);
};

class Shell {
    private:
        Parser parser;
        Visitor* visitor;

        // history
        // aliases

        string read();
    public:
        Shell();
        ~Shell();

        bool execute();

};