#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <pthread.h>
#include <queue>

#include "socketIO.cc"

using namespace std;

// shared variables between threads
bool inputDone = false;		// indicates if user input reaches EOF
queue<string> msgQueue;		// FIFO queue for input message buffering

// thread method for sending/receiving messages
void *sendRecv(void *param) {
    for (;;) {
        if (msgQueue.size() > 0) {
			// send next message from queue
            int size[1];
            size[0] = msgQueue.front().length() + 1;
            if (send((long)param, size, sizeof(size), 0) < 0) {
                cerr << "write failed" << endl;
                return 0;
            }

            char *sendBuf = new char[size[0]];
            memcpy(sendBuf, msgQueue.front().c_str(), size[0]);

            if (sendAll((long)param, sendBuf, size) < 0) {
                cerr << "write failed" << endl;
                return 0;
            }
            msgQueue.pop();
            delete sendBuf;

			// receive server response
            if (recv((long)param, size, sizeof(size), 0) > 0) {
                char *recvBuf = new char[size[0]];
                if (recvAll((long)param, recvBuf, size) == 0) {
                    cout << "Server: " << recvBuf << endl;
                }
                delete recvBuf;
            }

        }
        if (inputDone && msgQueue.empty()) {
            break;
        }
        sleep(2);
    }

    pthread_exit(NULL);
}

int main (int argc, char *argv[]) {
    // exit if we don't have exactly 1 argument
    if (argc != 1) {
        cerr << "argc should be 1" << endl;
        return 0;
    }

    struct hostent *hp;
    struct sockaddr_in siServer;

    int clientSocket = 0;
    int maxSocketFd = 1;
    fd_set readfds;

	// getting environment variables
    char *serverAddress = getenv("SERVER_ADDRESS");
    if (serverAddress == NULL) {
        cout << "$SERVER_ADDRESS is not set\n";
        return 0;
    }
    char *serverPort = getenv("SERVER_PORT");
    if (serverPort == NULL) {
        cout << "$SERVER_PORT is not set\n";
        return 0;
    }

	// create client socket
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Cannot create socket" << endl;
        return 0;
    }

	// set client info, port is implicitly set to 0 by memset
    memset((char *)&siServer, 0, sizeof(siServer));
    siServer.sin_family = AF_INET;
    siServer.sin_port = htons(atoi(serverPort));

	// connect to server
    hp = gethostbyname(serverAddress);
    if (!hp) {
        cout << "could not resolve hostname!" << endl;
        return 0;
    }

    memcpy((void *)&siServer.sin_addr, hp->h_addr_list[0], hp->h_length);

    if (connect(clientSocket, (struct sockaddr *)&siServer, sizeof(siServer)) < 0) {
        cerr << "Connection Failed" << endl;
        return 0;
    }

	// create thread that send/receive messages
	// use main() as another thread to get user input
    pthread_t sendRecvThread;
    int rc = pthread_create(&sendRecvThread, NULL, sendRecv, (void *)clientSocket);

    // get user input
    string line;
    while(getline(cin, line)) {
        if (line.length() == 0) {
            continue;
        }
        msgQueue.push(line);
    }
    inputDone = true;

	// wait for thread to finish
    void *status;
    rc = pthread_join(sendRecvThread, &status);

    pthread_exit(NULL);

    close(clientSocket);

    return 0;
}
