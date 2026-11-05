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

		class glob {
        public:
			void name(const char name[]){}
		public:
			class context {
            public:
				context& in(const char param_def[],
                    const char stub_val[] = "0",
                    const char stub_var_def[] = ""){return *this;}
				context& out(const char param_def[],
                    const char stub_val[] = "0",
                    const char stub_var_def[] = ""){return *this;}
			};
            context& def(const char name[]){return *(new context);}
		public:
			void generate(const char file[]){}
		};

		class acta {
		public:
			void name(const char name[]){}
		public:
			class context {
            public:
				context& in(const char name[],
                    const char message[]){return *this;}
				context& out(const char name[],
                    const char message[]){return *this;}
			};
			context& def(const char name[],bool on_run=false){return *(new context);}
		public:
			void generate(const char file[]){}
		};
	}
}
