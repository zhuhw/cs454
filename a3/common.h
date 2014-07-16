#ifndef COMMON_H
#define COMMON_H

#include <string>

enum MessageType {
    REGISTER,
    REGISTER_SUCCESS,
    REGISTER_FAILURE,
    LOC_REQUEST,
    LOC_SUCCESS,
    LOC_FAILURE,
    EXECUTE,
    EXECUTE_SUCCESS,
    EXECUTE_FAILURE,
    TERMINATE
};

extern int sendAll(int s, char *buf, int *len);
extern int recvAll(int s, char *buf, int *len);

struct ProcedureSignature {
    char *name;
    int *argTypes;
};

bool operator <(const ProcedureSignature& x, const ProcedureSignature& y);

struct ServerInfo {
    char* host;
    unsigned short port;
};

#endif
