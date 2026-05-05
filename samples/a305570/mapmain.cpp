#include "mapmain.hpp"

int main(){
    bool master = true;
    
    const int NUM_OF_CHUNKS = 1;
    const std::string CHUNKS_DIR = "";
    
    base_engine eng;
    dls7_mapper a_mapper(eng,CHUNKS_DIR,NUM_OF_CHUNKS);
    
    if(master) a_mapper.init((NUM_OF_CHUNKS*(1+NUM_OF_CHUNKS))/2);
   
    a_mapper.map();
   
    if(master){
        std::cout << "number of ortogonal dls(7) = " 
            << a_mapper.orto_dls7_final.size() << std::endl;
        std::cout << "duration = " << eng.duration() << " s" << std::endl;
    }
}