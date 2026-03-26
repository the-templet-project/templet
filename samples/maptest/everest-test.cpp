#include <chrono>

#include "general-classes.hpp"

class everest_engine: public mapper::engine{
public:
    everest_engine(int argc, char *argv[]){ _master = (argc>1); }
    double duration(){ std::chrono::duration<double> dur = _end - _beg;
            return dur.count();}
private:
    void init(unsigned size)override
            {_beg=std::chrono::high_resolution_clock::now();
             _size=size;on_init(size);}
    void map()override{
        for(int id=0;id<_size;id++)
            {io_test(id,false); on_map(id); io_test(id,true);
             _end=std::chrono::high_resolution_clock::now();}
    }
    void io_test(unsigned id,bool mapped){
        std::stringstream sstr;
        on_save(id,sstr,mapped);on_load(id,sstr,mapped);
    }
    unsigned _size;
    std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;
    bool _master;
};

int main()
{
    //std::cout << "Execution time (in sec): " << eng.duration();
    return EXIT_SUCCESS;
}