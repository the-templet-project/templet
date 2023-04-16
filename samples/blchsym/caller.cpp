#include <iostream>
#include "../../lib/everest.hpp"

using namespace std;

int main(int argc, char* argv[])
{
	templet::everest_engine teng("--session-token--");

	if (!teng) { cout << "There was an error connecting to Everest." << endl; return EXIT_FAILURE; }

	cout << "Success!!!";

	return EXIT_SUCCESS;
}