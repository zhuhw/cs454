#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <queue>

#include "socketIO.cc"

using namespace std;

bool inputDone = false;
queue<string> msgQueue;

void *sendRecv(void *param) {
    while (1) {
        if (msgQueue.size() > 0) {
            int size[1];
            size[0] = msgQueue.front().length() + 1;
            if (write((long)param, size, sizeof(size)) < 0) {
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

            int n;
            if ((n = read((long)param, size, sizeof(int))) > 0) {
				char *recvBuf = new char[size[0]];
                if ((n = recvAll((long)param, recvBuf, size)) == 0) {
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

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Cannot create socket" << endl;
        return 0;
    }

    memset((char *)&siServer, 0, sizeof(siServer));
    siServer.sin_family = AF_INET;
    siServer.sin_port = htons(atoi(serverPort));

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

    void *status;
    rc = pthread_join(sendRecvThread, &status);

    pthread_exit(NULL);

    close(clientSocket);

    return 0;
}
