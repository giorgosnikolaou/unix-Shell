#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <fcntl.h>


#include "Shell.hpp"

#define FORK(pid)                   \
    if ((pid = fork()) < 0) {       \
        perror("fork() failed");    \
        exit(EXIT_FAILURE);         \
    }                               \

#define CPIPE(pipeid)               \
    if (pipe(pipeid) < 0) {         \
        perror("pipe() failed");    \
        exit(EXIT_FAILURE);         \
    }                               \

void ShellVisitor::visit(Commands* commands) {
    for (Command* c : commands->commands)
        c->accept(this);
}

static void set_fd(int fd, int type) {
    close(type);
    dup2(fd, type);
    close(fd);
}

void ShellVisitor::visit(In* in) {
    int fd = open(in->file.data(), O_RDONLY);
    set_fd(fd, STDIN_FILENO);
}

void ShellVisitor::visit(Out* out) {
    int perms = out->append ? O_WRONLY|O_APPEND|O_CREAT : O_WRONLY|O_TRUNC|O_CREAT;
    int fd = open(out->file.data(), perms);
    set_fd(fd, STDOUT_FILENO);
}

void ShellVisitor::run(vector<IO*> ios, char* const * argv) {
    for (IO* io : ios) 
        io->accept(this);

    if (execvp(argv[0], argv) < 0) 
        fprintf(stderr, "exec() failed\n");
}

void ShellVisitor::visit(BasicCommand* bc) {

    // handle special commands
    if (strcmp(bc->argv[0], "exit") == 0) 
        throw Exception("exit");
    // else if (bc->executable.compare("cd") == 0)
    // else if (bc->executable.compare("cd") == 0)


    // Forking a child
    pid_t pid; 
    FORK(pid);
  
    if (pid == 0) {

        if (bc->io[0] != STDIN_FILENO) 
            set_fd(bc->io[0], STDIN_FILENO);
        
        if (bc->io[1] != STDOUT_FILENO)
            set_fd(bc->io[1], STDOUT_FILENO);

        run(bc->ios, bc->argv);

        exit(0);
    }
    else if (!bc->bg) {
        wait(NULL);
        
        // int temp;
        // do {
        //     waitpid(pid, &temp, WUNTRACED);
        // } while (!WIFEXITED(temp) && !WIFSIGNALED(temp));
        
        return;
    }
}

void ShellVisitor::visit(Pipe* pipe_) {
    int in = 0;
    int pipeid[2]; 

    for (size_t i = 0; i + 1 < pipe_->commands.size(); i++) {
        CPIPE(pipeid);

        pipe_->commands[i]->io[0] = in;
        pipe_->commands[i]->io[1] = pipeid[1];
        pipe_->commands[i]->accept(this);

        close(pipeid[1]);
        in = pipeid[0];
    }

    BasicCommand* last = pipe_->commands.back();   

    if (in > 0) 
        last->io[0] = in;

    last->accept(this);
}


Shell::Shell() : parser(Parser()), visitor(new ShellVisitor()) {

}

Shell::~Shell() {
    delete visitor;
}

string Shell::read() {
    string str;
    std::getline(std::cin, str);
    return str + "\n";
}

bool Shell::execute() {
    fflush(stdin);
    printf("\nin-mysh-now:> ");

    string input = read();
    
    Node* root = NULL;

    try {
        root = parser.parse(input.data());
    }
    catch (Exception& e) {
        printf("%s\n", e.what());
        delete root;
        return true;
    }

    // root->print();

    try {
        root->accept(visitor);
        delete root;
    }
    catch (Exception& e) {
        delete root;
        if (strcmp(e.what(), "exit") == 0)
            return false;

        printf("%s\n", e.what());

    }

    // save in history

    return true;
}


