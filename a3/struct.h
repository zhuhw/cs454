#ifndef STRUCT_H
#define STRUCT_H

enum MessageType {
    REGISTER,
    LOC_REQUEST,
    LOC_FAILURE,
    EXECUTE,
    EXECUTE_SUCCESS,
    EXECUTE_FAILURE,
    TERMINATE
};

struct BaseHeader{
    int length;
    MessageType type;
};

#endif
