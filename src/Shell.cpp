#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <fcntl.h>
#include <regex>
#include <signal.h>


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

static void set_fd(int fd, int type) {
    close(type);    // maybe not needed, need to see doc
    dup2(fd, type);
    close(fd);
}

void Shell::visit(Commands* commands) {
    for (Command* c : commands->commands)
        c->accept(this);
}

// check for perms, set004 slide 25
void Shell::visit(In* in) {
    int fd = open(in->file.data(), O_RDONLY);
    set_fd(fd, STDIN_FILENO);
}

void Shell::visit(Out* out) {
    int perms = O_WRONLY|O_CREAT|(out->append ? O_APPEND : O_TRUNC);
    int fd = open(out->file.data(), perms);
    set_fd(fd, STDOUT_FILENO);
}

// Reap zombies
void handle_sigchld(int signum) {
    signal(SIGCHLD, handle_sigchld);
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void Shell::visit(BasicCommand* bc) {

    // handle special commands
    if (strcmp(bc->argv[0], "exit") == 0) {
        cont = false;
        return ;
    }
    else if (strcmp(bc->argv[0], "history") == 0) {
        if (bc->argv[1]) {

            if (!std::regex_match(bc->argv[1], std::regex(R"([0-9]*)")))
                throw Exception("use: history <positive int>?");

            size_t num = std::stoi(bc->argv[1]);

            if (num > last || num < 1)
                throw Exception("history: out of range");
            
            parse_run(history[num - 1]);

            return ;
        }

        // printf("last %ld\n", last);
        for (size_t i = 0, j = 1; i < MAX_HISTORY; i++) {
            if (history[(last + i) % MAX_HISTORY].compare("") != 0)
                printf("[%2ld] %s", j++, history[(last + i) % MAX_HISTORY].data());
        }

        return ;
    }
    // else if (strcmp(bc->argv[0], "cd") == 0)
    // else if (strcmp(bc->argv[0], "createalias") == 0)
    // else if (strcmp(bc->argv[0], "destroyalias") == 0)

    signal(SIGCHLD, handle_sigchld);

    // check for alias and parse_run(replaced);

    pid_t pid; 
    FORK(pid);
  
    if (pid == 0) {

        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        if (bc->io[0] != STDIN_FILENO) 
            set_fd(bc->io[0], STDIN_FILENO);
        
        if (bc->io[1] != STDOUT_FILENO)
            set_fd(bc->io[1], STDOUT_FILENO);

        for (IO* io : bc->ios) 
            io->accept(this);

        if (execvp(bc->argv[0], bc->argv) < 0) 
            perror("exec() failed");

        exit(0);
    }

    if (!bc->bg) {
        running = pid;
        
        int temp;
        do {
            waitpid(pid, &temp, WUNTRACED);
        } while (!WIFEXITED(temp) && !WIFSIGNALED(temp) && !WIFSTOPPED(temp));
    }
}

void Shell::visit(Pipe* pipe_) {
    int in = STDIN_FILENO;
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

    // maybe change - past roodys had an idea that current roodys doesn't remember
    if (in != STDIN_FILENO) 
        last->io[0] = in;

    last->accept(this);
}


Shell::Shell() : parser(Parser()), cont(true), completed(false), last(0) { 
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    
    for (size_t i = 0; i < MAX_HISTORY; i++)
        history[i] = "";
}

Shell::~Shell() {
}

string Shell::read() {
    string str;
    std::getline(std::cin, str);
    return str + "\n";
}

void Shell::parse_run(string input) {
    size_t lastb = last;

    Node* root = NULL;

    try {
        root = parser.parse(input.data());
        root->accept(this);
        delete root;
    }
    catch (Exception& e) {
        delete root;
        printf("%s\n", e.what());
    }

    if (last == lastb) {
        history[last++] = input;
        last %= MAX_HISTORY;
        completed = completed || last == 0;
    }
}

void Shell::execute() {

    while(cont) {
        fflush(stdin);
        printf("\nin-mysh-now:> ");

        string input = read();
        parse_run(input);
    }
}


