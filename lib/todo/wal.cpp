#include "wal.hpp"
#include "globj.hpp"

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
    bool master = true;
    
    templet::cfg cfg; 
    templet::env env(cfg);

    // setting cfg parameters
    cfg.mode(templet::cfg::type::mem);
    cfg.numtasks(templet::cfg::SYSDEF);

    env([&](unsigned pid,templet::wal& wal) {
        counter a_counter(wal);
        a_counter.inc(1); templet::env::delay(1.0); 
        
        if (pid == 0 && master) // in master 'process'	
            std::cout << "total number of processes: " << a_counter.inc(0) << std::endl;
    });
 }
