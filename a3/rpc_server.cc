#include "rpc.h"
#include <iostream>

using namespace std;

int rpcInit() {
    char * binder_address = getenv ("BINDER_ADDRESS");
    char * binder_port = getenv("BINDER_PORT");

    if (binder_address == 0 || binder_port == 0) {
        cerr << "Error: SERVER_ADDRESS or SERVER_PORT is empty." << endl;
        exit(-1);
    }

    return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
    return 0;
}

int rpcExecute() {
    return 0;
}
