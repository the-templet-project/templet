#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>

#include "../../lib/everest.hpp"

using namespace std;

/////////////////////////////////////////////////////////
const int NUM_TASKS = 10;        // 10  20  30  40  50 //
const int TASK_DELAY_IN_SEC = 10;// 10  20  30  40  50 //
/////////////////////////////////////////////////////////

const unsigned RND_SEED = 0;
const int NUM_WORKERS = 10;

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
	if (argc != 2) {
		cout << "Usage: caller <process ID=0..NUM_WORKERS-1>" << endl;
		return EXIT_FAILURE;
	}

	templet::everest_engine teng("login","password");
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
    in["inputs"]["task"] = -1;
    
	init();    

    task.submit(in);
    int atteptNo = 0;

wait_for_agent_to_start:
    teng.run();
    
    if (teng.error(&cntxt)) {
        teng.print_error(&cntxt); cout << endl;
        this_thread::sleep_for(RESUB_DELAY_SEC);
        cout << "Restart attempt # " << ++atteptNo << endl;
        cntxt._task->resubmit(in); goto wait_for_agent_to_start;        
    }
    
    cout << "Execution started !!!" << endl;
    
    auto start_time=chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_TASKS; i++) {
		int task_num = EXEC_ORDER[this_worker_in_EXEC_ORDER][i];
		
		if (task_solved(task_num)) continue;

		in["inputs"]["task"] = task_num;
		std::this_thread::sleep_for(TASK_DELAY_SEC);
        task.submit(in);

		teng.run();

		if (teng.error(&cntxt)) {
			cout << "Error: exiting the app" << endl;
            teng.print_error(&cntxt);
			return EXIT_FAILURE;
		}

		out = task.result();
		fill_in_solved_task_array(out["completed_tasks"].get<string>());
		for (int x : solved_task_array) cout << x << " "; cout << endl;
	}

	auto end_time = chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end_time - start_time;
    
	cout << "Success !!!" << endl;
    
    cout << "Number of tasks   is " << NUM_TASKS << endl;
    cout << "Task delay        is " << TASK_DELAY_IN_SEC << " seconds" << endl;
	cout << "App executon time is " << duration.count() << " seconds" << endl;
    cout << "Speedup is " << (double)(TASK_DELAY_IN_SEC * NUM_TASKS) / duration.count() << endl;
    
    return EXIT_SUCCESS;
}
