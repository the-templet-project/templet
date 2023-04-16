#include <iostream>
#include "../../lib/everest.hpp"

using namespace std;

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

        cout << out["completed_tasks"] << endl;
    }
    
    return EXIT_SUCCESS;
}