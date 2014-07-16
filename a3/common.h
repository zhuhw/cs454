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

extern int sendAll(int s, char *buf, int *len);
extern int recvAll(int s, char *buf, int *len);

#endif
