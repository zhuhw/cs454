#ifndef COMMON_H
#define COMMON_H

#include <string>

enum MessageType {
    REGISTER,
    REGISTER_SUCCESS,
    REGISTER_FAILURE,
    LOC_REQUEST,
    LOC_FAILURE,
    EXECUTE,
    EXECUTE_SUCCESS,
    EXECUTE_FAILURE,
    TERMINATE
};

extern int sendAll(int s, char *buf, int *len);
extern int recvAll(int s, char *buf, int *len);

struct FunctionSignature {
    char *name;
    int *argTypes;
};

bool operator <(const FunctionSignature& x, const FunctionSignature& y);

#endif
