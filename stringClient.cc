#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
	if (argc != 1) {
		cout << "argc should be 1\n";
		return 0;
	}

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
}
