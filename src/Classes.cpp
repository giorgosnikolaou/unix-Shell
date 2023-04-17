#include "Classes.hpp"
#include <unistd.h>


Visitor::~Visitor() {

}

void Visitor::visit(Node* node) {
    
}

void Visitor::visit(Commands* commands) {
    
}

void Visitor::visit(In* in) {
    
}

void Visitor::visit(Out* out) {
    
}

void Visitor::visit(BasicCommand* bc) {
    
}

void Visitor::visit(SubPipe* sp) {
    
}

void Visitor::visit(Pipe* pipe) {
    
}


Node::~Node() {};
void Node::accept(Visitor* v) {
    v->visit(this);
}
void Node::print() {
    fprintf(stderr, "Node\n");
}


Command::~Command() {};


Commands::~Commands() {
    for (Command* c : commands)
        delete c;
}

void Commands::accept(Visitor* v) {
    v->visit(this);
}

void Commands::add(Command* command) {
    commands.push_back(command);
}

void Commands::print() {
    for (Command* c : commands) {
        c->print();
        fprintf(stderr, "\n");
    }
}


IO::IO(string file_) : file(file_) {};
void IO::set_file(string file_) { 
    file = file_;
}


In::In(string file) : IO(file) {};
void In::accept(Visitor* v) {
    v->visit(this);
}
void In::print() {
    fprintf(stderr, "< %s ", file.data());
}

IO* In::copy() {
    return new In(*this);
}

Out::Out(bool append_, string file) : IO(file), append(append_) {};
void Out::set_append(bool append_) { append = append_; }
void Out::accept(Visitor* v) {
    v->visit(this);
}
void Out::print() {
    fprintf(stderr, "%s %s ", append ? ">>" : ">", file.data());
}

IO* Out::copy() {
    return new Out(*this);
}


BasicCommand::BasicCommand(string executable_) : executable(executable_), io{STDIN_FILENO, STDOUT_FILENO} { bg = false; }
BasicCommand::~BasicCommand() {
    for (IO* io : ios)
        delete io;
    
    for (size_t i = 0; argv[i]; i++)
        delete [] argv[i];
    
    delete [] argv;
}

void BasicCommand::set_exe(string executable_) { executable = executable_; }

void BasicCommand::set_bg(bool bg_) { bg = bg_; }

void BasicCommand::add_arg(string arg) {
    args.push_back(arg);
}

void BasicCommand::add_io(IO* io) {
    ios.push_back(io);
}

void BasicCommand::build() {
    argv = new char*[args.size() + 2];
    argv[0] = new char[executable.length() + 1];
    sprintf(argv[0], "%s", executable.data());

    for (size_t i = 0; i < args.size(); i++) {
        argv[i+1] = new char[args[i].length() + 1];
        sprintf(argv[i+1], "%s", args[i].data());
    }

    argv[args.size() + 1] = NULL;
}

void BasicCommand::accept(Visitor* v) {
    v->visit(this);
}
void BasicCommand::print() {
    fprintf(stderr, "%s ", executable.data());
    for (string arg : args)
        fprintf(stderr, "||%s|| ", arg.data());
    
    for (IO* io : ios)
        io->print();
    
    // change this, Command::print(), put it there and also put this to Pipe::print
    fprintf(stderr, "%s", bg ? "&" : "");
}

SubPipe::SubPipe(BasicCommand* bc) : BasicCommand(*bc) { 
    argv = new char*[args.size() + 2];
    argv[0] = new char[executable.length() + 1];
    sprintf(argv[0], "%s", executable.data());

    for (size_t i = 0; i < args.size(); i++) {
        argv[i+1] = new char[args[i].length() + 1];
        sprintf(argv[i+1], "%s", args[i].data());
    }

    argv[args.size() + 1] = NULL;
    
    for (IO* io : bc->ios) 
        ios.push_back(io->copy());
}

void SubPipe::accept(Visitor* v) {
    v->visit(this);
}

Pipe::Pipe(BasicCommand* left, Command* right) : c1(new SubPipe(left)), c2(right) { delete left; bg = false; }
Pipe::~Pipe() { 
    delete c1;
    delete c2;
}

void Pipe::accept(Visitor* v) {
    v->visit(this);
}
void Pipe::print() {
    c1->print();
    fprintf(stderr, " | ");
    c2->print();
    
}
