#include <vector>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <cstdio>
#include <chrono>

class mapper{
public:
    class engine{
    friend class mapper;
    private:
        virtual void init(unsigned size)=0;
        virtual void map()=0;
    protected:
        void on_init(unsigned size){_mapper->on_init(size);}
        void on_map(unsigned id){_mapper->on_map(id);}
    	void on_save(unsigned id, std::ostream&out, bool mapped){
            _mapper->on_save(id,out,mapped);}
    	void on_load(unsigned id, std::istream&in, bool mapped){
            _mapper->on_load(id,in,mapped);}
    private:
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

class base_engine: public mapper::engine {
    void init(unsigned size)override{
        _beg=std::chrono::high_resolution_clock::now();
        _size=size;on_init(size);}
    void map()override{
        for(int id=0;id<_size;id++){io_test(id,false);on_map(id);io_test(id,true);}
        _end=std::chrono::high_resolution_clock::now();}
    void io_test(unsigned id,bool mapped){
        std::stringstream sstr;on_save(id,sstr,mapped);on_load(id,sstr,mapped);}
    unsigned _size;
    std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;
public:
    double duration(){ std::chrono::duration<double> dur = _end - _beg;
        return dur.count();}
};

class sextuples_mapper: public mapper{
public:
    sextuples_mapper(mapper::engine& eng,int nslots,long long srange):
        mapper(eng),prime_limit(3),NUMBER_OF_TASK_SLOTS(nslots),SEARCH_RANGE_LIMIT(srange){
        }
public:
    std::vector<std::list<long long>> primesixs;
private:
    void on_init(unsigned size) override{
        primesixs.resize(NUMBER_OF_TASK_SLOTS);
    }
    void on_map(unsigned id) override{
        long long max_prime_needed = chank_tag_to_max_verif_prime(id);
        if(prime_limit < max_prime_needed)
            prime_limit = extend_prime_table(prime_limit,max_prime_needed);
        long long from, to;
        chank_tag_to_range(id,from,to);
        find_sextuplets_in_range(from,to,primesixs[id]);
    }
    void on_save(unsigned id, std::ostream& out, bool mapped) override{
        if(mapped){
            out << primesixs[id].size() << " ";
            for(auto& it:primesixs[id]) out << it << " ";
        }
    }
	void on_load(unsigned id, std::istream& in, bool mapped) override{
        if(mapped){
            int size; in >> size; long long x;
            primesixs[id].clear();
            for(int i=0;i<size;i++){in >> x; primesixs[id].push_back(x);}
        }
    }
private:
    long long chank_tag_to_max_verif_prime(int tag){
        const long long CHANK_SIZE = SEARCH_RANGE_LIMIT / NUMBER_OF_TASK_SLOTS; 
        long long to = (tag < NUMBER_OF_TASK_SLOTS-1) ? tag * CHANK_SIZE + CHANK_SIZE : SEARCH_RANGE_LIMIT;
        return sqrt((double)to)+1; 
    }
    long long extend_prime_table(long long from, long long to){
        for(;;from+=2){
            if(is_prime(from)){
                prime_table.push_back(from);
                if(from>=to) break;
            }
        }
        return from;
    }
    void chank_tag_to_range(int tag,long long& from,long long& to){
        const long long CHANK_SIZE = SEARCH_RANGE_LIMIT / NUMBER_OF_TASK_SLOTS; 
        from = tag * CHANK_SIZE;
        to = (tag < NUMBER_OF_TASK_SLOTS-1) ? from + CHANK_SIZE - 1 : SEARCH_RANGE_LIMIT;
        if(from-15 > 2) from-=15;
        if(from==0)     from=3;
    }
    void find_sextuplets_in_range(long long from,long long to,std::list<long long>&found){
        int count = 0;
        
        long long n = from;
        long cur_size = 0;
        long long last_5[5]={0};
        found.clear();
        
    	while (n <= to){
            if(is_prime(n)){
                if( 
                    ++cur_size > 5 &&
                    n==last_5[4]+4 &&
                    n==last_5[3]+6 &&
                    n==last_5[2]+10 &&
                    n==last_5[1]+12 &&
                    n==last_5[0]+16                 
                ){ 
                    found.push_back(last_5[0]);
                    std::cout << "a022008() = " << last_5[0] << std::endl; 
                }
                
                last_5[0]=last_5[1];
                last_5[1]=last_5[2];
                last_5[2]=last_5[3];
                last_5[3]=last_5[4];
                last_5[4]=n;
            }
            n+=2;
        }    
    }
    bool is_prime(long long num)
    {
    	for (long long divisor:prime_table) {
    		std::ldiv_t dv = std::ldiv(num, divisor);
    		if (dv.rem == 0) return false;
    		if (dv.quot <= divisor) break;
    	}
    	return true;
    }
private:
    std::list<long long> prime_table;
    long long prime_limit;
    int  NUMBER_OF_TASK_SLOTS;
    long long SEARCH_RANGE_LIMIT;
};