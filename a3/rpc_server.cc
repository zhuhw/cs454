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

void close_and_clean_fd_set(int socket, fd_set *active_fd_set){
    cout << "close and clean " << endl;
    close(socket);
    FD_CLR(socket, active_fd_set);
}

int processRequests(int socket, fd_set *active_fd_set){
    // waiting for result
    int size[1];
    if (recv(socket, size, sizeof(size), 0) <= 0) {
        cerr << "receive failed3" << endl;
        // socket closed, remove it from active_fd_set
        close_and_clean_fd_set(socket, active_fd_set);
        return 0;
    }
    cout << "from socket" << socket << " size:"<< size[0] << endl;
    char *recvBuf = new char[size[0]];
    if (recvAll(socket, recvBuf, size) <= 0) {
        cerr << "receive failed4" << endl;
        close_and_clean_fd_set(socket, active_fd_set);
        return 0;
    }

    int msgType;
    memcpy(&msgType, recvBuf, sizeof(int));
    cout <<"TYPE:"<< msgType << endl;

    if (msgType == EXECUTE) {
        char *cur = recvBuf + sizeof(msgType);
        int nameSize = ptrSize(cur);

        char* name = new char[nameSize];
        memcpy(name, cur, nameSize);
        // cout<<"NAME:"<<name<<endl;

        int *intCur = (int*)(recvBuf + sizeof(LOC_REQUEST) + nameSize);
        int argTypesSize = ptrSize(intCur);
        int *argTypes = new int[argTypesSize];
        memcpy(argTypes, intCur, argTypesSize * sizeof(int));

        cout<<"ARGTYPES:";
        for (int i = 0;i < argTypesSize;i++) {
            cout << (unsigned int)argTypes[i] << " ";
        } cout<<endl;

        void **args = new void*[argTypesSize - 1]; //exclude trailing zero
        for (int i = 0; i < argTypesSize - 1; i++) { // for each arg
            int argType = (argTypes[i] >> 16) & 0xFF;
            unsigned int argSize = argTypes[i] & 0xFFFF;

            int type_size = typeToSize(argType);

            if (argSize == 0){
                size[0] = type_size;
                args[i] = new char[type_size];

                if (recvAll(socket, (char *)args[i], size) <= 0) {
                    cerr << "receive failed4" << endl;
                    close_and_clean_fd_set(socket, active_fd_set);
                    return 0;
                }
            } else {
                size[0] = type_size*argSize;
                args[i] = new char[size[0]];

                if (recvAll(socket, (char *)args[i], size) <= 0) {
                    cerr << "receive failed4" << endl;
                    close_and_clean_fd_set(socket, active_fd_set);
                    return 0;
                }
            }

            // if (argType == ARG_CHAR) {
            //     if (argSize == 0) {
            //         args[i] = new char[1];
            //         size[0] = sizeof(char);
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     } else {
            //         args[i] = new char[argSize];
            //         size[0] = sizeof(char) * argSize;
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     }
            // } else if (argType == ARG_SHORT) {
            //     if (argSize == 0) {
            //         args[i] = new short[1];
            //         size[0] = sizeof(short);
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     } else {
            //         args[i] = new short[argSize];
            //         size[0] = sizeof(short) * argSize;
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     }
            // } else if (argType == ARG_INT) {
            //     if (argSize == 0) {
            //         args[i] = new int[1];
            //         size[0] = sizeof(int);
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     } else {
            //         args[i] = new int[argSize];
            //         size[0] = sizeof(int) * argSize;
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     }
            // } else if (argType == ARG_LONG) {
            //     if (argSize == 0) {
            //         args[i] = new long[1];
            //         size[0] = sizeof(long);
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     } else {
            //         args[i] = new long[argSize];
            //         size[0] = sizeof(long) * argSize;
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     }
            // } else if (argType == ARG_DOUBLE) {
            //     if (argSize == 0) {
            //         args[i] = new double[1];
            //         size[0] = sizeof(double);
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     } else {
            //         args[i] = new double[argSize];
            //         size[0] = sizeof(double) * argSize;
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     }
            // } else if (argType == ARG_FLOAT) {
            //     if (argSize == 0) {
            //         args[i] = new float[1];
            //         size[0] = sizeof(float);
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     } else {
            //         args[i] = new float[argSize];
            //         size[0] = sizeof(float) * argSize;
            //         if (recvAll(socket, (char *)args[i], size) <= 0) {
            //             cerr << "receive failed4" << endl;
            //             close_and_clean_fd_set(socket, active_fd_set);
            //             return 0;
            //         }
            //     }
            // }
        }

        struct ProcedureSignature key = {name, argTypes};
        skeleton func = skeletonMap[key];

        int returnVal = func(argTypes, args);

        cout <<"returnVal"<<returnVal<<endl;
        if (returnVal == 0) {
            size[0] = sizeof(EXECUTE_SUCCESS) + strlen(name) + 1 + sizeof(int) * argTypesSize; //all

            if (send(socket, size, sizeof(size), 0) < 0) {
                cerr << "write failed1" << endl;
                return -1;
            }

            //sending msg except for args
            char *sendBuf = new char[size[0]];
            msgType = EXECUTE_SUCCESS;
            memcpy(sendBuf,
                &msgType, sizeof(msgType));
            memcpy(sendBuf + sizeof(msgType),
                name, strlen(name) + 1);
            memcpy(sendBuf + sizeof(msgType) + strlen(name) + 1,
                argTypes, sizeof(int) * argTypesSize);
            if (sendAll(socket, sendBuf, size) < 0) {
                cerr << "write failed2" << endl;
                return -1;
            }
            delete []sendBuf;

            for (int i = 0; i < argTypesSize - 1; i++) { // for each arg
                int argType = (argTypes[i] >> 16) & 0xFF;
                unsigned int argSize = argTypes[i] & 0xFFFF;

                if (argSize == 0) {
                    size[0] = typeToSize(argType);
                } else {
                    size[0] = typeToSize(argType) * argSize;
                }
                if (sendAll(socket, (char *)args[i], size) < 0) {
                    cerr << "write failed2" << endl;
                    return -1;
                }
            }
        } else {
            //TODO handle this(send EXECUTE_FAILURE?)
        }




        // cout <<"returnVal"<<returnVal << endl;
    } else if (msgType == TERMINATE) {

    } else {
        // TODO return wrong message type code
    }


    delete []recvBuf;

    return 0;
}

