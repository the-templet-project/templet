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

	system("clear");

	std::cout << "Введите номер желаемого действия + <Enter>" << std::endl;
    std::cout << "1. Изменить полное имя пользователя" << std::endl;
    std::cout << "2. Распечатать список пользователей и их полных имен" << std::endl;
    
	char ch;    
    std::cin >> ch; std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	std::string full_name;

    switch(ch){
        case '1':{         
            std::cout << "Пользователь " << user_name << "!" <<std::endl;
            std::cout << "Пожалуйста, введите свое полное имя:" << std::endl;
            std::getline(std::cin,full_name);      
            break;
        }
    }

	sem_wait(sem);// getting exclusive lock to the dbase.txt
	templet::server_side_wal wal(1000, 1, std::string("dbase"), std::string("txt"));
	user_full_name ufname(wal);

	switch (ch) {
		case '1': {
			ufname.change_full_name(user_name, full_name);
			break;
		}
		case '2': {
			ufname.print(std::cout);
			break;
		}
	}

	sem_post(sem);
    sem_close(sem);

	return EXIT_SUCCESS;
}