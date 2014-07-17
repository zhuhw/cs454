CXX = g++
CXXFLAGS = -g -Wall -MMD

LIB_OBJECTS = rpc_client.o rpc_server.o common.o
LIB_TARGT = lib

SERVER_OBJECTS = server_functions.o server_function_skels.o server.o
SERVER_TARGET = server

CLIENT_OBJECTS = client1.o common.o
CLIENT_TARGET = client

BINDER_OBJECTS = binder.o common.o function_db.o
BINDER_TARGET = binder

###########################################################

all : clean ${LIB_TARGT} ${BINDER_TARGET} ${SERVER_TARGET} ${CLIENT_TARGET}

${LIB_TARGT} : ${LIB_OBJECTS}
	ar crs librpc.a ${LIB_OBJECTS}

${BINDER_TARGET} : ${BINDER_OBJECTS}
	${CXX} ${BINDER_OBJECTS} -o ${BINDER_TARGET}

${SERVER_TARGET} : ${SERVER_OBJECTS}
	${CXX} -L. ${SERVER_OBJECTS} -lrpc -pthread -o server

${CLIENT_TARGET} : ${CLIENT_OBJECTS}
	${CXX} -L. ${CLIENT_OBJECTS} -lrpc -pthread -o client


clean :
	rm -f *.d *.o *.a server client binder