int rpcInit() {
    char *binder_address = getenv("BINDER_ADDRESS");
    char *binder_port = getenv("BINDER_PORT");

    if (binder_address == 0 || binder_port == 0) {
        cerr << "Error: SERVER_ADDRESS or SERVER_PORT is empty." << endl;
        return -1;
    }

    //-----------------------------------------

    serverSocket = connectTo(binder_address, atoi(binder_port));

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
    getsockname(listenSocket, (struct sockaddr *)&siServer, &siLen);
    unsigned short portNum = ntohs(siServer.sin_port);

    int argTypesSize = ptrSize(argTypes);

    // sending message
    int size[1];
    size[0] = sizeof(REGISTER) + strlen(hostname) + 1 + sizeof(portNum) + strlen(name) + 1 + sizeof(int) * argTypesSize;
    if (send(serverSocket, size, sizeof(MessageType), 0) < 0) {
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
    delete []sendBuf;

    // waiting for result
    if (recv(serverSocket, size, sizeof(size), 0) <= 0) {
        cerr << "receive failed3" << endl;
        return -1;
    }
    char *recvBuf = new char[size[0]];
    if (recvAll(serverSocket, recvBuf, size) <= 0) {
        cerr << "receive failed4" << endl;
        return -1;
    }

    MessageType response;
    memcpy(&response, recvBuf, size[0]);
    delete []recvBuf;
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
    fd_set active_fd_set, read_fd_set;
    FD_ZERO (&active_fd_set);
    FD_SET (listenSocket, &active_fd_set);
    int fdmax = listenSocket;

    while(1) {
        read_fd_set = active_fd_set;
        if (select (fdmax+1, &read_fd_set, NULL, NULL, NULL) < 0) {
            cerr << "Error: select." << endl;
            return -1;
        }

        cout << "new round of select: " << endl;

        for (int curSocket = 0; curSocket <= fdmax; ++curSocket) {
            if (FD_ISSET(curSocket, &read_fd_set)) {
                if (curSocket == listenSocket) {
                    cout << "create socket "<< endl;
                    // Create new connection
                    int newsockfd = accept(listenSocket, (struct sockaddr*)NULL, NULL);
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

    close(listenSocket);

    return 0;
}
