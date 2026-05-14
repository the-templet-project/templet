#include "dbase.hpp"
#include "walimpl.hpp"

int main(int argc, char*argv[]){
    if(argc!=2)return EXIT_FAILURE;
    std::ofstream in(argv[1]);
    
    templet::server_side_wal wal(1000,1,std::string("dbase"),std::string("txt"));
    user_full_name ufname(wal);
    
    ufname.print(in);
    return EXIT_SUCCESS;
}