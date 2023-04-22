#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>

using namespace std;

int main(int argc, char* argv[])
{
	int task;
	fstream task_file;
	vector<int> task_array;

	if (argc != 3) {
		cout << "usage: caller <the number of the task just completed>"
			" <existing file with completed tasks>" << '\n';
		return EXIT_FAILURE;
	}

	try
	{
		task = stoi(string(argv[1]));

		task_file.open(argv[2], ios_base::in);
		if (!task_file.is_open()) throw invalid_argument("bad task file argument");

		while (!task_file.eof()) {
			int x;	task_file >> x;
			if(task_file) task_array.push_back(x);
		}

		bool completed = false;
		for (const int x : task_array) {
			if (task == x) {
				completed = true;
				break;
			}
		}

		if (!completed && task >= 0) task_array.push_back(task);
		
		task_file.close();

		task_file.open(argv[2], ios_base::out);
		if (!task_file.is_open()) throw invalid_argument("bad task file argument");

		for (const int x : task_array) {
			cout << x << " ";
			task_file << x << " ";
		}
	}
	catch (exception const& ex)
	{
		cout << "what(): " << ex.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}