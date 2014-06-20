#include <arpa/inet.h>
#include <iostream>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sstream>
#include <vector>

#include "socketIO.cc"

using namespace std;

int main(int argc, char *argv[]) {
    // exit if we don't have exactly 1 argument
    if (argc != 1) {
        cerr << "argc should be 1" << endl;
        return 0;
    }

    int serverSocket = 0;
    int maxSocketFd = 1;
    vector<int> clientSockets;
    struct sockaddr_in siServer;
    fd_set readfds;

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        cerr << "Cannot create master socket for listen" << endl;
        return 0;
    }

    // set server info
    memset((char *)&siServer, 0, sizeof(siServer));
    siServer.sin_family = AF_INET;
    siServer.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((bind(serverSocket, (const struct sockaddr*)&siServer, sizeof(struct sockaddr_in))) < 0) {
        cerr << "Bind failed" << endl;
        return 0;
    }

    char hostName[128];
    if (gethostname(hostName, sizeof(hostName)) < 0) {
        cerr << "Cannot get host name" << endl;
        return 0;
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(serverSocket, (struct sockaddr *)&sin, &len) < 0) {
        cerr << "Cannot get socket name" << endl;
        return 0;
    }

    cout << "SERVER_ADDRESS " << hostName << endl
         << "SERVER_PORT " << ntohs(sin.sin_port) << endl;

    if (listen(serverSocket, 5) < 0) {
        cerr << "lisen failed" << endl;
        return 0;
    }

    // Wait for message from client
    for (;;) {
        // clear read socket sets
        FD_ZERO(&readfds);

        // add server socket to the read set explicity
        FD_SET(serverSocket, &readfds);
        maxSocketFd = serverSocket + 1;

        // add all established connection sockets to read set
        for (vector<int>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it) {
            if (*it > 0) {
                FD_SET(*it, &readfds);
            }

            if (*it >= maxSocketFd) {
                maxSocketFd = *it + 1;
            }
        }

        // some error on select(), maybe need to dump errono
        if (select(maxSocketFd, &readfds, NULL, NULL, NULL) < 0) {
            cerr << "Select error" << endl;
        }

        if (FD_ISSET(serverSocket, &readfds)) {
            // New connection
            int fd = accept(serverSocket, (struct sockaddr*)NULL, NULL);
            if (fd < 0) {
                cerr << "Accept error" << endl;
                return 0;
            }
            clientSockets.push_back(fd);
        }

        for (vector<int>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it) {
            if (FD_ISSET(*it, &readfds)) {
                // Handle incomming message
                int n;
                int size[1];
                if ((n = read(*it, size, sizeof(int))) > 0) {
                    string output(*size, 0);
                    if ((n = read(*it, &output[0], *size)) > 0) {
                        cout << output << endl;

                        output[0] = toupper(output[0]);
                        for (int i = 1; i < n; i++) {
                            if (output[i - 1] == ' ') {
                                output[i] = toupper(output[i]);
                            } else {
                                output[i] = tolower(output[i]);
                            }
                        }

                        if (write(*it, size, sizeof(int)) < 0) {
                            cerr << "write failed" << endl;
                            return 0;
                        }
                        if (write(*it, output.c_str(), output.length()) < 0) {
                            cerr << "write failed" << endl;
                            return 0;
                        }
                    }
                }
            }
        }
    }
}
