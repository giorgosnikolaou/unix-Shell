#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <fcntl.h>
#include <regex>
#include <signal.h>
#include <pwd.h>

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

#define ASSERT1(call, arg)                  \
    if (call(arg) != 0)                     \
        perror(#call "(" #arg ") failed");  \

#define ASSERT2(call, arg1, arg2)                       \
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

void Shell::visit(In* in) {
    int fd = open(in->file.data(), O_RDONLY);
    set_fd(fd, STDIN_FILENO);
}

void Shell::visit(Out* out) {
    int perms = O_WRONLY|O_CREAT|(out->append ? O_APPEND : O_TRUNC);
    int fd = open(out->file.data(), perms, 0666);
    set_fd(fd, STDOUT_FILENO);
}

static void wait_on_pid(int pid) {        
    int temp;
    do {
        waitpid(pid, &temp, WUNTRACED);
    } while (!WIFEXITED(temp) && !WIFSIGNALED(temp) && !WIFSTOPPED(temp));
}

bool Shell::check_custom(BasicCommand* bc) {
    
    if (strcmp(bc->argv[0], "exit") == 0) {
        cont = false;
        return true;
    }
    else if (strcmp(bc->argv[0], "history") == 0) {
        if (bc->argv[1]) {

            if (bc->argv[2] || !std::regex_match(bc->argv[1], std::regex(R"([0-9]*)"))) 
                throw Exception("Usage: history <positive int>?");

            size_t num = std::stoi(bc->argv[1]);

            if (num > count || num < 1)
                throw Exception("history: out of range");
            
            parse_run(history[num - 1]);

            return true;
        }

        for (size_t i = 0; i < count; i++) {
            if (history[(start + i) % MAX_HISTORY].compare("") != 0) 
                printf("[%2ld] %s", i + 1, history[(start + i) % MAX_HISTORY].data());
        }

        add_history("history");
        
        return true;
    }
    else if (strcmp(bc->argv[0], "cd") == 0) {       
        char* path = bc->argv[1];

        if (!bc->argv[1] || strcmp(bc->argv[1], "~") == 0) {
            if (!(path = getenv("HOME"))) 
                path = getpwuid(getuid())->pw_dir;
        }

        ASSERT1(chdir, path);

        return true;
    }
    else if (strcmp(bc->argv[0], "createalias") == 0) {
        if (!bc->argv[1] || !bc->argv[2] || bc->argv[3]) 
            throw Exception("Usage: createalias <alias> <command>\n");
        
        if (aliases.count(bc->argv[1]) == 0) 
            aliases.insert({bc->argv[1], bc->argv[2]});
        else
            aliases[bc->argv[1]] = bc->argv[2]; 

        printf("Alias created succesfully!\n");
        return true;
    }
    else if (strcmp(bc->argv[0], "destroyalias") == 0) {
        if (!bc->argv[1] || bc->argv[2])
            throw Exception("Usage: destroyalias <alias>\n");
        
        if (aliases.erase(bc->argv[1]))
            printf("Alias destroyed succesfully!\n");
        else
            printf("Alias %s doesn't exist!\n", bc->argv[1]);
        return true;
        
    }

    
    if (aliases.count(bc->argv[0])) {
        string to_run = aliases.at(bc->argv[0]);
        for (size_t i = 1; bc->argv[i]; i++)
            to_run += bc->argv[i];

        to_run += "\n";

        parse_run(to_run, bc->argv[0]);

        return true;
    }

    return false;
}

void Shell::visit(BasicCommand* bc) {

    if (check_custom(bc))
        return ;

    pid_t pid; 
    FORK(pid);

    if (pid != 0)
        ASSERT2(setpgid, pid, pid);

    if (pid == 0) {
        
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
            
        for (IO* io : bc->ios) 
            io->accept(this);

        if (execvp(bc->argv[0], bc->argv) < 0) 
            perror("exec() failed");

        exit(0);
    }

    if (!bc->bg) {
        ASSERT2(tcsetpgrp, 0, pid);
        wait_on_pid(pid);
    }
    else
        printf("[1] %d\n", pid);
    
    
}

void Shell::visit(SubPipe* sp) {
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
        pipe_->set_pid(pid[0]);
    
    ASSERT2(setpgid, pid[0], pipe_->pid);

    if (pid[0] == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        close(pipeid[STDIN_FILENO]);
        set_fd(pipeid[1], STDOUT_FILENO);
        pipe_->c1->accept(this);
        exit(0);
    }

    FORK(pid[1]);
    
    ASSERT2(setpgid, pid[1], pipe_->pid);

    if (pid[1] == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        close(pipeid[STDOUT_FILENO]);
        set_fd(pipeid[0], STDIN_FILENO);
        pipe_->c2->accept(this);
        exit(0);
    }

    close(pipeid[0]);
    close(pipeid[1]);


    if (!pipe_->bg) {
        if (shell_pid == getpid())
            ASSERT2(tcsetpgrp, 0, pipe_->pid);

        wait_on_pid(-pipe_->pid);
        wait_on_pid(-pipe_->pid);
    }
    else
        printf("[1] %d\n", pipe_->pid);
}


Shell::Shell() : parser(Parser()), cont(true), completed(false), start(0), end(MAX_HISTORY - 1), count(0), shell_pid(getpid()) { 

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    ASSERT2(setpgid, 0, 0);
    ASSERT2(tcsetpgrp, 0, getpid());    
        
    for (size_t i = 0; i < MAX_HISTORY; i++)
        history[i] = "";
}


static vector<pid_t> get_children(pid_t pid) {
    string path = "/proc/" + std::to_string(pid) + "/task/" + std::to_string(pid) + "/children";

    FILE* fp = fopen(path.data(), "r");
    
    if (!fp)
        throw Exception(path + " does not exist!");

    vector<pid_t> children;
    int cpid;
    while (fscanf(fp, "%d", &cpid) != EOF) 
        children.push_back(cpid);

    fclose(fp);

    return children;
}

static void kill_children(pid_t pid) {
    for (pid_t& child : get_children(pid)) {
        kill_children(child);
        kill(child, 9);
    }
}

Shell::~Shell() {
    kill_children(getpid());
}

string Shell::read() {
    string str;
    std::getline(std::cin, str);
    return str + "\n";
}


void Shell::add_history(string c) {
    if (count == MAX_HISTORY)
        start++;
    else
        count++;

    history[end] = c;

    end = (end + 1) % MAX_HISTORY;
}

void Shell::parse_run(string input, string hist) {
    size_t lastc = count;

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

    if (hist.compare("") != 0)
        add_history(hist + "\n");
    else if (count == lastc) 
        add_history(input);
}

void Shell::execute() {

    while(cont) {
        fflush(stdin);
        printf("\nin-mysh-now:> ");

        string input = read();

        parse_run(input);

        ASSERT2(tcsetpgrp, 0, getpid()) 
    }
}


