#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <unistd.h>

#include "socketIO.cc"

using namespace std;

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
    char buf[BUFLEN];
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


    // get user input
    string line;
    while(getline(cin, line)) {
        if (line.length() == 0) {
            continue;
        }

    	if (write(clientSocket, line.c_str(), line.length()) < 0) {
			cerr << "write failed" << endl;
			return 0;
		}

        char buf[BUFLEN];
        int n = 0;

        if ((n = read(clientSocket, buf, BUFLEN)) > 0) {
            buf[n] = 0;
            cout << "Server: " << buf << endl;
        }
    }

    close(clientSocket);
    return 0;
}
