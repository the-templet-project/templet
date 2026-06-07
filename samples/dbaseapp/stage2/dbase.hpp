#include <string>
#include <map>
#include <iostream>
#include <iomanip>

#include "globj.hpp"

class user_full_name:public templet::globj{
public:
    user_full_name(templet::wal&w):globj(w) {init();}

    void change_full_name(const std::string&user,const std::string&fname){
         update(_change_full_name, [&](std::ostream&out) {
            out << user << std::endl << fname << std::endl;
		},
		[this](std::istream&in,std::ostream&) {
            std::string user, fname;
            getline(in,user); getline(in,fname);
            //--------------------------//
            user_fname_base[user] = fname;
            //--------------------------//
		}); 
    }
    void print(std::ostream&out){
        update();
        //-------------------------------------------------------------//
        out << std::left << std::setfill(' ') << std::setw(20)
                << "--user name-- " << "--user full name--" << std::endl;
        for(auto& it:user_fname_base){     
            out << std::left << std::setfill(' ') << std::setw(20)
                << it.first << it.second << std::endl;
        }
        //-------------------------------------------------------------//
    }
private:
    enum {_change_full_name};
	void on_init() override {
		change_full_name(std::string(""),std::string(""));
	}
private:
    //----------------------------------------------//
    std::map<std::string,std::string> user_fname_base;
    //----------------------------------------------//
};