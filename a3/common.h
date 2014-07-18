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
    ENV_NOT_SET = -1,
    SEND_FAILED = -2,
    RECV_FAILED = -3,
    UNKNOWN_MSG_TYPE = -4,
    TERMINATE_CALL_NOT_FROM_BINDER = -5,
    LOC_FAILURE_SERVER_NOT_FOUND = -6,
    SKELETON_FAILURE = -7,
    REGISTER_DUPLICATE = -8
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
int connectTo(char *address, unsigned short port);
int connectTo(struct ServerInfo info);

int ptrSize(char *ptr);
int ptrSize(int *ptr);

int typeToSize(int type);

#endif
