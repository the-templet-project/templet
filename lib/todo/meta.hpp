/*--------------------------------------------------------------------------*/
/*  Copyright 2026 Sergei Vostokin                                          */
/*--------------------------------------------------------------------------*/

#pragma once

#include <functional>
#include <istream>
#include <ostream>

namespace templet {

	namespace meta {

		class processor {};

		class globj {
		public:
			class update {
				update& in(const char param[], const char stub_value[] = "", const char stub_value_def[] = "");
				update& out(const char param[], const char stub_value[] = "", const char stub_value_def[] = "");
				update& ret(const char type[]);
				update& cnst();
			};
		public:
			void name(const char name[]);
			void pref(const char prefix[]);
			update& def(const char name[]);
		public:
			void generate(const char file[]);
		};

		class acta {
		public:
			class actor {
				actor& start();
				actor& in(const char message[], const char name[]);
				actor& out(const char message[], const char name[]);
				actor& task(const char name[]);
			};
		public:
			void name(const char name[]);
			void pref(const char prefix[]);
			actor& def(const char name[]);
		
		public:
			void generate(const char file[]);
		};
	}
}
