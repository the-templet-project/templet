#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>

#include "../../lib/everest.hpp"

using namespace std;

const unsigned RND_SEED = 0;
const int NUM_WORKERS = 20;
const int NUM_TASKS = 20;
const std::chrono::seconds TASK_DELAY_SEC(5);

int EXEC_ORDER[NUM_WORKERS][NUM_TASKS];
int cur_task_in_EXEC_ORDER;
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
    
    cur_task_in_EXEC_ORDER = 0;
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

bool check_completed(int task)
{
    bool completed = false;
	for (const int x : solved_task_array) {
		if (task == x) {
			completed = true;
			break;
		}
	}
    return completed;
}

int main(int argc, char* argv[])
{
	templet::everest_engine teng("ypoudxeb5wvz41uzv6roh5q58t2qgrf9bqzy1mmhbawy5s3um2h6muxkyx30imm2");
    
    templet::everest_task task(teng,"643a76ae14000084cc249ad7");
    
    json in;    
	in["name"] = "blockch-application";
    
    for(int i=0; i<10; i++){

        in["inputs"]["task"] = i;
        task.submit(in);

        teng.run();

        templet::everest_error cntxt;

        if (teng.error(&cntxt)) {
            teng.print_error(&cntxt);
            return EXIT_FAILURE;
        }

        json out = task.result();

        fill_in_solved_task_array(out["completed_tasks"].get<string>());
        
        for(int x:solved_task_array) cout << x << " "; cout << endl;
        
        cout << out["completed_tasks"] << endl;
    }
    
    return EXIT_SUCCESS;
}