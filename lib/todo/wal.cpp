//#include "wal.hpp"
#include "globj.hpp"
#include "extra.hpp"

#include <iostream>

class counter :public templet::globj {
public:
	counter(templet::wal& wal) :globj(wal),_counter(0) { init(); }
public:
	unsigned inc(unsigned count) {
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
private:
	enum { _inc };
	void on_init() override { inc(0); }
};

int main()
{
	templet::memwal wal; 
    templet::job job(4);
    
	job([&](unsigned pid) {
        counter a_counter(wal);
        
    	a_counter.inc(1); templet::job::delay(1.0);
        
		if (pid == 0) {// in master 'process'	
			std::cout << "total number of jobs: " << a_counter.inc(0) << std::endl;
		}
	});
    
	std::cout << "Duration is " << job.duration() << " seconds." << std::endl;
}
