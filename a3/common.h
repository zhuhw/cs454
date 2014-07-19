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
    TERMINATE,
    CACHE_REQEUST
};

enum ReasonCode {
    ENV_NOT_SET = -1,                        // environment variables are not set
    SEND_FAILED = -2,                        // socket cannot send successfully
    RECV_FAILED = -3,                        // socket cannot receive successfully
    UNKNOWN_MSG_TYPE = -4,                   // received message but type unexpected
    TERMINATE_CALL_NOT_FROM_BINDER = -5,     // terminate call is not sent from binder
    LOC_FAILURE_SERVER_NOT_FOUND = -6,       // cannot locate server
    SKELETON_FAILURE = -7,                   // skeleton failed to execute properly
    REGISTER_DUPLICATE = -8                  // another function of the same server exists
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

int connectTo(const char *address, char* port);
int connectTo(const char *address, unsigned short port);
int connectTo(struct ServerInfo info);

int ptrSize(char *ptr);
int ptrSize(int *ptr);

int typeToSize(int type);

#endif
