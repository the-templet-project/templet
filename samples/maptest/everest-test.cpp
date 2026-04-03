#include "general-classes.hpp"
#include <everest.hpp>

#include <chrono>
#include <fstream>
#include <sstream>

class everest_engine: public mapper::engine{
public:
    everest_engine(int argc, char *argv[]){         
        if(argc>1){_master=true; _num_workers=std::stoi(std::string(argv[1]));}
        else{_master=false; _num_workers=0;} 
    }
    double duration(){ std::chrono::duration<double> dur = _end - _beg;
            return dur.count();}
    bool master(){ return _master; }
private:
    void init(unsigned size)override{
        if(_master){
            _beg=std::chrono::high_resolution_clock::now();
            _size=size;on_init(size);
        }
    }
    void map()override{
        if(_master){
            int base_chunk_size = _size / _num_workers;
            int rest = _size % _num_workers;
            int from = 0;
            int to;

            templet::everest_engine teng("90r09vccwydt34u8tz0e8qmfxvukj31qcud177fs5pk6srfcfug4d83ehyvpfsdv");
            if (!teng) {
        		std::cout << "...task engine not connected..." << std::endl;
        		exit(EXIT_FAILURE);
        	}
            std::vector<templet::everest_task> task(_num_workers);
            
            for(unsigned w=0; w<_num_workers; w++){
                to = from + (base_chunk_size-1) + (rest?1:0); if(rest)rest--;
                if(from > to){ _num_workers = w; break;}
    
                task[w].app_id("69cd58ce1e00005d2ea17d90");
        		task[w].engine(teng);
                
                json in;
                in["name"] = std::string("everest-test-")+
                    std::to_string(from)+std::string("-")+std::to_string(to);

                std::stringstream sstr;
                sstr << _size << " " << from << " " << to << " ";
                for(int id=from;id<=to;id++){on_save(id,sstr,false);sstr<<" ";}
                
        		in["inputs"]["input"]=sstr.str();

                if(!task[w].submit(in)){
                    std::cout << "...task submit error..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    templet::everest_error cntxt;
                    teng.error(&cntxt);
                    if(*cntxt._type==templet::everest_error::SUBMIT_FAILED){
                        json input = json::parse(cntxt._task_input);
                        cntxt._task->resubmit(input);
                    }
                    if(teng.error(&cntxt)){
                        std::cout << "...submit recover error..." << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                from = to + 1;
            }
            teng.run();
            
            templet::everest_error cntxt;
        	if (teng.error(&cntxt)) {
        		std::cout << "...task engine error..." << std::endl;
        		templet::everest_engine::print_error(&cntxt);
        		exit(EXIT_FAILURE);
        	}
            for (unsigned w=0; w<_num_workers; w++) {
                json out = task[w].result();
                std::string str_out = out["output"].get<string>();
                std::stringstream sstr(str_out);
                int from, to;
                sstr >> from; sstr >> to; 
                for(int id=from;id<=to;id++)on_load(id,sstr,true);
            }
            _end=std::chrono::high_resolution_clock::now();
        }
        else{//worker
            std::ifstream in("in.txt"); std::ofstream out("out.txt");
            in >> _size; on_init(_size);
            unsigned from; in >> from; out << from << " ";
            unsigned to; in >> to; out << to << " ";
            
            for(int id=from;id<=to;id++){
                on_load(id,in,false); on_map(id); on_save(id,out,true); out<<" ";
            }
        }
    }
unsigned _size;
    std::chrono::time_point<std::chrono::high_resolution_clock> _beg, _end;
    unsigned _num_workers;
    bool     _master;
};

int main(int argc, char *argv[])
{
    //set NUM_PROC as a command line argument
    const int SIZE = 1500;

    everest_engine eng(argc,argv);
    throughput_test_mapper a_mapper(eng);
    
    if(eng.master()){// in master 'process'
        a_mapper.N.resize(SIZE);
        for(int i=0;i<SIZE;i++) a_mapper.N[i] = i;
        a_mapper.init(SIZE);
    }
    a_mapper.map();
   
    if(eng.master()){// in master 'process'
        for(int i=0;i<SIZE;i++){
            std::cout << a_mapper.N[i] <<"^2 = " << a_mapper.NxN[i] << std::endl;
            if(i>100){ std::cout << "..." << std::endl; break;}
        }
        std::cout << "Execution time (in sec): " << eng.duration() << std::endl;
    }             
    return EXIT_SUCCESS;
}