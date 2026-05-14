#include "dbase.hpp"
#include "walimpl.hpp"
#include <limits>
#include <fstream>

int main(int argc, char*argv[]){
    if(argc!=2)return EXIT_FAILURE;
    std::ifstream in(argv[1]);
    
    std::string user_name;
    std::getline(in,user_name);
    
    std::string full_name; 
    std::getline(in,full_name);
    
    templet::server_side_wal wal(1000,1,std::string("dbase"),std::string("txt"));
    user_full_name ufname(wal);
    
    ufname.change_full_name(user_name,full_name);
    return EXIT_SUCCESS;
}