#include "rpc.h"
#include <iostream>

using namespace std;

static int

int rpcInit() {
    char *binder_address = getenv("BINDER_ADDRESS");
    char *binder_port = getenv("BINDER_PORT");

    if (binder_address == 0 || binder_port == 0) {
        cerr << "Error: SERVER_ADDRESS or SERVER_PORT is empty." << endl;
        return -1;
    }

    //-----------------------------------------
    int serverSocket;
    struct hostent *host;
    struct sockaddr_in siServer;

    // create client socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Cannot create socket" << endl;
        return -1;
    }

    // set client info, port is implicitly set to 0 by memset
    memset((char *)&siServer, 0, sizeof(siServer));
    siServer.sin_family = AF_INET;
    siServer.sin_port = htons(atoi(binder_port));

    // connect to server
    host = gethostbyname(binder_address);
    if (!host) {
        cout << "could not resolve hostname!" << endl;
        return -1;
    }

    memcpy((void *)&siServer.sin_addr, host->h_addr_list[0], host->h_length);

    if (connect(serverSocket, (struct sockaddr *)&siServer, sizeof(siServer)) < 0) {
        cerr << "Connection Failed" << endl;
        return -1;
    }
    //-----------------------------------------

    int listenSocket;
    struct sockaddr_in siListen;

    // create client socket
    if ((listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Cannot create socket" << endl;
        return -1;
    }

    // set client info, port is implicitly set to 0 by memset
    memset((char *)&siListen, 0, sizeof(siListen));
    siListen.sin_family = AF_INET;
    siListen.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket to port
    if ((bind(listenSocket, (const struct sockaddr*)&siListen, sizeof(struct sockaddr_in))) < 0) {
        cerr << "Bind failed" << endl;
        return -1;
    }

    // listen max 128 clients
    if (listen(listenSocket, 128) < 0) {
        cerr << "lisen failed" << endl;
        return -1;
    }

    return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
    return 0;
}

int rpcExecute() {
    return 0;
}
