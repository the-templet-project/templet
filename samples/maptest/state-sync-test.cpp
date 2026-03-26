#include <walimpl.hpp>
#include <syncmem.hpp>
#include <chrono>

#include "general-classes.hpp"


class sync_state_engine: public mapper::engine{
public:
    sync_state_engine(int argc, char *argv[]):_tbag(_wal,*this){
        if(argc>1){_master=true; _num_workers=std::stoi(std::string(argv[1]));}
        else{_master=false; _num_workers=0;}
    }
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
                unprocessed.clear();
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
    		});     
        }
        bool ready_to_get(){
            update();
            return unprocessed.size()==_size;
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
        unsigned get_rand_unprocessed(){
            int selected = rand() % unprocessed.size();
    		auto it = unprocessed.begin(); 
            for (int i = 0; i != selected; i++, it++) {}
            return *it;
        }
    } _tbag;
    templet::write_ahead_log _wal;
    std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;
    bool _master;
    int  _num_workers;
};

int main()
{
    return EXIT_SUCCESS;
}
