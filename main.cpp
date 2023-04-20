#include <unistd.h>
#include <sys/wait.h>
#include "Shell.hpp"


int main() {
    Shell sh = Shell();
    sh.execute();

    return 0;
}