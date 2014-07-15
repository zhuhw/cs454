#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <iostream>

using namespace std;

void exit_and_close(int code, int sockfd){
    close(sockfd);
    exit(code);
}

int processRequests(int socket, fd_set active_fd_set){

}

int main() {
    struct sockaddr_in addr;
    addr.sin_port = 0;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    int sockfd = socket(AF_INET,SOCK_STREAM,0);

    // Bind
    if (bind(sockfd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) {
        cerr << "Error: fail to bind server." << endl;
        exit_and_close(-1, sockfd);
    }

    // Get hostname
    char hostname[255];
    if(gethostname(hostname,255)==-1) {
        cerr << "Error: fail to get hostname."<<endl;
    }
    cout << "BINDER_ADDRESS " << hostname <<endl;

    listen(sockfd, 0);

    // Get port number
    socklen_t len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr *)&addr, &len) == -1) {
        cerr << "Error: fail to get port number " << endl;
        exit_and_close(-1, sockfd);
    }
    cout << "BINDER_PORT " << ntohs(addr.sin_port) << endl;

    fd_set active_fd_set, read_fd_set;
    FD_ZERO (&active_fd_set);
    FD_SET (sockfd, &active_fd_set);
    int fdmax = sockfd;

    while (1) {
        read_fd_set = active_fd_set;
        if (select (fdmax+1, &read_fd_set, NULL, NULL, NULL) < 0) {
            cerr << "Error: select." << endl;
            exit_and_close(-1, sockfd);
        }

        // Loop through all sockets with input pending
        for (int i = 0; i <= fdmax; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                if (i == sockfd){
                    // request on main socket, create a new socket
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    int newsockfd = accept(sockfd, (struct sockaddr *) &client, &len);

                    if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                    }

                    FD_SET (newsockfd, &active_fd_set);
                } else {


                    // If on a client socket
                    // char* request_msg;

                    // receive all messages
                    // if (recvall(i, request_msg) < 0) {
                    //     close(i); // close socket
                    //     FD_CLR(i, &active_fd_set); // remove from active set
                    // } else {
                    //     string message = title_case(string(request_msg));

                    //     if(sendall(i, message)){
                    //         cerr << "Error on sending message" <<endl;
                    //         exit_and_close(-1, sockfd);
                    //     }

                    //     delete [] request_msg;
                    // }

                }
            }
        }
    }

    close(sockfd);
}