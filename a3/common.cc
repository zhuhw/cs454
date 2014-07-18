#include "common.h"

#include <sys/socket.h>
#include <string.h>
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

#include "rpc.h"

using namespace std;

int sendAll(int s, char *buf, int *len) {
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return (n == -1) ? n : total; // return -1 on failure, 0 on success
}

int recvAll(int s, char *buf, int *len) {
    int total = 0;        // how many bytes we've received
    int bytesleft = *len; // how many we have left to receive
    int n;

    while(total < *len) {
        n = recv(s, buf+total, bytesleft, 0);
        // cout << "recvAll:" << total << " " << n << endl;
        if (n <= 0) {
            break;
        }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually received here

    return (n <= 0) ? n : total; // return -1 on failure, 0 on success
}

bool operator <(const ProcedureSignature& x, const ProcedureSignature& y) {
    if (x.name != y.name) {
        return x.name < y.name;
    }

    // same function name, check args
    int i = 0;

    // the array ends with 0
    while (x.argTypes[i] != 0 && y.argTypes[i] != 0) {
        int x_last_2 = x.argTypes[i] & 0xffff;
        int y_last_2 = y.argTypes[i] & 0xffff;

        // first 8 bits have to be the same
        // the last 8 bits need to be either all 0 or none 0
        if ( ( (x.argTypes[i] & 0xFFFF0000) != (y.argTypes[i] & 0xFFFF0000)) ||
             (x_last_2 != 0 && y_last_2== 0) || (x_last_2 == 0 && y_last_2 != 0) ) {
            return x.argTypes[i] < y.argTypes[i];
        }

        ++i;
    }


    if (x.argTypes[i] == 0){
        // y is longer than x
        return false;
    } else {
        // x is longer than y
        return true;
    }
}

bool operator ==(const ServerInfo& x, const ServerInfo& y){
    return ( x.host == y.host ) && (x.port == y.port);
}

int connectTo(const char* address, sockaddr_in siServer) {
    int clientSocket;
    struct hostent *host;

    // create client socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Cannot create socket" << endl;
        exit(-1);
    }

    siServer.sin_family = AF_INET;

    // connect to server
    host = gethostbyname(address);
    if (!host) {
        cout << "could not resolve hostname!" << endl;
        return -1;
    }

    memcpy((void *)&siServer.sin_addr, host->h_addr_list[0], host->h_length);


    if (connect(clientSocket, (struct sockaddr *)&siServer, sizeof(siServer)) < 0) {
        cerr << "Connection Failed" << endl;
        return -1;
    }

    return clientSocket;
}

int connectTo(char *address, char* port) {
    return connectTo(address, atoi(port));
}

int connectTo(char *address, unsigned short port) {
    struct sockaddr_in siServer;
    // set client info, port is implicitly set to 0 by memset
    memset((char *)&siServer, 0, sizeof(siServer));
    siServer.sin_port = htons(port);
    // cout << address << " " << port << endl;
    return connectTo(address, siServer);
}

int connectTo(struct ServerInfo info) {
    struct sockaddr_in siServer;
    // set client info, port is implicitly set to 0 by memset
    memset((char *)&siServer, 0, sizeof(siServer));
    siServer.sin_port = info.port;
    return connectTo(info.host.c_str(), siServer);
}

int ptrSize(char *ptr) {
    int size = 0;
    while (ptr[size] != 0) {
        ++size;
    }
    return size + 1;
}

int ptrSize(int *ptr) {
    int size = 0;
    while (ptr[size] != 0) {
        ++size;
    }
    return size + 1;
}

int typeToSize(int type) {
    if (type == ARG_CHAR) {
        return sizeof(char);
    } else if (type == ARG_SHORT) {
        return sizeof(short);
    } else if (type == ARG_INT) {
        return sizeof(int);
    } else if (type == ARG_LONG) {
        return sizeof(long);
    } else if (type == ARG_DOUBLE) {
        return sizeof(double);
    } else if (type == ARG_FLOAT) {
        return sizeof(float);
    }
    return -1;
}
