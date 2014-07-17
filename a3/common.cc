#include "common.h"
#include <sys/socket.h>
#include <string.h>
#include <iostream>

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
    int result = strcmp(x.name, y.name);
    if (result != 0) {
        if (result > 0)
            return true;
        else
            return false;
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
