#pragma once

/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#include <functional>
#include <istream>
#include <ostream>

namespace templet {

	namespace meta {

		class processor {};

		class globj {
		public:
			class update {
            public:
				update& in(const char param[], const char stub_value[] = "0", const char stub_value_def[] = ""){
                    return *this;
                }
				update& out(const char param[], const char stub_value[] = "0", const char stub_value_def[] = ""){
                    return *this;
                }
				update& ret(const char type[], const char ret_value[] = "0", const char ret_value_def[] = ""){
                    return *this;
                }
			};
		public:
			void name(const char name[]){}
			void pref(const char prefix[]){}
			update& def(const char name[]){
                return *(new update);
            }
		public:
			void generate(const char file[]){}
		};

		class acta {
		public:
			class actor {
            public:
				actor& start(){
                    return *this;
                }
				actor& in(const char message[], const char name[]){
                    return *this;
                }
				actor& out(const char message[], const char name[]){
                    return *this;
                }
				actor& task(const char name[]){
                    return *this;
                }
			};
		public:
			void name(const char name[]){}
			void pref(const char prefix[]){}
			actor& def(const char name[]){
                return *(new actor);
            }
		
		public:
			void generate(const char file[]){}
		};
	}
}
