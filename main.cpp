#include <iostream>
#include <unistd.h>
#include "Shell.hpp"

using namespace std;


int main() {

    Shell sh = Shell();
    sh.execute();

    return 0;
}