#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <sstream>

#include "common.h"

using namespace std;

map<struct FunctionSignature, string> serverMap;

void exit_and_close(int code, int sockfd){
    close(sockfd);
    exit(code);
}

int processRequests(int socket, fd_set active_fd_set){
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

    int hostnameLen = strlen(recvBuf + 4);
    char hostname[hostnameLen];
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
        cout << intPrt[i] << " ";
    } cout<<endl;
    delete recvBuf;
    // end of parsing---------------------------

    struct FunctionSignature function = {name, intPrt};
    stringstream ss;
    ss << hostname << ":" << portno;

    if (serverMap[function] == "") {
        serverMap[function] = ss.str();
        msgType = REGISTER_SUCCESS;
    } else {
        cerr << "duplicate function" << endl;
        msgType = REGISTER_FAILURE;
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

    for (map<struct FunctionSignature, string>::iterator it=serverMap.begin(); it!=serverMap.end(); ++it) {
        cout << it->first.name << ", ";
        for (int *i = it->first.argTypes; *i != 0; i++) {
            cout << (unsigned int)*i << " ";
        } cout << " => ";
        cout << it->second << endl;
    }
}

int main() {
    struct sockaddr_in addr;
    addr.sin_port = 0;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    int sockfd = socket(AF_INET,SOCK_STREAM,0);

    // Bind
    if (bind(sockfd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) {
        cerr << "Error: fail to bind server." << endl;
        exit_and_close(-1, sockfd);
    }

    // Get hostname
    char hostname[255];
    if(gethostname(hostname,255)==-1) {
        cerr << "Error: fail to get hostname."<<endl;
    }
    cout << "BINDER_ADDRESS " << hostname <<endl;

    listen(sockfd, 128);

    // Get port number
    socklen_t len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr *)&addr, &len) == -1) {
        cerr << "Error: fail to get port number " << endl;
        exit_and_close(-1, sockfd);
    }
    cout << "BINDER_PORT " << ntohs(addr.sin_port) << endl;

    fd_set active_fd_set, read_fd_set;
    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);
    int fdmax = sockfd;

    while (1) {
        read_fd_set = active_fd_set;
        if (select (fdmax+1, &read_fd_set, NULL, NULL, NULL) < 0) {
            cerr << "Error: select." << endl;
            exit_and_close(-1, sockfd);
        }

        // Loop through all sockets with input pending
        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                if (i == sockfd){
                    // request on main socket, create a new socket
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int newsockfd = accept(sockfd, (struct sockaddr *) &client, &len);

                    if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                    }

                    FD_SET (newsockfd, &active_fd_set);
                } else {
                    processRequests(i, active_fd_set);

                    // If on a client socket
                    // char* request_msg;

                    // receive all messages
                    // if (recvall(i, request_msg) < 0) {
                    //     close(i); // close socket
                    //     FD_CLR(i, &active_fd_set); // remove from active set
                    // } else {
                    //     string message = title_case(string(request_msg));

                    //     if(sendall(i, message)){
                    //         cerr << "Error on sending message" <<endl;
                    //         exit_and_close(-1, sockfd);
                    //     }

                    //     delete [] request_msg;
                    // }

                }
            }
        }
    }

    close(sockfd);
}
