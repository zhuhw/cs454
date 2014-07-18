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

enum ReasonCode {
    TERMINATE_SUCCESS,
    TERMINATE_CALL_NOT_FROM_BINDER
};

extern int sendAll(int s, char *buf, int *len);
extern int recvAll(int s, char *buf, int *len);

struct ProcedureSignature {
    std::string name;
    int *argTypes;
};

bool operator <(const ProcedureSignature& x, const ProcedureSignature& y);

struct ServerInfo {
    std::string host;
    unsigned short port;
};

bool operator == (const ServerInfo& x, const ServerInfo& y);

int connectTo(char *address, char* port);
int connectTo(struct ServerInfo info);

int ptrSize(char *ptr);
int ptrSize(int *ptr);

#endif
