#include <vector>
#include <set>
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

class dls7_mapper: public mapper{
static const unsigned DIM=7;
public:
    dls7_mapper(mapper::engine& eng,const std::string&fdir,unsigned nchunks):
        mapper(eng),filedir(fdir),num_of_chunks(nchunks){
            for(int i=0; i<num_of_chunks; i++) tasks.push_back(std::pair(i,i));
            for(int i=0; i<num_of_chunks; i++)
            for(int j=i+1; j<num_of_chunks; j++)tasks.push_back(std::pair(i,j));
        }
public:
    struct sq7x7{
        unsigned sq[DIM*DIM];
        bool operator<(const sq7x7&arg)const{
            for(int i=0;i<DIM*DIM;i++)
                if(!(sq[i]<arg.sq[i])) return false;
            return true;
        }
    };
    unsigned get_mates_number(){return orto_mates_final.size();} 
private:
    void on_init(unsigned size) override{
       tasks.resize(num_of_chunks*(1+num_of_chunks)/2);
    }
    void on_map(unsigned id) override{
        if(tasks[id].first==tasks[id].second){
            load(first_ndls7_chunk,tasks[id].first);
            find_orto_in_one();
        }
        else{
            load(first_ndls7_chunk,tasks[id].first);
            load(second_ndls7_chunk,tasks[id].second);
            find_orto_in_two();
        }
    }
    void on_save(unsigned id, std::ostream& out, bool mapped) override{
        return;
        if(mapped){
            out << orto_mates_local.size() << ' ';
            for(auto& it:orto_mates_local)
                for(int i=0; i< DIM*DIM; i++) out << it.sq[i] << ' ';
            
        }
        else out << tasks[id].first << ' ' << tasks[id].second;
    }
	void on_load(unsigned id, std::istream& in, bool mapped) override{
        return;
        if(mapped){
            int size; in >> size;
            for(int i=0; i<size; i++){
                sq7x7 dls;
                for(int i=0; i< DIM*DIM; i++) in >> dls.sq[i]; 
                orto_mates_final.insert(dls);
            }
        }
        else in >> tasks[id].first >> tasks[id].second;     
    }
private:
    void load(std::vector<sq7x7>& chunk, unsigned chunk_id){
        chunk.clear();

        char name_buf[10];
		sprintf(name_buf, "nls%.2d.txt", chunk_id);
        std::string chunk_file_name = filedir + name_buf;
        
        std::ifstream file(chunk_file_name);
    	std::cout << "loading file " << chunk_file_name << std::endl;

        if(!file){
            std::cout << "Error!!! Cannot open file";
    		exit(EXIT_FAILURE);
        }
   
        for(;;){
            sq7x7 sqware; unsigned digit;
            for (int i = 0; i < DIM*DIM; i++){
                if (file >> digit) sqware.sq[i] = digit;
                else {
                    if (i == 0 && file.eof()){ 
                        std::cout << "file " << name_buf << " loaded" << std::endl;
                        return;//////////////////////////////
                    }
                    else {
                        std::cout << "Error!!! Bad file";
                        exit(EXIT_FAILURE);
                    }
                }
            }
            chunk.push_back(sqware);
        }
    }
    void find_orto_in_one(){
        orto_mates_local.clear();
        for (int m1 = 0; m1 < first_ndls7_chunk.size(); m1++)
			for (int m2 = m1 + 1; m2 < first_ndls7_chunk.size(); m2++)
				if (is_ortogonal_pair(first_ndls7_chunk[m1],first_ndls7_chunk[m2])) {
                    orto_mates_final.
            		/*orto_mates_local.*/insert(first_ndls7_chunk[m1]);
				}
    }
    void find_orto_in_two(){
        orto_mates_local.clear();
        for (int m1 = 0; m1 < first_ndls7_chunk.size(); m1++)
			for (int m2 = 0; m2 < second_ndls7_chunk.size(); m2++)
				if (is_ortogonal_pair(first_ndls7_chunk[m1],second_ndls7_chunk[m2])) {
                    orto_mates_final.
            		/*orto_mates_local.*/insert(first_ndls7_chunk[m1]);
				}
    }
    static bool is_ortogonal_pair(sq7x7 f, sq7x7 s){
    	unsigned my_set[DIM*DIM];
    	int cur_elem = 0;
    	for (int i = 0; i < DIM*DIM; i++) {
    		unsigned inserted = f.sq[i] * DIM + s.sq[i];
    		for (int j = 0; j < cur_elem; j++)
    			if (my_set[j] == inserted) return false;
    		my_set[cur_elem++] = inserted;
    	}
    	return true;
    }
    void save_dls(std::ostream& out,sq7x7 dls){
        for(int i=0; i< DIM*DIM; i++)out << dls.sq[i] << ' ';
    }
    void load_dls(std::istream& in,sq7x7& dls){
        for(int i=0; i< DIM*DIM; i++) 
            in >> dls.sq[i];
    }
private:
    std::string filedir;
    unsigned num_of_chunks;
    std::vector<std::pair<unsigned,unsigned>> tasks;
    std::set<sq7x7> orto_mates_local;
    std::set<sq7x7> orto_mates_final;
    std::vector<sq7x7> first_ndls7_chunk;
    std::vector<sq7x7> second_ndls7_chunk;
};