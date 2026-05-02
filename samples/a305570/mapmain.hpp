#include <vector>
#include <iostream>
#include <sstream>

class mapper{
public:
    class engine{
    friend class mapper;
    protected:
        virtual void init(unsigned size){_size=size;on_init(size);}
        virtual void map(){
            for(int id=0;id<_size;id++){io_test(id,false);on_map(id);io_test(id,true);}}
    protected:
        void io_test(unsigned id,bool mapped){
            std::stringstream sstr;on_save(id,sstr,mapped);on_load(id,sstr,mapped);}
        unsigned _size;
    protected:
        void on_init(unsigned size){_mapper->on_init(size);}
        void on_map(unsigned id){_mapper->on_map(id);}
    	void on_save(unsigned id, std::ostream&out, bool mapped){
            _mapper->on_save(id,out,mapped);}
    	void on_load(unsigned id, std::istream&in, bool mapped){
            _mapper->on_load(id,in,mapped);}
    protected:
        mapper* _mapper;
    };
public:
    mapper(mapper::engine& eng):_engine(eng){eng._mapper=this;}
    void map(){_engine.map();}
    void init(unsigned size){_engine.init(size);}
protected:
    virtual void on_init(unsigned size) = 0;
    virtual void on_map(unsigned id) = 0;
	virtual void on_save(unsigned id, std::ostream&, bool mapped) = 0;
	virtual void on_load(unsigned id, std::istream&, bool mapped) = 0;
private:
    mapper::engine& _engine;
};

class dls7_mapper: public mapper{
public:
    dls7_mapper(mapper::engine& eng): mapper(eng){}
private:
    void on_init(unsigned size) override{
        N.resize(size); NxN.resize(size);
    }
    void on_map(unsigned id) override{
        NxN[id] = N[id] * N[id];
    }
    void on_save(unsigned id, std::ostream& out, bool mapped) override{
        if(mapped) out << NxN[id]; else out << N[id];  
    }
	void on_load(unsigned id, std::istream& in, bool mapped) override{
        if(mapped) in >> NxN[id]; else in >> N[id];
    }
public:
    std::vector<int> N;
    std::vector<int> NxN;
};