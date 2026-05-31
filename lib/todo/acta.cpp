#include "wal.hpp"
#include "acta.hpp"
#include "extra.hpp"

#include <iostream>

const int NUM_PROC = 10;
const int SIZE = 10;

class message:public templet::acta::message{
public:
    message():templet::acta::message(){}
    int n, nxn;
    unsigned iter;
    bool result;
};

class master:public templet::acta::actor{
public:
    master(templet::acta&acta,unsigned pid):templet::acta::actor(acta){
        _pid = pid;
        _cur_iter = 0;
        _num_processed = 0;
    }
    void in(message&m){ m.srv(this,[this,&m](){on_in(m);}); }
private:
    void on_in(message&m){
        task(_pid==0, // in master 'process'
            [this,&m](std::ostream&out){
                if(m.result){ NxN[m.iter]=m.nxn; _num_processed++; }
                if(_num_processed==SIZE){ stop(); return; }
                out << N[_cur_iter] << " " << _cur_iter;
                _cur_iter++;
            },
            [this,&m](std::istream&in){
                in >> m.n >> m.iter;
                ret(m);
            }
        );
    }
    unsigned _cur_iter;
    unsigned _num_processed;
    unsigned _pid;
public:
    std::vector<int> N;
    std::vector<int> NxN;
};

class worker:public templet::acta::actor{
public:
    worker():templet::acta::actor(),out(){}
    void init(templet::acta&acta){
        templet::acta::actor::init(acta,true);
        out.init(this,[this](){on_out(out);});
    }
    message out;
private:
    void on_out(message&m){
        task(
            [this,&m](std::ostream& out){ 
                out << m.n*m.n;
                templet::job::delay(1.0);//simulate workload
            },
            [this,&m](std::istream& in){ 
                in >> m.nxn;
                m.result = true; ask(m);
            }
        );
    }
    void on_start()override{
        out.result=false; 
        ask(out);
    }
};

int main()
{  
    templet::memwal wal;
    templet::job job(NUM_PROC);

    job([&](unsigned pid){
 
        templet::acta acta(wal);
        master a_master(acta,pid);
        std::vector<worker> workers(NUM_PROC);

        for(int i=0;i<NUM_PROC;i++){
            workers[i].init(acta);
            a_master.in(workers[i].out);
        }

        if(pid==0){// in master 'process'
            a_master.N.resize(SIZE); a_master.NxN.resize(SIZE);
            for(int i=0; i<SIZE; i++) a_master.N[i]=i+1;
        }
        
        acta();
        
        if(pid==0){// in master 'process'
            for(int i=0;i<SIZE;i++)
                std::cout << a_master.N[i] <<"^2 = " << a_master.NxN[i] << std::endl;
        }             
    });

    std::cout << "Duration with " << NUM_PROC << 
        " thread(s) is " << job.duration() << " seconds." << std::endl;
}
