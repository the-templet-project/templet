#include <iostream>
#include "../../lib/everest.hpp"

using namespace std;

int main(int argc, char* argv[])
{
	templet::everest_engine teng("vostokinsv", "SergeyVostokin1");

	if (!teng) { cout << "There was an error connecting to Everest." << endl; return EXIT_FAILURE; }

	cout << "Success!!!";

	return EXIT_SUCCESS;
}