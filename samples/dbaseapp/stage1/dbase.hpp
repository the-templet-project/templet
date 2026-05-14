#include <string>
#include <map>
#include <iostream>
#include <iomanip>

class user_full_name{
public:
    void change_full_name(std::string&user,std::string&fname){
        user_fname_base[user] = fname;
    }
    void print(std::ostream&out){
        out << std::left << std::setfill(' ') << std::setw(20)
                << "--user name-- " << "--user full name--" << std::endl;
        for(auto& it:user_fname_base){     
            out << std::left << std::setfill(' ') << std::setw(20)
                << it.first << it.second << std::endl;
        }
    }
private:
    std::map<std::string,std::string> user_fname_base;
};