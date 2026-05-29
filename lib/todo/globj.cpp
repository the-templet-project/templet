#include "wal.hpp"
#include "globj.hpp"
#include "extra.hpp"

#include <random>
#include <set>
#include <iostream>

const int NUM_PROC = 10;
const int SIZE = 10;

class bag_of_tasks:public templet::globj{
public:
    bag_of_tasks(templet::wal&l):globj(l) {
		std::random_device rd;_rand.seed(rd());init(); }
    void resize(unsigned size){//---------------------
        update(_resize, [&](std::ostream&out) {
            out << size;
		},
		[this](std::istream&in, std::ostream&) {
            unsigned size; in >> size;
            //----------------------------------------
            N.resize(size); NxN.resize(size);
            _unprocessed.clear(); _ready_to_get = false;
            //----------------------------------------
		});   
    }
    void add(unsigned id,int n){//--------------------
        update(_add, [&](std::ostream&out) {
            out << id << " " << n;
		},
		[this](std::istream&in, std::ostream&) {
            unsigned id; int n; in >> id >> n;
            //----------------------------------------
            N[id]=n; _unprocessed.insert(id);
            if(_unprocessed.size()==N.size()) _ready_to_get = true;
            //----------------------------------------
		});     
    }
    bool ready_to_get(){//------------------------
        update();
        //----------------------------------------
        return _ready_to_get;
        //----------------------------------------
    }
    bool get(unsigned& id, int& n){//-------------
        update();
        //----------------------------------------
        if(_unprocessed.empty())return false;
        id = get_rand_unprocessed(); n = N[id];
        return true;
        //----------------------------------------
    }
    void put(unsigned id,int nxn){//-----------------
        update(_put, [&](std::ostream&out) {
            out << id << " " << nxn;
		},
		[this](std::istream&in, std::ostream&) {
            unsigned id; int nxn; in >> id >> nxn;
            //----------------------------------------
            _unprocessed.erase(id);
            NxN[id]=nxn;
            //----------------------------------------
		});  
    }
    //----------------------------------------
public:
    std::vector<int> N;
    std::vector<int> NxN;
private:
    std::set<unsigned> _unprocessed;
    bool _ready_to_get;
private:
    std::minstd_rand _rand;
    unsigned get_rand_unprocessed(){
        int selected = _rand() % _unprocessed.size();
		auto it = _unprocessed.begin(); 
        for (int i = 0; i != selected; i++, it++) {}
        return *it;
    }
    //----------------------------------------
private:
	enum {_resize,_add,_put};
	void on_init() override {
		resize(0); add(0,0); put(0,0);
	}
};

int main()
{
    templet::wal wal;
    templet::job job(NUM_PROC);

    job([&](unsigned pid){

        bag_of_tasks tbag(wal);

        if(pid==0 && !tbag.ready_to_get()){// in master 'process'
            tbag.resize(SIZE);
            for(int i=0;i<SIZE;i++) tbag.add(i,i+1);
        }
        
        while(!tbag.ready_to_get())/*wait*/;
    
        unsigned id; int N, NxN; 
        while(tbag.get(id,N)){
            NxN = N*N;
            templet::job::delay(1.0);//simulate workload
            tbag.put(id,NxN);
        }
    
        if(pid==0){// in master 'process'
            for(int i=0;i<SIZE;i++)
                std::cout << tbag.N[i] <<"^2 = " << tbag.NxN[i] << std::endl;
        }             
    });

    std::cout << "Duration with " << NUM_PROC << 
        " thread(s) is " << job.duration() << " seconds." << std::endl;
}
