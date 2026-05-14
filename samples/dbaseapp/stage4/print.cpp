#include "dbase.hpp"
#include "walimpl.hpp"

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <stdio.h>

int main(int argc, char*argv[])
{
    if(argc!=2)return EXIT_FAILURE;
    std::ofstream in(argv[1]);

    sem_t *sem = sem_open("/dbase_txt_sem", O_CREAT, 0666, 1);
    
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    // getting exclusive lock to the dbase.txt
    sem_wait(sem);
    
    templet::server_side_wal wal(1000,1,std::string("dbase"),std::string("txt"));
    user_full_name ufname(wal);
    
    ufname.print(in);

    sem_post(sem);
    sem_close(sem);
    
    return EXIT_SUCCESS;
}