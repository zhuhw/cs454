#include <iostream>
#include <cstdlib>
#include <string>

using namespace std;

int main(int argc, char **argv) {
	if (argc != 1) {
		cout << "argc should be 1\n";
		return 0;
	}

	string serverAddress = getenv("SERVER_ADDRESS");
	if (serverAddress == NULL) {
		cout << "$SERVER_ADDRESS is not set";
		return 0;
	}
	string serverPort = getenv("SERVER_PORT");
	if (serverPort == NULL) {
		cout << "$SERVER_PORT is not set";
		return 0;
	}


}
