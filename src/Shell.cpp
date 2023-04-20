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

#define ASSERT(call, arg1, arg2)                        \
    if (call(arg1, arg2) != 0) {                        \
        perror(#call "(" #arg1 ", " #arg2 ") failed");  \
        exit(0);                                        \
    }                                                   \

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
    int fd = open(out->file.data(), perms, 0666);
    set_fd(fd, STDOUT_FILENO);
}

// Reap zombies
void handle_sigchld(int signum) {
    signal(SIGCHLD, handle_sigchld);
    while (waitpid(0, NULL, WNOHANG) > 0);
        // fprintf(stderr, "hehehehehe\n");
}

static void wait_on_pid(int pid) {        
    int temp;
    do {
        waitpid(pid, &temp, WUNTRACED);
    } while (!WIFEXITED(temp) && !WIFSIGNALED(temp) && !WIFSTOPPED(temp));
}


// static char* custom[5] = { "exit", "history", "cd", "createalias", "destroyalias" }; 
bool Shell::check_custom(BasicCommand* bc) {
    
    if (strcmp(bc->argv[0], "exit") == 0) {
        cont = false;
        return true;
    }
    else if (strcmp(bc->argv[0], "history") == 0) {
        if (bc->argv[1]) {

            if (!std::regex_match(bc->argv[1], std::regex(R"([0-9]*)")))
                throw Exception("use: history <positive int>?");

            size_t num = std::stoi(bc->argv[1]);

            if (num > last || num < 1)
                throw Exception("history: out of range");
            
            parse_run(history[num - 1]);

            exit(0);

            return true;
        }

        for (size_t i = 0, j = 1; i < MAX_HISTORY; i++) {
            if (history[(last + i) % MAX_HISTORY].compare("") != 0)
                printf("[%2ld] %s", j++, history[(last + i) % MAX_HISTORY].data());
        }
        
        return true;
    }
    // else if (strcmp(bc->argv[0], "cd") == 0)
    // else if (strcmp(bc->argv[0], "createalias") == 0)
    // else if (strcmp(bc->argv[0], "destroyalias") == 0)

    return false;
}

void Shell::visit(BasicCommand* bc) {

    if (check_custom(bc))
        return ;

    // check for alias and parse_run(replaced);

    pid_t pid; 
    FORK(pid);

    ASSERT(setpgid, pid, pid);
  
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
            
        for (IO* io : bc->ios) 
            io->accept(this);

        // fprintf(stderr, "pid of child: %d\n", getpid());

        if (execvp(bc->argv[0], bc->argv) < 0) 
            perror("exec() failed");

        exit(0);
    }


    // Parent 
    if (!bc->bg) {
        ASSERT(tcsetpgrp, 0, pid);
        wait_on_pid(pid);
    }
    
    
}

void Shell::visit(SubPipe* sp) {
    // put custom commands here, make function maybe?

    if (check_custom(sp))
        return ;

    for (IO* io : sp->ios) 
        io->accept(this);

    if (execvp(sp->argv[0], sp->argv) < 0) 
        perror("exec() failed");

    exit(0);
    
}

void Shell::visit(Pipe* pipe_) {

    int pid[2]; 
    int pipeid[2]; 
    CPIPE(pipeid);

    FORK(pid[0]);

    if (pipe_->pid == -1)
        pipe_->pid = pid[0];

    ASSERT(setpgid, pid[0], pipe_->pid);

    if (pid[0] == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        close(pipeid[0]);
        set_fd(pipeid[1], STDOUT_FILENO);
        pipe_->c1->accept(this);
        exit(0);
    }

    FORK(pid[1]);
    
    ASSERT(setpgid, pid[1], pipe_->pid);

    if (pid[1] == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        close(pipeid[1]);
        set_fd(pipeid[0], STDIN_FILENO);
        pipe_->c2->accept(this);
        exit(0);
    }

    close(pipeid[0]);
    close(pipeid[1]);


    if (!pipe_->bg) {
        ASSERT(tcsetpgrp, 0, pipe_->pid);
        wait_on_pid(-pipe_->pid);
        wait_on_pid(-pipe_->pid);
    }
}

// dd if=/dev/urandom | base64 | head -10000 | tail -1


Shell::Shell() : parser(Parser()), cont(true), completed(false), last(0) { 

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGCHLD, handle_sigchld);

    signal(SIGTTIN,SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    ASSERT(setpgid, 0, 0);
    ASSERT(tcsetpgrp, 0, getpid());    
        
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
        // root->print();
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



        ASSERT(tcsetpgrp, 0, getpid()) 
        
    }
}


