#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    cout<<"smash: got ctrl-C"<<endl;
    pid_t foreground=SmallShell::getInstance().getForeground();
    if (foreground!=0){
        kill(foreground,SIGINT);
        cout<<"smash: process "<<foreground<<" was killed"<<endl;
    }
}
