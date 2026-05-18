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
    sem_t *sem = sem_open("/dbase_txt_sem", O_CREAT, 0666, 1);
    
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }

    std::string user_name;
    
    uid_t uid = getuid(); // Get the real user ID of the calling process
    struct passwd *pw = getpwuid(uid);
    
    if (pw == NULL) {
        perror("getpwuid error or user not found");
        return 1;
    }

    user_name+=pw->pw_name; // getting current user from the system

    templet::server_side_wal wal(1000,1,std::string("dbase"),std::string("txt"));
    user_full_name ufname(wal);

    for(;;){
        system("clear");
        std::cout << "Введите номер желаемого действия + <Enter>" << std::endl;
        std::cout << "1. Изменить полное имя пользователя" << std::endl;
        std::cout << "2. Распечатать список пользователей и их полных имен" << std::endl;
        std::cout << "3. Выйти из программы" << std::endl;
        char ch;
        std::cin >> ch; std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        switch(ch){
            case '1':{
                std::string full_name; 
                std::cout << "Пользователь " << user_name << "!" <<std::endl;
                std::cout << "Пожалуйста, введите свое полное имя:" << std::endl;
                std::getline(std::cin,full_name);
                   
                // getting exclusive lock to the dbase.txt
                sem_wait(sem); ufname.change_full_name(user_name,full_name); sem_post(sem);        
                break;
            }
            case '2':{
                // getting exclusive lock to the dbase.txt
                sem_wait(sem); ufname.print(std::cout); sem_post(sem);
                std::cout << "---" <<std::endl;
                std::cout << "Введите любой символ + <Enter> чтобы продолжить" <<std::endl;
                char ch; std::cin >> ch;
                break;
            }
            case '3':{
                goto exit_for;
                break;
            }
        }
    }
exit_for:       
    sem_close(sem);
    return EXIT_SUCCESS;
}