#include <iostream>
#include <cstdlib>

using namespace std;

int main(int argc, char **argv) {
	if (argc != 1) {
		cout << "argc should be 1\n";
		return 0;
	}

	string hostName = getenv("HOSTNAME");
	// cout << hostName;
}
