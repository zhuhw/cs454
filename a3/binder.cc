#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <string.h>

#include "common.h"

using namespace std;

static map<struct ProcedureSignature, struct ServerInfo> serverMap;

void exit_and_close(int code, int sockfd){
    close(sockfd);
    exit(code);
}

int processRequests(int socket){
    // waiting for result
    int size[1];
    if (recv(socket, size, sizeof(size), 0) < 0) {
        cerr << "receive failed3" << endl;
        exit_and_close(-1, socket);
    }
    char *recvBuf = new char[size[0]];
    if (recvAll(socket, recvBuf, size) < 0) {
        cerr << "receive failed4" << endl;
        exit_and_close(-1, socket);
    }

    int msgType;
    memcpy(&msgType, recvBuf, sizeof(int));
    cout <<"TYPE:"<< msgType << endl;

    if (msgType == REGISTER) {
        int hostnameLen = strlen(recvBuf + 4);
        char *hostname = new char[hostnameLen];
        memcpy(hostname, recvBuf + 4, hostnameLen);
        hostname[hostnameLen] = '\0';
        cout <<"HOSTNAME:"<<hostname<<endl;

        unsigned short portno;
        memcpy(&portno, recvBuf + 4 + hostnameLen + 1, sizeof(short));
        cout <<"PORT:"<<portno<<endl;

        int nameLen = strlen(recvBuf + 4 + hostnameLen + 1 + sizeof(short));
        char *name = new char[nameLen];
        memcpy(name, recvBuf + 4 + hostnameLen + 1 + sizeof(short), nameLen);
        name[nameLen] = '\0';
        cout <<"FUNC NAME:"<<name<<endl;

        int intPrtLen = 0;
        int *cur = (int*)(recvBuf + 4 + hostnameLen + 1 + sizeof(short) + nameLen + 1);
        while (cur[intPrtLen] != 0) {
            ++intPrtLen;
        }
        ++intPrtLen;
        int *intPrt = new int[intPrtLen];
        memcpy(intPrt, recvBuf + 4 + hostnameLen + 1 + sizeof(short) + nameLen + 1, sizeof(int) * intPrtLen);

        cout<<"ARGTYPES:";
        for (int i = 0;i < intPrtLen;i++) {
            cout << (unsigned int)intPrt[i] << " ";
        } cout<<endl;
        delete recvBuf;
        // end of parsing---------------------------

        struct ProcedureSignature function = {name, intPrt};
        struct ServerInfo serverInfo = {hostname, portno};

        if (serverMap.find(function) == serverMap.end()) {
            msgType = REGISTER_SUCCESS;
        } else {
            cerr << "duplicate function" << endl;
            msgType = REGISTER_FAILURE;
        }
        serverMap[function] = serverInfo;

        for (map<struct ProcedureSignature, struct ServerInfo>::iterator it=serverMap.begin(); it!=serverMap.end(); ++it) {
            cout << it->first.name << ", ";
            for (int *i = it->first.argTypes; *i != 0; i++) {
                cout << (unsigned int)*i << " ";
            } cout << " => ";
            cout << it->second.host << it->second.port << endl;
        }

        size[0] = sizeof(msgType);
        if (send(socket, size, sizeof(size), 0) < 0) {
            cerr << "write failed1" << endl;
            exit_and_close(-1, socket);
        }

        char *sendBuf = new char[size[0]];
        cout <<"msgType:"<<msgType<<endl;
        memcpy(sendBuf, &msgType, sizeof(msgType));
        if (sendAll(socket, sendBuf, size) < 0) {
            cerr << "write failed2" << endl;
            exit_and_close(-1, socket);
        }
        delete sendBuf;

    } else if (msgType == LOC_REQUEST) {
        cout<<"LOC_REQUEST"<<endl;
        unsigned int nameSize = 0;
        char *cur = recvBuf + sizeof(LOC_REQUEST);
        while (cur[nameSize] != 0) {
            ++nameSize;
        }
        ++nameSize;

        char* name = new char[nameSize];
        memcpy(name, cur, nameSize);
        name[nameSize] = '\0';
        cout<<"NAME:"<<name<<endl;

        int argTypesSize = 0;
        int *intCur = (int*)(recvBuf + sizeof(LOC_REQUEST) + nameSize);
        while (intCur[argTypesSize] != 0) {
            ++argTypesSize;
        }
        ++argTypesSize;
        int *argTypes = new int[argTypesSize];
        memcpy(argTypes, intCur, argTypesSize * sizeof(int));

        cout<<"ARGTYPES:";
        for (int i = 0;i < argTypesSize;i++) {
            cout << (unsigned int)argTypes[i] << " ";
        } cout<<endl;

        struct ProcedureSignature function = {name, argTypes};
        map<struct ProcedureSignature, struct ServerInfo>::iterator findIt = serverMap.find(function);

        int size[1];
        char *sendBuf;
        if (findIt == serverMap.end()) {
            msgType = LOC_FAILURE;
            size[0] = sizeof(LOC_FAILURE) + sizeof(int);

            if (send(socket, size, sizeof(size), 0) < 0) {
                cerr << "write failed1" << endl;
                return -1;
            }

            sendBuf = new char[size[0]];
            memcpy(sendBuf, &msgType, sizeof(msgType));

            int reasonCode = -1;
            memcpy(sendBuf + sizeof(msgType), &reasonCode, sizeof(int));
        } else {
            msgType = LOC_SUCCESS;
            size[0] = sizeof(LOC_SUCCESS) + findIt->second.host.length() + 1 + sizeof(unsigned short);

            if (send(socket, size, sizeof(size), 0) < 0) {
                cerr << "write failed1" << endl;
                return -1;
            }

            sendBuf = new char[size[0]];
            memcpy(sendBuf, &msgType, sizeof(msgType));

            struct ServerInfo serverInfo = findIt->second;
            char *host = new char[serverInfo.host.length() + 1];
            strcpy(host, serverInfo.host.c_str());

            memcpy(sendBuf + sizeof(msgType),
                host, serverInfo.host.length() + 1);
            unsigned short port = serverInfo.port;
            memcpy(sendBuf + sizeof(msgType) + serverInfo.host.length() + 1,
                &port, sizeof(unsigned short));
        }

        if (sendAll(socket, sendBuf, size) < 0) {
            cerr << "write failed2" << endl;
            return -1;
        }

        delete sendBuf;
        delete recvBuf;
    }

    return 0;
}

