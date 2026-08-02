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
        std::cout << "Initial development with 'in memory' WAL ..." << std::endl;
        
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
        std::cout << "'Single deployment' WAL ..." << std::endl;

        templet::config conf("some params");
        
    	templet::srvwal swal(conf);
        if(conf.is_srv())swal.listen();
        
        templet::job job(conf.jobs_num());
        
    	job([&](unsigned pid) {
            templet::cliwal wal(swal);//or 'templet::cliwal wal; wal.connect(conf);' 
            if(!conf.is_srv()) wal.connect(conf);

            counter a_counter(wal);
        	a_counter.inc(1); templet::job::delay(1.0);
            
    		if (conf.is_master() && pid == 0) // in master 'process'	
    			std::cout << "total number of jobs: " << a_counter.inc(0) << std::endl;
    	});    
    	std::cout << "Duration is " << job.duration() << " seconds." << std::endl;
    }
}
