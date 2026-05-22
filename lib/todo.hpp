/*
new programming models/code names:
==================================

actor task model -- templet::acta + templet::meta::acta
(in templet::acta::actor  use 'task','ask','ret','say';
 in templet::acta::message use bool operator() <- access
)

global object model -- templet::globj + templet::meta::globj
(in templet::globj use 'update')

async-await model -- templet::async
(in templet::async use 'task', 'wait')

ask-say model- templet::chat
(in templet::chat use 'ask', 'say')

parallel map - templet::map
(in templet::map use 'on_run(unsigned iter)','on_save','on_load','on_init(unsigned size)')
*/
#include <functional>
#include <istream>
#include <ostream>

namespace templet {
	class wal {
	public:
		virtual void write(unsigned& index, unsigned tag, const std::string& blob) = 0;
		virtual bool read(unsigned index, unsigned& tag, std::string& blob) = 0;
	};

	class memwal :public wal {};

	class cliwal :public wal {
	private:
		class proxy :public wal {};
	};

	class stub {
		;
	private:
		class srvwal :public wal {};
	};

	class map {
	public:
		map(wal&);
		void init(unsigned size);
		void operator()(
			std::function<void(unsigned size)>init,
			std::function<void(unsigned iter)>map,
			std::function<void(unsigned iter, std::istream&, bool mapped)>save
			= [](unsigned, std::istream&, bool) {},
			std::function<void(unsigned iter, std::ostream&, bool mapped)>load
			= [](unsigned, std::ostream&, bool) {}
		);
	};

	class procpool {
	public:
		procpool(unsigned size, wal&);
		void operator()(std::function<unsigned pid, wal&) > process);
		void delay(double seconds);
		double duration();
	};
}