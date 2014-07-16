#include "common.h"
#include <sys/socket.h>
#include <string.h>

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

    return (n == -1) ? -1 : 0; // return -1 on failure, 0 on success
}

int recvAll(int s, char *buf, int *len) {
    int total = 0;        // how many bytes we've received
    int bytesleft = *len; // how many we have left to receive
    int n;

    while(total < *len) {
        n = recv(s, buf+total, bytesleft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually received here

    return (n == -1) ? -1 : 0; // return -1 on failure, 0 on success
}

bool operator <(const ProcedureSignature& x, const ProcedureSignature& y) {
    if (x.name != y.name) {
        return x.name < y.name;
    }

    int xLen = 0;
    int yLen = 0;
    while (x.argTypes[xLen] != 0) {
        ++xLen;
    }
    while (y.argTypes[yLen] != 0) {
        ++yLen;
    }
    if (xLen != yLen) {
        return xLen < yLen;
    }

    return false;
}
