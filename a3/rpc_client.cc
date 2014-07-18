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
    // request server loc
    clientSocket = connectTo(binder_address, atoi(binder_port));

    int argTypesSize = ptrSize(argTypes);

    int size[1];
    size[0] = sizeof(LOC_REQUEST) + strlen(name) + 1 + sizeof(int) * argTypesSize;
    if (send(clientSocket, size, sizeof(size), 0) < 0) {
        cerr << "write failed1" << endl;
        return -1;
    }

    cout << "send 1:" << size[0] <<endl;

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
    if (recv(clientSocket, size, sizeof(size), 0) <= 0) {
        cerr << "receive failed3" << endl;
        return -1;
    }
    char *recvBuf = new char[size[0]];
    if (recvAll(clientSocket, recvBuf, size) <= 0) {
        cerr << "receive failed4" << endl;
        return -1;
    }

    MessageType response;
    memcpy(&response, recvBuf, sizeof(response));
    cout <<"RESPONSE:"<<(int)response<<endl;
    char *hostname;
    unsigned short portno;
    if (response == LOC_FAILURE) {
        int reason;
        memcpy(&reason, recvBuf + sizeof(LOC_FAILURE), sizeof(int));
        cout<<"LOC_FAILURE reason code: "<<reason<<endl;
        return reason;
    }

    cout<<"success"<<endl;
    int hostnameSize = ptrSize(recvBuf + sizeof(LOC_SUCCESS));
    cout << "hostnameSize:" <<hostnameSize<<endl;
    hostname = new char[hostnameSize];

    memcpy(hostname, recvBuf + sizeof(LOC_SUCCESS), hostnameSize);
    memcpy(&portno, recvBuf + sizeof(LOC_SUCCESS) + hostnameSize, sizeof(unsigned short));

    delete recvBuf;
    close(clientSocket);

    // send real request
    cout <<"HOST:PORT: "<<hostname<<":"<<portno<<endl;

    int sockfd = connectTo(hostname, portno);

    // int ppSize = 0;
    // for (int i = 0; i < argTypesSize; i++) { // for each arg
    //     int argType = (argTypes[i] >> 16) & 0xFF;
    //     unsigned int argSize = argTypes[i] & 0xFFFF;
    //     cout << "t:"<<argType << " s:"<<argSize << endl;
    //
    //     if (argSize == 0) { //scalar
    //         ppSize += typeToSize(argType);
    //     } else { //vector
    //         ppSize += typeToSize(argType) * argSize;
    //     }
    // }
    size[0] = sizeof(EXECUTE) + strlen(name) + 1 + sizeof(int) * argTypesSize/* + ppSize*/; //all

    if (send(sockfd, size, sizeof(size), 0) < 0) {
        cerr << "write failed1" << endl;
        return -1;
    }

    //sending msg except for args
    // size[0] -= ppSize;
    sendBuf = new char[size[0]];
    msgType = EXECUTE;
    memcpy(sendBuf,
        &msgType, sizeof(msgType));
    memcpy(sendBuf + sizeof(msgType),
        name, strlen(name) + 1);
    memcpy(sendBuf + sizeof(msgType) + strlen(name) + 1,
        argTypes, sizeof(int) * argTypesSize);
    if (sendAll(sockfd, sendBuf, size) < 0) {
        cerr << "write failed2" << endl;
        return -1;
    }
    delete sendBuf;

    for (int i = 0; i < argTypesSize; i++) { // for each arg
        int argType = (argTypes[i] >> 16) & 0xFF;
        unsigned int argSize = argTypes[i] & 0xFFFF;


        if (argSize == 0) {
            size[0] = typeToSize(argType);
        } else {
            size[0] = typeToSize(argType) * argSize;
        }
        if (sendAll(sockfd, (char *)args[i], size) < 0) {
            cerr << "write failed2" << endl;
            return -1;
        }
    }

    // TODO receive

    close(sockfd);

    return 0;
}

int rpcTerminate() {
    return 0;
}
