#include "general-classes.hpp"

#include <walimpl.hpp>
#include <syncmem.hpp>
#include <chrono>
#include <thread>
#include <atomic>

class sync_state_engine: public mapper::engine{
public:
    sync_state_engine(templet::write_ahead_log&wal):_tbag(wal,*this){}
    double duration(){ std::chrono::duration<double> dur = _end - _beg;
            return dur.count();}
private:
    void init(unsigned size)override{
        _beg=std::chrono::high_resolution_clock::now();
        _tbag.resize(size);
        for(int id=0; id<size; id++)_tbag.add(id);
    }
    void map()override{
        while(!_tbag.ready_to_get())/*wait*/;
        unsigned id;
        while(_tbag.get(id)){on_map(id);_tbag.put(id);}
        _end=std::chrono::high_resolution_clock::now();
    }
    class taskbag:public templet::state{
    friend class sync_state_engine;
        taskbag(templet::write_ahead_log&l,sync_state_engine&eng):
            state(l),_engine(eng) {init();}
        void resize(unsigned size){
            update(_resize, [&](std::ostream&out) {
                out << size;
    		},
    		[this](std::istream&in) {
                unsigned size; in >> size;
                _size = size; _engine.on_init(size);
                unprocessed.clear(); _ready_to_get = false;
    		});   
        }
        void add(unsigned id){
            update(_add, [&](std::ostream&out) {
                out << id << " "; _engine.on_save(id,out,false);
    		},
    		[this](std::istream&in) {
                unsigned id; in >> id;
                _engine.on_load(id,in,false);
                unprocessed.insert(id);
                if(unprocessed.size()==_size) _ready_to_get = true;
    		});     
        }
        bool ready_to_get(){
            update();
            return _ready_to_get;
        }
        bool get(unsigned& id){
            update(); 
            if(unprocessed.empty())return false;
            id = get_rand_unprocessed();  
            return true;
        }
        void put(unsigned id){
            update(_put, [&](std::ostream&out) {
                out << id << " "; _engine.on_save(id,out,true);
    		},
    		[this](std::istream&in) {
                unsigned id; in >> id; _engine.on_load(id,in,true);
                unprocessed.erase(id);
    		});  
        }
        enum {_resize,_add,_put};
    	void on_init() override {resize(0); add(0); put(0);}
        sync_state_engine& _engine;
        unsigned _size;
        std::set<unsigned> unprocessed;
        bool _ready_to_get;
        unsigned get_rand_unprocessed(){
            int selected = rand() % unprocessed.size();
    		auto it = unprocessed.begin(); 
            for (int i = 0; i != selected; i++, it++) {}
            return *it;
        }
    } _tbag;
    std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;
};

int main()
{
    const int NUM_PROC = 100;
    const int SIZE = 10000;
    
    //templet::write_ahead_log wal;
    templet::server_side_wal server_wal(10000,10,std::string("file"), std::string("txt"), true);
    templet::client_side_wal wal(10000,10,std::string("file"), std::string("txt"),server_wal);
    
    //////////////// 'process' simulation ///////////////////////////////
    std::atomic_int PID = 0;
    std::vector<std::thread> threads(NUM_PROC);
	for (auto& t : threads)t = std::thread([&] { int pid = PID++;
	//////////////// inside a 'process' /////////////////////////////////
    sync_state_engine eng(wal);
    throughput_test_mapper a_mapper(eng);
    
    if(pid==0){// in master 'process'
        a_mapper.N.resize(SIZE);
        for(int i=0;i<SIZE;i++) a_mapper.N[i] = i;
        a_mapper.init(SIZE);
    }
    a_mapper.map();
   
    if(pid==0){// in master 'process'
        for(int i=0;i<SIZE;i++){
            std::cout << a_mapper.N[i] <<"^2 = " << a_mapper.NxN[i] << std::endl;
            if(i>100){ std::cout << "..." << std::endl; break;}
        }
        std::cout << "Execution time (in sec): " << eng.duration() << std::endl;
    }                                             
    ////////////// outside a 'process' //////////////////////////////////
	}); for (auto& t : threads) t.join();
	std::cout << "Success!" << std::endl;
    /////////////////////////////////////////////////////////////////////
    return EXIT_SUCCESS;
}
