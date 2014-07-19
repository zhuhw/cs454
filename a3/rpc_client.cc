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
#include "function_db.h"

using namespace std;

FunctionDB function_db;

int callServer (char* name, int* argTypes, void** args, const char *hostname, unsigned short portno) {
    // sending execute request
    int size[1];
    int sockfd = connectTo(hostname, portno);

    int argTypesSize = ptrSize(argTypes);
    size[0] = sizeof(MessageType) + strlen(name) + 1 + sizeof(int) * argTypesSize; //all

    if (send(sockfd, size, sizeof(size), 0) < 0) {
        return SEND_FAILED;
    }

    // sending msg except for args
    char *sendBuf = new char[size[0]];
    MessageType msgType = EXECUTE;
    memcpy(sendBuf,
        &msgType, sizeof(MessageType));
    memcpy(sendBuf + sizeof(MessageType),
        name, strlen(name) + 1);
    memcpy(sendBuf + sizeof(MessageType) + strlen(name) + 1,
        argTypes, sizeof(int) * argTypesSize);
    if (sendAll(sockfd, sendBuf, size) < 0) {
        return SEND_FAILED;
    }
    delete []sendBuf;

    // sending args now
    for (int i = 0; i < argTypesSize - 1; i++) { // for each arg
        int argType = (argTypes[i] >> 16) & 0xFF;
        unsigned int argSize = argTypes[i] & 0xFFFF;

        if (argSize == 0) {
            size[0] = typeToSize(argType);
        } else {
            size[0] = typeToSize(argType) * argSize;
        }
        if (sendAll(sockfd, (char *)args[i], size) < 0) {
            return SEND_FAILED;
        }
    }

    // receive and write to buffer
    if (recv(sockfd, size, sizeof(size), 0) <= 0) {
        return RECV_FAILED;
    }
    char *recvBuf = new char[size[0]];
    if (recvAll(sockfd, recvBuf, size) <= 0) {
        return RECV_FAILED;
    }

    memcpy(&msgType, recvBuf, sizeof(MessageType));
    // if execute failed, return reason code
    if (msgType == EXECUTE_FAILURE) {
        int reason;
        memcpy(&reason, recvBuf + sizeof(MessageType), sizeof(int));
        delete []recvBuf;
        return reason;
    }

    // receive result
    if (msgType == EXECUTE_SUCCESS) {
        char *cur = recvBuf + sizeof(MessageType);
        int nameSize = ptrSize(cur);

        char* name = new char[nameSize];
        memcpy(name, cur, nameSize);

        int *intCur = (int*)(recvBuf + sizeof(MessageType) + nameSize);
        int argTypesSize = ptrSize(intCur);
        memcpy(argTypes, intCur, argTypesSize * sizeof(int));

        for (int i = 0; i < argTypesSize - 1; i++) { // for each arg
            int argType = (argTypes[i] >> 16) & 0xFF;
            unsigned int argSize = argTypes[i] & 0xFFFF;

            int type_size = typeToSize(argType);
            if (argSize == 0){
                size[0] = type_size;
            } else {
                size[0] = type_size * argSize;
            }
            args[i] = new char[size[0]];

            if (recvAll(sockfd, (char *)args[i], size) <= 0) {
                return RECV_FAILED;
            }
        }
    } else {
        // unknown message type
        return UNKNOWN_MSG_TYPE;
    }

    close(sockfd);

    return 0;
}

int connect_and_lookup(char* name, int* argTypes, MessageType msgType){
    // get binder information
    char *binder_address = getenv("BINDER_ADDRESS");
    char *binder_port = getenv("BINDER_PORT");

    if (binder_address == 0 || binder_port == 0) {
        cerr << "Error: BINDER_ADDRESS or BINDER_PORT is empty." << endl;
        return ENV_NOT_SET;
    }

    // request server location
    int clientSocket = connectTo(binder_address, binder_port);

    int argTypesSize = ptrSize(argTypes);

    int size[1];
    size[0] = sizeof(MessageType) + strlen(name) + 1 + sizeof(int) * argTypesSize;
    if (send(clientSocket, size, sizeof(size), 0) < 0) {
        return SEND_FAILED;
    }

    char *sendBuf = new char[size[0]];
    memcpy(sendBuf,
        &msgType, sizeof(MessageType));
    memcpy(sendBuf + sizeof(MessageType),
        name, strlen(name) + 1);
    memcpy(sendBuf + sizeof(MessageType) + strlen(name) + 1,
        argTypes, sizeof(int) * argTypesSize);
    if (sendAll(clientSocket, sendBuf, size) < 0) {
        return SEND_FAILED;
    }
    delete []sendBuf;

    return clientSocket;
}

