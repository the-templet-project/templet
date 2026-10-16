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
				//	e.g. in("const std::string& str","stub_str","std::string stub_str")
				update& in(const char param_def[], const char stub_val[] = "0", const char stub_var_def[] = ""){
                    return *this;
                }
				//	e.g. out("bool& ret","stub_ret","bool stub_ret")
				update& out(const char param_def[], const char stub_val[] = "0", const char stub_var_def[] = ""){
                    return *this;
                }
				//	e.g. io("bool& ret","stub_ret","bool stub_ret")
				update& io(const char param_def[], const char stub_val[] = "0", const char stub_var_def[] = ""){
                    return *this;
                }
				//	e.g. ret("bool ret","ret","bool")
				update& ret(const char ret_var_def[], const char ret_var[],  const char ret_type[]){
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
