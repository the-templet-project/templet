#pragma templet +header

#include "meta.hpp"
#include "globj.hpp"

class meta_bag_of_tasks: public templet::meta::globj {
public:
    meta_bag_of_tasks(){
        name("bag_of_tasks");

        def("resize")
            .in("unsigned size");

        def("add")
            .in("unsigned id")
            .in("int n");

        def("put")
            .in("unsigned id")
            .in("int nxn");
    }
};
#pragma templet -header

class bag_of_tasks:public templet::globj{
public:
    bag_of_tasks(templet::wal&l):globj(l) {
#pragma templet +bag_of_tasks.bag_of_tasks
        
#pragma templet -bag_of_tasks.bag_of_tasks
        init(); 
	}
    void resize(unsigned size){
        update(_resize, [&](std::ostream&out) {
#pragma templet +bag_of_tasks.resize.save
            
#pragma templet -bag_of_tasks.resize.save
		},
		[this](std::istream&in, std::ostream&) {
#pragma templet +bag_of_tasks.resize.update
            
#pragma templet -bag_of_tasks.resize.update
		});   
    }
    void add(unsigned id,int n){
        update(_add, [&](std::ostream&out) {
#pragma templet +bag_of_tasks.add.save
            
#pragma templet -bag_of_tasks.add.save
		},
		[this](std::istream&in, std::ostream&) {
#pragma templet +bag_of_tasks.add.update
            
#pragma templet -bag_of_tasks.add.update
		});     
    }
    void put(unsigned id,int nxn){
        update(_put, [&](std::ostream&out) {
#pragma templet +bag_of_tasks.put.save
            
#pragma templet -bag_of_tasks.put.save
		},
		[this](std::istream&in, std::ostream&) {
#pragma templet +bag_of_tasks.put.update
            
#pragma templet -bag_of_tasks.put.update
		});  
    }
private:
	enum {_resize,_add,_put};
	void on_init() override {
		resize(0);
        add(0,0);
        put(0,0);
	}
#pragma templet +bag_of_tasks.class
            
#pragma templet -bag_of_tasks.class
};

#pragma templet +footer
int main()
{
    meta_bag_of_tasks meta;
    meta.generate(__FILE__);
}
#pragma templet -footer