int rpcCall(char* name, int* argTypes, void** args) {
    // get binder information
    char *binder_address = getenv("BINDER_ADDRESS");
    char *binder_port = getenv("BINDER_PORT");

    if (binder_address == 0 || binder_port == 0) {
        cerr << "Error: BINDER_ADDRESS or BINDER_PORT is empty." << endl;
        return ENV_NOT_SET;
    }

    // request server location
    int clientSocket = connectTo(binder_address, binder_port);

    int argTypesSize = ptrSize(argTypes);

    int size[1];
    size[0] = sizeof(MessageType) + strlen(name) + 1 + sizeof(int) * argTypesSize;
    if (send(clientSocket, size, sizeof(size), 0) < 0) {
        return SEND_FAILED;
    }

    char *sendBuf = new char[size[0]];
    MessageType msgType = LOC_REQUEST;
    memcpy(sendBuf,
        &msgType, sizeof(MessageType));
    memcpy(sendBuf + sizeof(MessageType),
        name, strlen(name) + 1);
    memcpy(sendBuf + sizeof(MessageType) + strlen(name) + 1,
        argTypes, sizeof(int) * argTypesSize);
    if (sendAll(clientSocket, sendBuf, size) < 0) {
        return SEND_FAILED;
    }
    delete []sendBuf;

    // receive server location
    if (recv(clientSocket, size, sizeof(size), 0) <= 0) {
        return RECV_FAILED;
    }
    char *recvBuf = new char[size[0]];
    if (recvAll(clientSocket, recvBuf, size) <= 0) {
        return RECV_FAILED;
    }

    MessageType response;
    memcpy(&response, recvBuf, sizeof(MessageType));
    if (response == LOC_FAILURE) {
        int reason;
        memcpy(&reason, recvBuf + sizeof(MessageType), sizeof(int));
        return reason;
    }

    int hostnameSize = ptrSize(recvBuf + sizeof(MessageType));
    char *hostname = new char[hostnameSize];
    memcpy(hostname, recvBuf + sizeof(MessageType), hostnameSize);
    unsigned short portno;
    memcpy(&portno, recvBuf + sizeof(MessageType) + hostnameSize, sizeof(unsigned short));

    delete []recvBuf;
    close(clientSocket);

    return callServer(name, argTypes, args, hostname, portno);
}

int rpcTerminate() {
    // get binder information
    char *binder_address = getenv("BINDER_ADDRESS");
    char *binder_port = getenv("BINDER_PORT");

    if (binder_address == 0 || binder_port == 0) {
        cerr << "Error: BINDER_ADDRESS or BINDER_PORT is empty." << endl;
        return ENV_NOT_SET;
    }

    // send terminate message
    int binderSocket = connectTo(binder_address, binder_port);

    int size[1];
    size[0] = sizeof(MessageType);

    if (send(binderSocket, size, sizeof(MessageType), 0) < 0) {
        return SEND_FAILED;
    }

    char *sendBuf = new char[size[0]];
    MessageType msgType = TERMINATE;
    memcpy(sendBuf, &msgType, sizeof(msgType));
    if (sendAll(binderSocket, sendBuf, size) < 0) {
        return SEND_FAILED;
    }

    return 0;
}

int rpcCacheCall(char * name, int * argTypes, void ** args){
    struct ProcedureSignature key = {name, argTypes};
    int status = -1;
    bool connected_to_binder = false;
    int list_size = 0;

    do {
        ServerInfo serverInfo = function_db.locate(key);
        int size[1];

        MessageType msgType;
        if (serverInfo.host == "") {
            msgType = CACHE_REQEUST;
            int clientSocket = connect_and_lookup(name, argTypes, CACHE_REQEUST);
            if (clientSocket < 0){
                return clientSocket;
            }

            // receive server location
            if (recv(clientSocket, size, sizeof(size), 0) <= 0) {
                return RECV_FAILED;
            }

            int serverSize = size[0];

            if (serverSize <= 0) {
                return serverSize;
            }

            for (int i = 0; i < serverSize; i++) {
                if (recv(clientSocket, size, sizeof(size), 0) <= 0) {
                    return RECV_FAILED;
                }
                char *recvBuf = new char[size[0]];
                if (recvAll(clientSocket, recvBuf, size) <= 0) {
                    return RECV_FAILED;
                }

                int hostSize = ptrSize(recvBuf);
                char host[hostSize];
                memcpy(host, recvBuf, hostSize);
                serverInfo.host = string(host);

                unsigned short port;
                memcpy(&port, recvBuf + hostSize, sizeof(unsigned short));
                serverInfo.port = port;

                function_db.register_function(key, serverInfo);
            }

            connected_to_binder = true;
        }

        status = callServer(name, argTypes, args, serverInfo.host.c_str(), serverInfo.port);

        if (status < 0){
            function_db.remove(key, serverInfo);
        }

        function_db.print();

        list<ServerInfo>* info_list = function_db.getList(key);
        list_size = info_list == NULL ? 0 : info_list->size();
    } while (status < 0 && !(connected_to_binder && list_size == 0) );

    return 0;
}
