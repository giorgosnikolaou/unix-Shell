#pragma once
#include <vector>
#include <string>

#define vector std::vector
#define string std::string

class Node;
class Commands;
class In;
class Out;
class BasicCommand;
class Pipe;

class Visitor {
    public:
        virtual ~Visitor();
        virtual void visit(Node* node);
        virtual void visit(Commands* commands);
        virtual void visit(In* in);
        virtual void visit(Out* out);
        virtual void visit(BasicCommand* bc);
        virtual void visit(Pipe* pipe);
};

class Node {
    private:

    public:
        virtual ~Node();
        virtual void accept(Visitor* v);

        virtual void print();
};

class Command : public Node {
    private:
    public:
        virtual ~Command();
        virtual void accept(Visitor* v) = 0;
};

class Commands : public Node {
    private:
    public:
        vector<Command*> commands;

        ~Commands();
        void accept(Visitor* v);
        void add(Command* command);
        virtual void print();
};

class IO : public Node {
    private:
    public:
        string file;

        IO(string file_);
        void set_file(string file_);
        virtual void accept(Visitor* v) = 0;
};

class In : public IO {
    private:
    public:
        In(string file = "");
        void accept(Visitor* v);
        virtual void print();
};

class Out : public IO {
    private:
    public:
        bool append;
        Out(bool append_ = false, string file = "");
        void set_append(bool append_);
        void accept(Visitor* v);
        virtual void print();
};

class BasicCommand : public Command {
    private:
        string executable;
        vector<string> args;
    public:
        char** argv;
        vector<IO*> ios;  
        bool bg;
        int io[2];

        BasicCommand(string executable_ = "", bool bg_ = false);
        ~BasicCommand();

        void set_exe(string executable_);
        void set_bg(bool bg_);
        void add_arg(string arg);
        void add_io(IO* io);

        void build();
        
        void accept(Visitor* v);
        virtual void print();
};

class Pipe : public Command {
    private:
    public:
        vector<BasicCommand*> commands;

        Pipe(BasicCommand* left, BasicCommand* right);
        ~Pipe();
        void add(BasicCommand* command);
        void accept(Visitor* v);
        virtual void print();
};