#include "mapmain.hpp"

int main(){
    bool master = true;
    
    const int NUM_OF_CHUNKS = 10;
    const std::string CHUNKS_DIR = "C:\\Users\\Сергей\\Downloads\\DLS\\";
    
    base_engine eng;
    dls7_mapper a_mapper(eng,CHUNKS_DIR,NUM_OF_CHUNKS);
    
    if(master) a_mapper.init((NUM_OF_CHUNKS*(1+NUM_OF_CHUNKS))/2);
   
    a_mapper.map();
   
    if(master){
        std::cout << "Number of diagonal Latin squares of order 7 with the first row in order" << std::endl
                  << "and at least one orthogonal diagonal mate = A305570(7) = " 
                  << a_mapper.get_mates_number() << std::endl;
        std::cout << "duration = " << eng.duration() << " s" << std::endl;
    }
}