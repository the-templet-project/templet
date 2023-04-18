#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>

#include "../../lib/everest.hpp"

using namespace std;

const unsigned RND_SEED = 0;
const int NUM_WORKERS = 20;
const int NUM_TASKS = 20;
const int TASK_DELAY_IN_SEC = 5;

const std::chrono::seconds RESUB_DELAY_SEC(1);
const std::chrono::seconds TASK_DELAY_SEC(TASK_DELAY_IN_SEC);

int EXEC_ORDER[NUM_WORKERS][NUM_TASKS];
int this_worker_in_EXEC_ORDER;

vector<int> solved_task_array;

void init(){
    srand(RND_SEED);    
    
    for(int i=0; i<NUM_WORKERS; i++){
        for(int j=0; j<NUM_TASKS; j++){
redogen:                
            EXEC_ORDER[i][j] = rand()%NUM_TASKS;
            for(int k=0; k<j; k++) if(EXEC_ORDER[i][j]==EXEC_ORDER[i][k]) goto redogen;
        }
    }

    solved_task_array.clear();
}

void fill_in_solved_task_array(const string& s)
{
    istringstream instream(s);
    solved_task_array.clear();
    
    while (!instream.eof()) {
		int x;	instream >> x;
		if(instream) solved_task_array.push_back(x);
	}
}

bool task_solved(int task)
{
    bool solved = false;
	for (const int x : solved_task_array) {
		if (task == x) {
			solved = true;
			break;
		}
	}
    return solved;
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		cout << "Usage: caller <process ID=0..NUM_WORKERS-1> <session token>" << endl;
		return EXIT_FAILURE;
	}

	templet::everest_engine teng(argv[2]);
	templet::everest_task task(teng,"643a76ae14000084cc249ad7");
	templet::everest_error cntxt;

	try {
		string str(argv[1]);
		this_worker_in_EXEC_ORDER = stoi(str);
		if (!(0 <= this_worker_in_EXEC_ORDER && this_worker_in_EXEC_ORDER < NUM_WORKERS))
			throw invalid_argument("Process ID is out of range");
	}
	catch (std::out_of_range const& ex) {
		cout << ex.what() << endl;
		return EXIT_FAILURE;
	} 
	
    json in, out;    
	in["name"] = "blockchain-sym-app";
    
	init();

	bool execution_started = false;
	auto start_time=chrono::high_resolution_clock::now();

	for (int i = 0; i < NUM_TASKS; i++) {
		int task_num = EXEC_ORDER[this_worker_in_EXEC_ORDER][i];
		
		if (task_solved(task_num)) continue;

		in["inputs"]["task"] = task_num;
		std::this_thread::sleep_for(TASK_DELAY_SEC);
        task.submit(in);

resubmit:
		teng.run();

		if (teng.error(&cntxt)) {

			if (execution_started) {
				cout << "Double error: exiting the app" << endl;
				return EXIT_FAILURE;
			}

			teng.print_error(&cntxt);
			cout << "-------------------------" << endl;
			cntxt._task->resubmit(in);
			std::this_thread::sleep_for(RESUB_DELAY_SEC);
			goto resubmit;
		}

		if (!execution_started) {
			execution_started = true;
			start_time = chrono::high_resolution_clock::now() - TASK_DELAY_SEC;
		}

		out = task.result();
		fill_in_solved_task_array(out["completed_tasks"].get<string>());
		for (int x : solved_task_array) cout << x << " "; cout << endl;

	}

	auto end_time = chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end_time - start_time;
    
	cout << "Success !!!" << endl;
	cout << "App executon time is " << duration.count() << " seconds" << endl;
    cout << "Speedup is " << (DELAY * NUM_TASKS) / duration << endl;
    cout << "Number of worker processes is " << NUM_WORKERS << endl;
    cout << "Efficiency is " << DELAY * NUM_TASKS / duration / NUM_WORKERS << endl;
    
    return EXIT_SUCCESS;
}
