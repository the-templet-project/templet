#include "wal.hpp"
#include "glob.hpp"
#include "opt.hpp"

#include <iostream>

class counter :public templet::glob {
public:
	counter(templet::wal& wal) :glob(wal),_counter(0),_go_flag(false) { init(); }
    counter(const std::string& filename) :glob(filename),_counter(0),_go_flag(false) { init(); }
public:
    void go(){ //{_go_flag = true;}
		update(_go,
			[this](std::istream&,std::ostream&) {
                _go_flag = true;
			});
    }
    void wait_go(){
        update();
        while(!_go_flag){
            templet::env::delay(0.5);
            update();
        }
    }
	unsigned inc(unsigned count) { //{_counter += count;return _counter;}
        unsigned ret;
		update(_inc, [&](std::ostream&out) {
			out << count;
			},
			[this](std::istream&in, std::ostream&out) {
				unsigned count; in >> count;
				_counter += count;
                out << _counter;
			},
            [&](std::istream&in) {
    			in >> ret;
		});
        return ret;
	}
private:
	unsigned _counter;
    bool _go_flag;
private:
	enum { _go,_inc };
	void on_init() override { go(); inc(0); }
};

int main(int argc, char* argv[])
{   
    bool master=true;
    templet::cfg cfg;
    templet::env env(cfg);

    cfg.file(std::string("wal.dat"));
    cfg.pool(10);
    
    /* uncomment for distributed testing
    if(argc==1){
        master = true;    
        cfg.srv();
    }
    else if(argc>=2){
        master = false;
        cfg.cli(std::string(argv[1]),51717);
    }
    */
    
    // uncomment for local loop testing
    //cfg.srv();cfg.cli(std::string("localhost"),51717);

    std::mutex mut;
    env([&](unsigned pid,templet::wal& wal) {
        
        counter a_counter(wal); //using 'a_counter' as a global object

        if (pid == 0 && master){ // in master 'process'
            std::cout << "when all workers are ready, enter something here >";
            std::string str; std::cin >> str;
            a_counter.go();
        }
        else // in worker 'process'
            a_counter.wait_go();
            
        a_counter.inc(1); templet::env::delay(1.0); 
        { 
            std::unique_lock<std::mutex> lock(mut);
            std::cout << "local process #" << pid << " finished" << std::endl;
        }
    });

    if(master){
        counter a_counter(std::string("wal.dat")); //using 'a_counter' as a local object
        std::cout << "total number of processes: " << a_counter.inc(0) << std::endl;
    }  
 }
