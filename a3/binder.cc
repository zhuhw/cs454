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
#include <algorithm>
#include <vector>

#include "function_db.h"
#include "common.h"

using namespace std;

FunctionDB function_db;
vector<int> server_sockets;
bool terminate_flag = false;

void exit_and_close(int code, int sockfd){
    close(sockfd);
    exit(code);
}

// socket closed, remove it from active_fd_set
void close_and_clean_fd_set(int socket, fd_set *active_fd_set){
    close(socket);
    FD_CLR(socket, active_fd_set);
}

void add_socket(int socket){
    if (find(server_sockets.begin(), server_sockets.end(), socket)
            == server_sockets.end()) {
        server_sockets.push_back(socket);
    }
}

int processRequests(int socket, fd_set *active_fd_set){
    // waiting for result
    int size[1];
    if (recv(socket, size, sizeof(size), 0) <= 0) {
        close_and_clean_fd_set(socket, active_fd_set);
        return RECV_FAILED;
    }

    char *recvBuf = new char[size[0]];
    if (recvAll(socket, recvBuf, size) <= 0) {
        close_and_clean_fd_set(socket, active_fd_set);
        return RECV_FAILED;
    }

    MessageType msgType;
    memcpy(&msgType, recvBuf, sizeof(MessageType));

    if (msgType == REGISTER) {
        int hostnameLen = strlen(recvBuf + sizeof(MessageType));
        char *hostname = new char[hostnameLen + 1];
        memcpy(hostname, recvBuf + sizeof(MessageType), hostnameLen + 1);

        unsigned short portno;
        memcpy(&portno, recvBuf + sizeof(MessageType)+ hostnameLen + 1, sizeof(unsigned short));

        int nameLen = strlen(recvBuf + sizeof(MessageType) + hostnameLen + 1 + sizeof(unsigned short));
        char *name = new char[nameLen + 1];
        memcpy(name, recvBuf + sizeof(MessageType) + hostnameLen + 1 + sizeof(short), nameLen + 1);

        int intPrtLen = ptrSize((int *)(recvBuf + 4 + hostnameLen + 1 + sizeof(short) + nameLen + 1));
        int *intPrt = new int[intPrtLen];
        memcpy(intPrt, recvBuf + 4 + hostnameLen + 1 + sizeof(short) + nameLen + 1, sizeof(int) * intPrtLen);

        delete []recvBuf;

        struct ProcedureSignature function = {name, intPrt};
        struct ServerInfo serverInfo = {hostname, portno};

        function_db.register_function(function, serverInfo);
        add_socket(socket);

        msgType = REGISTER_SUCCESS;

        size[0] = sizeof(MessageType);
        if (send(socket, size, sizeof(MessageType), 0) < 0) {
            close_and_clean_fd_set(socket, active_fd_set);
            return SEND_FAILED;
        }

        char *sendBuf = new char[size[0]];
        memcpy(sendBuf, &msgType, sizeof(MessageType));
        if (sendAll(socket, sendBuf, size) < 0) {
            close_and_clean_fd_set(socket, active_fd_set);
            return SEND_FAILED;
        }
        delete []sendBuf;

    } else if (msgType == LOC_REQUEST) {
        char *cur = recvBuf + sizeof(MessageType);
        int nameSize = ptrSize(cur);

        char* name = new char[nameSize];
        memcpy(name, cur, nameSize);

        int *intCur = (int*)(recvBuf + sizeof(MessageType) + nameSize);
        int argTypesSize = ptrSize(intCur);
        int *argTypes = new int[argTypesSize];
        memcpy(argTypes, intCur, argTypesSize * sizeof(int));

        struct ProcedureSignature function = {name, argTypes};

        ServerInfo serverInfo = function_db.locate(function);

        char *sendBuf;
        if (serverInfo.host.length() == 0) {
            msgType = LOC_FAILURE;
            size[0] = sizeof(MessageType) + sizeof(int);

            if (send(socket, size, sizeof(size), 0) < 0) {
                close_and_clean_fd_set(socket, active_fd_set);
                return SEND_FAILED;
            }

            sendBuf = new char[size[0]];
            memcpy(sendBuf, &msgType, sizeof(MessageType));

            int reasonCode = LOC_FAILURE_SERVER_NOT_FOUND;
            memcpy(sendBuf + sizeof(MessageType), &reasonCode, sizeof(ReasonCode));
        } else {
            msgType = LOC_SUCCESS;
            size[0] = sizeof(MessageType) + serverInfo.host.length() + 1 + sizeof(unsigned short);

            if (send(socket, size, sizeof(size), 0) < 0) {
                close_and_clean_fd_set(socket, active_fd_set);
                return SEND_FAILED;
            }

            sendBuf = new char[size[0]];
            memcpy(sendBuf, &msgType, sizeof(MessageType));

            char *host = new char[serverInfo.host.length() + 1];
            strcpy(host, serverInfo.host.c_str());

            memcpy(sendBuf + sizeof(MessageType),
                host, serverInfo.host.length() + 1);
            unsigned short port = serverInfo.port;
            memcpy(sendBuf + sizeof(MessageType) + serverInfo.host.length() + 1,
                &port, sizeof(unsigned short));
        }

        if (sendAll(socket, sendBuf, size) < 0) {
            close_and_clean_fd_set(socket, active_fd_set);
            return SEND_FAILED;
        }

        delete []sendBuf;
        delete []recvBuf;
    } else if (msgType == TERMINATE) {
        size[0] = sizeof(MessageType);

        char *sendBuf = new char[size[0]];
        memcpy(sendBuf, &msgType, sizeof(MessageType));

        for (int i = 0; i < server_sockets.size(); ++i) {
            int server_socket = server_sockets[i];

            if (send(server_socket, size, sizeof(MessageType), 0) < 0) {
                continue;
            }

            if (sendAll(server_socket, sendBuf, size) < 0) {
                continue;
            }

            char *recvBuf = new char[size[0]];
            if (recvAll(server_socket, recvBuf, size) <= 0) {
                continue;
            }

            delete[] recvBuf;
        }

        terminate_flag = true;

        delete [] sendBuf;
    } else if (CACHE_REQEUST) {
        char *cur = recvBuf + sizeof(MessageType);
        int nameSize = ptrSize(cur);

        char* name = new char[nameSize];
        memcpy(name, cur, nameSize);

        int *intCur = (int*)(recvBuf + sizeof(MessageType) + nameSize);
        int argTypesSize = ptrSize(intCur);
        int *argTypes = new int[argTypesSize];
        memcpy(argTypes, intCur, argTypesSize * sizeof(int));

        struct ProcedureSignature function = {name, argTypes};

        list<ServerInfo>* server_list = function_db.getList(function);

        int first_int;
        size[0] = sizeof(first_int);
        if (server_list == NULL){
            first_int = LOC_FAILURE_SERVER_NOT_FOUND;
        } else {
            first_int = server_list->size();
        }

        char *sendBuf = new char[size[0]];
        MessageType msgType = TERMINATE;
        memcpy(sendBuf, &first_int, sizeof(msgType));
        if (sendAll(socket, sendBuf, size) < 0) {
            return SEND_FAILED;
        }

        if (server_list == NULL)
            return 0;

        for (list<ServerInfo>::iterator it=server_list->begin(); it!=server_list->end(); ++it) {
            ServerInfo info = (*it);
            size[0] = info.host.length() + 1 + sizeof(unsigned short);

            if (send(socket, size, sizeof(size), 0) < 0) {
                return SEND_FAILED;
            }

            char *sendBuf = new char[size[0]];

            char *host = new char[info.host.length() + 1];
            strcpy(host, info.host.c_str());

            memcpy(sendBuf,
                host, info.host.length() + 1);
            unsigned short port = info.port;
            memcpy(sendBuf + info.host.length() + 1,
                &port, sizeof(unsigned short));

            if (sendAll(socket, sendBuf, size) < 0) {
                return SEND_FAILED;
            }
        }

    } else {
        close_and_clean_fd_set(socket, active_fd_set);
        return UNKNOWN_MSG_TYPE;
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

        for (int curSocket = 0; curSocket <= fdmax; ++curSocket) {
            if (FD_ISSET(curSocket, &read_fd_set)) {
                if (curSocket == sockfd) {
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
                    processRequests(curSocket, &active_fd_set);
                }
            }
        }

        if (terminate_flag)
            break;
    }

    close(sockfd);
}
