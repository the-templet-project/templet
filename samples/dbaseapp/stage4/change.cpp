#include "dbase.hpp"
#include "walimpl.hpp"

#include <limits>
#include <fstream>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <semaphore.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>

int main(int argc, char*argv[])
{
    if(argc!=2)return EXIT_FAILURE;
    std::ifstream in(argv[1]);

    sem_t *sem = sem_open("/dbase_txt_sem", O_CREAT, 0666, 1);
    
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    std::string user_name;
    
    // getting current user from the system
    uid_t uid = getuid(); // Get the real user ID of the calling process
    struct passwd *pw = getpwuid(uid);
    
    if (pw == NULL) {
        perror("getpwuid error or user not found");
        return 1;
    }

    user_name+=pw->pw_name;

    // getting exclusive lock to the dbase.txt
    sem_wait(sem);
    
    std::string full_name; 
    std::getline(in,full_name);
    
    templet::server_side_wal wal(1000,1,std::string("dbase"),std::string("txt"));
    user_full_name ufname(wal);

    ufname.change_full_name(user_name,full_name);
        
    sem_post(sem);
    sem_close(sem);
        
    return EXIT_SUCCESS;
}