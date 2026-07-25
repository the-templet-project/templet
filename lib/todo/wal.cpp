#include "wal.hpp"
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
    {
        std::cout << "Development with 'in memory' WAL ..." << std::endl;
        
    	templet::memwal wal; 
        templet::job job(4);
        
    	job([&](unsigned pid) {
            counter a_counter(wal);
        	a_counter.inc(1); templet::job::delay(1.0); 
    		if (pid == 0) // in master 'process'	
    			std::cout << "total number of jobs: " << a_counter.inc(0) << std::endl;
    	});    
    	std::cout << "Duration is " << job.duration() << " seconds." << std::endl;
    }
    {
        std::cout << "Development with 'in file' WAL ..." << std::endl;

        templet::config conf("some params");
    	templet::server srv(conf); 
        templet::job job(4);
        
    	job([&](unsigned pid) {
            templet::client cli(srv);
            counter a_counter(cli);
        	a_counter.inc(1); templet::job::delay(1.0); 
    		if (pid == 0) // in master 'process'	
    			std::cout << "total number of jobs: " << a_counter.inc(0) << std::endl;
    	});    
    	std::cout << "Duration is " << job.duration() << " seconds." << std::endl;
    }
    {
        std::cout << "Development with 'in file' WAL and networking ..." << std::endl;

        templet::config conf("some params");
    	templet::server srv(conf);
        srv.listen();
        templet::job job(4);
        
    	job([&](unsigned pid) {
            templet::client cli(conf);
            counter a_counter(cli);
        	a_counter.inc(1); templet::job::delay(1.0); 
    		if (pid == 0) // in master 'process'	
    			std::cout << "total number of jobs: " << a_counter.inc(0) << std::endl;
    	});    
    	std::cout << "Duration is " << job.duration() << " seconds." << std::endl;
    }

}