int main() {
    struct sockaddr_in addr;
    addr.sin_port = 0;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Cannot create master socket for listen" << endl;
        exit_and_close(-1, sockfd);
    }

    // Bind
    if (bind(sockfd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) {
        cerr << "Error: fail to bind server." << endl;
        exit_and_close(-1, sockfd);
    }

    // Get hostname
    char hostname[255];
    if(gethostname(hostname,255) < 0) {
        cerr << "Error: fail to get hostname."<<endl;
    }
    cout << "BINDER_ADDRESS " << hostname <<endl;

    // listen socket
    if (listen(sockfd, 128) < 0) {
        cerr << "lisen failed" << endl;
        return 0;
    }

    // Get port number
    socklen_t len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr *)&addr, &len) < 0) {
        cerr << "Error: fail to get port number " << endl;
        exit_and_close(-1, sockfd);
    }
    cout << "BINDER_PORT " << ntohs(addr.sin_port) << endl;

    fd_set readfds;
    int maxSocketFd = 1;
    vector<int> clientSockets;
    // Wait for message from client
    for (;;) {
        // clear read socket sets
        FD_ZERO(&readfds);

        // add server socket to the read set explicity
        FD_SET(sockfd, &readfds);
        maxSocketFd = sockfd + 1;

        // add all established connection sockets to read set
        for (vector<int>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it) {
            if (*it > 0) {
                FD_SET(*it, &readfds);
            }

            if (*it >= maxSocketFd) {
                maxSocketFd = *it + 1;
            }
        }

        // some error on select()
        if (select(maxSocketFd, &readfds, NULL, NULL, NULL) < 0) {
            cerr << "Select error" << endl;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            // New connection
            int fd = accept(sockfd, (struct sockaddr*)NULL, NULL);
            if (fd < 0) {
                cerr << "Accept error" << endl;
                return 0;
            }
            clientSockets.push_back(fd);
        }

        for (vector<int>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it) {
            if (FD_ISSET(*it, &readfds)) {
                processRequests(*it);
            }
        }
    }

    close(sockfd);
}
