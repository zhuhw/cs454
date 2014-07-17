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

#include "function_db.h"
#include "common.h"

using namespace std;

FunctionDB function_db;

void exit_and_close(int code, int sockfd){
    close(sockfd);
    exit(code);
}

void close_and_clean_fd_set(int socket, fd_set *active_fd_set){
    cout << "close and clean " << endl;
    close(socket);
    FD_CLR(socket, active_fd_set);
}

int processRequests(int socket, fd_set *active_fd_set){
    // waiting for result
    int size[1];
    if (recv(socket, size, sizeof(size), 0) < 0) {
        cerr << "receive failed3" << endl;
        // socket closed, remove it from active_fd_set
        close_and_clean_fd_set(socket, active_fd_set);
    }
    cout << "from socket" << socket << " size:"<< size[0] << endl;
    char *recvBuf = new char[size[0]];
    if (recvAll(socket, recvBuf, size) < 0) {
        cerr << "receive failed4" << endl;
        close_and_clean_fd_set(socket, active_fd_set);
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


        int intPrtLen = ptrSize((int *)(recvBuf + 4 + hostnameLen + 1 + sizeof(short) + nameLen + 1));
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

        function_db.register_function(function, serverInfo);

        msgType = REGISTER_SUCCESS;

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
        cout<<"-------------------------------------"<<endl;
        cout<<"LOC_REQUEST"<<endl;
        char *cur = recvBuf + sizeof(LOC_REQUEST);
        int nameSize = ptrSize(cur);

        char* name = new char[nameSize];
        memcpy(name, cur, nameSize);
        name[nameSize] = '\0';
        cout<<"NAME:"<<name<<endl;

        int *intCur = (int*)(recvBuf + sizeof(LOC_REQUEST) + nameSize);
        int argTypesSize = ptrSize(intCur);
        int *argTypes = new int[argTypesSize];
        memcpy(argTypes, intCur, argTypesSize * sizeof(int));

        cout<<"ARGTYPES:";
        for (int i = 0;i < argTypesSize;i++) {
            cout << (unsigned int)argTypes[i] << " ";
        } cout<<endl;

        struct ProcedureSignature function = {name, argTypes};

        ServerInfo serverInfo = function_db.locate(function);

        int size[1];
        char *sendBuf;
        if (serverInfo.host.length() == 0) {
            cout << "LOC_FAILURE" <<endl;
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
            cout << "LOC_SUCCESS" <<endl;
            msgType = LOC_SUCCESS;

            size[0] = sizeof(LOC_SUCCESS) + serverInfo.host.length() + 1 + sizeof(unsigned short);

            if (send(socket, size, sizeof(size), 0) < 0) {
                cerr << "write failed1" << endl;
                return -1;
            }

            sendBuf = new char[size[0]];
            memcpy(sendBuf, &msgType, sizeof(msgType));

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

        cout << "finish sending" <<endl;

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

    fd_set active_fd_set, read_fd_set;
    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);
    int fdmax = sockfd;

    while(1) {
        read_fd_set = active_fd_set;
        if (select (fdmax+1, &read_fd_set, NULL, NULL, NULL) < 0) {
            cerr << "Error: select." << endl;
            exit_and_close(-1, sockfd);
        }

        cout << "new round of select: " << endl;

        for (int curSocket = 0; curSocket <= fdmax; ++curSocket) {
            if (FD_ISSET(curSocket, &read_fd_set)) {
                if (curSocket == sockfd) {
                    cout << "create socket "<< endl;
                    // Create new connection
                    int newsockfd = accept(sockfd, (struct sockaddr*)NULL, NULL);
                    if (newsockfd < 0) {
                        cerr << "Accept error" << endl;
                        return 0;
                    }
                    if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                    }

                    FD_SET(newsockfd, &active_fd_set);
                } else {
                    cout << "process" << endl;
                    processRequests(curSocket, &active_fd_set);
                }
            }
        }
    }

    close(sockfd);
}
