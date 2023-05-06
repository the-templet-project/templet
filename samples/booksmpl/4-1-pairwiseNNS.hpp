/*$TET$$header*/

#include <templet.hpp>
using namespace templet;

#include <iostream>
using namespace std;

class block : public templet::message {
public:
	block(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
	int blockID;
};

/*$TET$*/

#pragma templet !Start(out!block)

struct Start :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((Start*)a)->on_out(*(block*)m);}

	Start(templet::engine&e) :Start() {
		Start::engines(e);
	}

	Start() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$Start$Start*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Start$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$Start$start*/
        out.send();
/*$TET$*/
	}

	inline void on_out(block&m) {
/*$TET$Start$out*/
/*$TET$*/
	}

	block out;

/*$TET$Start$$footer*/
/*$TET$*/
};

#pragma templet Stop(in?block)

struct Stop :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((Stop*)a)->on_in(*(block*)m);}

	Stop(templet::engine&e) :Stop() {
		Stop::engines(e);
	}

	Stop() :templet::actor(false)
	{
/*$TET$Stop$Stop*/
        num_of_computed_blocks = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Stop$engines*/
/*$TET$*/
	}

	inline void on_in(block&m) {
/*$TET$Stop$in*/
        if(++num_of_computed_blocks == N) stop();
/*$TET$*/
	}

	void in(block&m) { m.bind(this, &on_in_adapter); }

/*$TET$Stop$$footer*/
    int num_of_computed_blocks;
/*$TET$*/
};

#pragma templet Pair(in1?block,in2?block,out1!block,out2!block)

struct Pair :public templet::actor {
	static void on_in1_adapter(templet::actor*a, templet::message*m) {
		((Pair*)a)->on_in1(*(block*)m);}
	static void on_in2_adapter(templet::actor*a, templet::message*m) {
		((Pair*)a)->on_in2(*(block*)m);}
	static void on_out1_adapter(templet::actor*a, templet::message*m) {
		((Pair*)a)->on_out1(*(block*)m);}
	static void on_out2_adapter(templet::actor*a, templet::message*m) {
		((Pair*)a)->on_out2(*(block*)m);}

	Pair(templet::engine&e) :Pair() {
		Pair::engines(e);
	}

	Pair() :templet::actor(false),
		out1(this, &on_out1_adapter),
		out2(this, &on_out2_adapter)
	{
/*$TET$Pair$Pair*/
        _in1 = _in2 = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$Pair$engines*/
/*$TET$*/
	}

	inline void on_in1(block&m) {
/*$TET$Pair$in1*/
        _in1 = &m; swap();
/*$TET$*/
	}

	inline void on_in2(block&m) {
/*$TET$Pair$in2*/
        _in2 = &m; swap();
/*$TET$*/
	}

	inline void on_out1(block&m) {
/*$TET$Pair$out1*/
/*$TET$*/
	}

	inline void on_out2(block&m) {
/*$TET$Pair$out2*/
/*$TET$*/
	}

	void in1(block&m) { m.bind(this, &on_in1_adapter); }
	void in2(block&m) { m.bind(this, &on_in2_adapter); }
	block out1;
	block out2;

/*$TET$Pair$$footer*/
    void swap(){
        if(access(_in1) && access(_in2)){
            if(arr[_in1->blockID] > arr[_in2->blockID]){
                int tmp = arr[_in2->blockID];
                arr[_in2->blockID] = arr[_in1->blockID];
                arr[_in1->blockID] = tmp;
            }
            out1.blockID = _in1->blockID;
            out2.blockID = _in2->blockID;
            
            out1.send(); out2.send();
        }
    }
    
    block *_in1;
    block *_in2;
/*$TET$*/
};

/*$TET$$footer*/
/*$TET$*/
