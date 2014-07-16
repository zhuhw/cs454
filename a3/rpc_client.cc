#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rpc.h"
#include "common.h"

using namespace std;

int rpcCall(char* name, int* argTypes, void** args) {
    int clientSocket;

    char *binder_address = getenv("BINDER_ADDRESS");
    char *binder_port = getenv("BINDER_PORT");

    if (binder_address == 0 || binder_port == 0) {
        cerr << "Error: BINDER_ADDRESS or BINDER_PORT is empty." << endl;
        return -1;
    }

    //-----------------------------------------
    struct hostent *host;
    struct sockaddr_in siServer;

    // create client socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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

    if (connect(clientSocket, (struct sockaddr *)&siServer, sizeof(siServer)) < 0) {
        cerr << "Connection Failed" << endl;
        return -1;
    }


    unsigned int argTypesSize = 0;
    while (argTypes[argTypesSize] != 0) {
        ++argTypesSize;
    }
    ++argTypesSize;

    int size[1];
    size[0] = sizeof(LOC_REQUEST) + strlen(name) + 1 + sizeof(int) * argTypesSize;
    if (send(clientSocket, size, sizeof(size), 0) < 0) {
        cerr << "write failed1" << endl;
        return -1;
    }

    char *sendBuf = new char[size[0]];
    int msgType = LOC_REQUEST;
    memcpy(sendBuf,
        &msgType, sizeof(msgType));
    memcpy(sendBuf + sizeof(LOC_REQUEST),
        name, strlen(name) + 1);
    memcpy(sendBuf + sizeof(LOC_REQUEST) + strlen(name) + 1,
        argTypes, sizeof(int) * argTypesSize);
    if (sendAll(clientSocket, sendBuf, size) < 0) {
        cerr << "write failed2" << endl;
        return -1;
    }
    delete sendBuf;

    cout<<"finish sending"<<endl;
    // waiting for result
    if (recv(clientSocket, size, sizeof(size), 0) < 0) {
        cerr << "receive failed3" << endl;
        return -1;
    }
    char *recvBuf = new char[size[0]];
    if (recvAll(clientSocket, recvBuf, size) < 0) {
        cerr << "receive failed4" << endl;
        return -1;
    }

    MessageType response;
    memcpy(&response, recvBuf, sizeof(response));
    cout <<"RESPONSE:"<<(int)response<<endl;
    char *hostname;
    unsigned short portno;
    if (response == LOC_SUCCESS) {
        cout<<"success"<<endl;
        char *cur = recvBuf + sizeof(LOC_SUCCESS);
        int hostnameSize = 0;
        while (cur[hostnameSize] != 0) {
            ++hostnameSize;
        }
        ++hostnameSize;
        cout << "hostnameSize:" <<hostnameSize<<endl;
        hostname = new char[hostnameSize];

        memcpy(hostname, recvBuf + sizeof(LOC_SUCCESS), hostnameSize);
        memcpy(&portno, recvBuf + sizeof(LOC_SUCCESS) + hostnameSize, sizeof(unsigned short));

        cout <<"HOST:PORT: "<<hostname<<":"<<portno<<endl;

    } else {
        int reason;
        memcpy(&reason, recvBuf + sizeof(LOC_FAILURE), sizeof(int));
        cout<<"LOC_FAILURE reason code: "<<reason<<endl;
        return reason;
    }
    delete recvBuf;

    return 0;
}

int rpcTerminate() {
    return 0;
}
