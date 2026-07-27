#pragma templet +header

#include "meta.hpp"
#include "acta.hpp"

class meta_bag_of_tasks: public templet::meta::acta {
public:
    meta_bag_of_tasks(){
        name("bag_of_tasks");

        def("master")
            .in("message", "in");

        def("worker")
            .start()
            .out("message", "out");
    }
};
#pragma templet -header

class bag_of_tasks : public templet::acta {
public:
    bag_of_tasks(templet::wal&l):templet::acta(l){
#pragma templet +bag_of_tasks.bag_of_tasks
        
#pragma templet -bag_of_tasks.bag_of_tasks
    }
public:
    class message:public templet::acta::message{
#pragma templet +message

#pragma templet -message
    };
    
    class master:public templet::acta::actor{
    public:
        master():templet::acta::actor(){
#pragma templet +master.master
            
#pragma templet -master.master            
        }
        master(templet::acta&acta):templet::acta::actor(acta){master();}
        void init(templet::acta&acta){
            templet::acta::actor::init(acta);
#pragma templet +master.init
            
#pragma templet -master.init
        }
        void in(message&m){ m.srv(this,[this,&m](){on_in(m);}); }
    private:
        void on_in(message&m){
            p_in = &m;
#pragma templet +master.on_in
            
#pragma templet -master.on_in
        }
        message* p_in;
#pragma templet +master.class
            
#pragma templet -master.class
    };
    
    class worker:public templet::acta::actor{
    public:
        worker():templet::acta::actor(),out(){
#pragma templet +worker.worker
            
#pragma templet -worker.worker
        }
        worker(templet::acta&acta):templet::acta::actor(acta,true),out(){worker();}
        void init(templet::acta&acta){
            templet::acta::actor::init(acta,true);
            out.init(this,[this](){on_out(out);});
#pragma templet +worker.init

#pragma templet -worker.init
        }
    private:
        void on_out(message&m){
#pragma templet +worker.on_out

#pragma templet -worker.on_out
        }
        void on_start()override{
#pragma templet +worker.on_start

#pragma templet -worker.on_start   
        }
        message out;
#pragma templet +worker.class

#pragma templet -worker.class
    };
private:
    void on_run()override{
#pragma templet +bag_of_tasks.on_run
        
#pragma templet -bag_of_tasks.on_run     
    }
#pragma templet +bag_of_tasks.class
        
#pragma templet -bag_of_tasks.class     
};

int main()
{  
    meta_bag_of_tasks meta;
    meta.generate(__FILE__);
}
