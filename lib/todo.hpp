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
			std::function<void(unsigned size)> init,
			std::function<void(unsigned iter)> map,
			std::function<void(unsigned iter, std::ostream&, bool mapped)> save
			= [](unsigned, std::ostream&, bool) {},
			std::function<void(unsigned iter, std::istream&, bool mapped)> load
			= [](unsigned, std::istream&, bool) {}
		);
	};

	class procpool {
	public:
		procpool(unsigned size, wal&);
		void operator()(std::function<unsigned pid, wal&>) process);
		void delay(double seconds);
		double duration();
	};

    class globj{
    public:
        globj(wal&);
        void init();
    public:
        virtual void on_init() = 0; 
    public:
        void update(
            unsigned id,
            std::function<void(std::ostream&)> save,
            std::function<void(std::istream&,std::ostream&)> update,
            std::function<void(std::istream&)> load
        );
        void update(
            unsigned id,
            std::function<void(std::ostream&)> update,
            std::function<void(std::istream&)> load
        );
        void update(
            unsigned id,
            std::function<void(std::ostream&)> save,
            std::function<void(std::istream&)> update
        );
        void update();
    };

namespace meta{

    class globj{
    public:
        void name(const char name[]);
		void prefix(const char prefix[]);
        update& def(const char name[]);
        class update{
            update& in(const char param[],const char stub_value[]="",const char stub_value_def[]="");
            update& out(const char param[],const char stub_value[]="",const char stub_value_def[]="");
            update& constant();
        };
    };
}
    class async{
    public:
        async(wal&);
        void task(bool local,
            std::function<void(std::ostream&)> action,
            std::function<void(std::istream&)> load
        );
        void wait();
    };

    class chat{
    public:
        chat(wal&);
        void chat(const std::string& user);
        virtual void on_chat(const std::string& user) = 0;
        void say(std::function<void()>output_action);
        void ask(
            std::function<void(std::ostream&)>input_action,
            std::istream& input
        ); 
    }

    class acta{
    public:
        acta(wal&);
        void run();
        virtual on_run() 
    public:
        class actor{
        public:
            actor(acta&);
            actor(acta*);
        public:
            void ask(message&);
            void ask(message*);
            void ret(message&);
            void ret(message*);
            void say(message&);
            void say(message*);
        public:
             void task(bool local,
                std::function<void(std::ostream&)> action,
                std::function<void(std::istream&)> load
            );
        };
        class message{
        public:
            message(actor&);
            message(actor*);
        public:
            bool operator(actor*)();
            bool operator(actor&)();
        };
    };

namespace meta{

    class acta{
    public:
        void name(const char name[]);
		void prefix(const char prefix[]);
        actor& def(const char name[]);
    public:
        class actor{
            actor& in(const char type[],const char name[]);
            actor& out(const char type[],const char name[]);
            actor& task(const char name[]);
        }
    };
}
}