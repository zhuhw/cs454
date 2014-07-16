#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rpc.h"
#include "common.h"

using namespace std;

static int serverSocket;
static int listenSocket;
static map<struct ProcedureSignature, skeleton> skeletonMap;

int rpcInit() {
    char *binder_address = getenv("BINDER_ADDRESS");
    char *binder_port = getenv("BINDER_PORT");

    if (binder_address == 0 || binder_port == 0) {
        cerr << "Error: SERVER_ADDRESS or SERVER_PORT is empty." << endl;
        return -1;
    }

    //-----------------------------------------
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

    // cout << "rpcInit done" << endl;
    return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
    // cout << "rpcRegister start" << endl;
    // prepare message content to be sent
    char hostname[128];
    gethostname(hostname, 128);

    struct sockaddr_in siServer;
    socklen_t siLen = sizeof(siServer);
    getsockname(serverSocket, (struct sockaddr *)&siServer, &siLen);
    unsigned short portNum = ntohs(siServer.sin_port);

    unsigned int argTypesSize = 0;
    while (argTypes[argTypesSize] != 0) {
        ++argTypesSize;
    }
    ++argTypesSize;

    // sending message
    int size[1];
    size[0] = sizeof(REGISTER) + strlen(hostname) + 1 + sizeof(portNum) + strlen(name) + 1 + sizeof(int) * argTypesSize;
    if (send(serverSocket, size, sizeof(size), 0) < 0) {
        cerr << "write failed1" << endl;
        return -1;
    }

    char *sendBuf = new char[size[0]];
    int msgType = REGISTER;
    memcpy(sendBuf,
        &msgType, sizeof(msgType));
    memcpy(sendBuf + sizeof(REGISTER),
        hostname, strlen(hostname) + 1);
    memcpy(sendBuf + sizeof(REGISTER) + strlen(hostname) + 1,
        &portNum, sizeof(portNum));
    memcpy(sendBuf + sizeof(REGISTER) + strlen(hostname) + 1 + sizeof(portNum),
        name, strlen(name) + 1);
    memcpy(sendBuf + sizeof(REGISTER) + strlen(hostname) + 1 + sizeof(portNum) + strlen(name) + 1,
        argTypes, sizeof(int) * argTypesSize);
    if (sendAll(serverSocket, sendBuf, size) < 0) {
        cerr << "write failed2" << endl;
        return -1;
    }
    delete sendBuf;

    // waiting for result
    if (recv(serverSocket, size, sizeof(size), 0) < 0) {
        cerr << "receive failed3" << endl;
        return -1;
    }
    char *recvBuf = new char[size[0]];
    if (recvAll(serverSocket, recvBuf, size) < 0) {
        cerr << "receive failed4" << endl;
        return -1;
    }

    MessageType response;
    memcpy(&response, recvBuf, size[0]);
    delete recvBuf;
    cout <<"RESPONSE:"<<(int)response<<endl;
    if (response == REGISTER_SUCCESS) {
        struct ProcedureSignature function = {name, argTypes};
        skeletonMap[function] = f;
        cout<<"success"<<endl;
        return 0;
    } else {
        cout<<"fail"<<endl;
        return 1;
    }

    return -1;
}

int rpcExecute() {
    for (map<struct ProcedureSignature, skeleton>::iterator it=skeletonMap.begin(); it!=skeletonMap.end(); ++it) {
        cout << it->first.name << ", ";
        for (int *i = it->first.argTypes; *i != 0; i++) {
            cout << (unsigned int)*i << " ";
        } cout << '\n';
    }

    for (;;) {}

    return 0;
}
