#ifndef COMMON_H
#define COMMON_H

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

int sendAll(int s, char *buf, int *len);
int recvAll(int s, char *buf, int *len);

#endif
