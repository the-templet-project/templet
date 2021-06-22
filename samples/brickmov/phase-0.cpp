/*$TET$$header*/
/*
Задача: смоделировать процесс перекладки кирпичей из исходной кучи в новую кучу по цепочке рабочих.

Цели данного демонстрационного примера -- показать:  
       1) этапы проектирования алгоритмов с использованием фреймворка Templet;
       2) способы описания локальных действий в распределенных алгоритмах;
	   3) метод описания масштабируемого алгоритма с неизменяемой в процессе вычислений структурой;
	   4) тестирование правильности работы алгоритма, введение задач с произвольным порядком выполнения;
	   5) тестирование потенциала параллельного выполения задач в алгоритме.

фаза 0
------
-- определение типов сообщений и акторов
-- решение редуцированной задачи с двумя рабочими, перекладывающими кирпичи
-- простейшие действия с предусловием  "поступление сообщения"

фаза 1
------ 
-- более сложные действия с предусловием в виде произвольного локального состояния актора
-- этап тестирования базовой логики работы алгоритма в немасштабируемом варианте со всеми типами акторов

фаза 2
------
-- алгорим в масштабируемом виде
-- выделение задач в коде действий акторов
-- тестирование работоспособности алгорима при произвольном порядке завершения задач

фаза 3
------
-- исследование потенциального ускорения алгоритма при параллельном выполении задач 
*/

#include <templet.hpp>
#include <cmath>
#include <iostream>
#include <ctime>

class brick : public templet::message {
public:
	brick(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
	int brick_num;
};
/*$TET$*/

#pragma templet !first(out!brick)

struct first :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((first*)a)->on_out(*(brick*)m);}

	first(templet::engine&e) :first() {
		first::engines(e);
	}

	first() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$first$first*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$first$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$first$start*/
        pass_next_brick();
/*$TET$*/
	}

	inline void on_out(brick&m) {
/*$TET$first$out*/
        pass_next_brick();
/*$TET$*/
	}

	brick out;

/*$TET$first$$footer*/
    void pass_next_brick(){
        if(access(out) && last_brick_num > 0){
            std::cout << "выдаем кирпич №" << last_brick_num << std::endl; 
            out.brick_num = last_brick_num--;
            out.send();
        }
    }
    
    int last_brick_num;
/*$TET$*/
};

#pragma templet center(in?brick,out!brick)

struct center :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((center*)a)->on_in(*(brick*)m);}
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((center*)a)->on_out(*(brick*)m);}

	center(templet::engine&e) :center() {
		center::engines(e);
	}

	center() :templet::actor(false),
		out(this, &on_out_adapter)
	{
/*$TET$center$center*/
        _in = 0;
        have_a_brick = false;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$center$engines*/
/*$TET$*/
	}

	inline void on_in(brick&m) {
/*$TET$center$in*/
        _in = &m;
        take_a_brick();
        pass_a_brick();
/*$TET$*/
	}

	inline void on_out(brick&m) {
/*$TET$center$out*/
        take_a_brick();
        pass_a_brick();
/*$TET$*/
	}

	void in(brick&m) { m.bind(this, &on_in_adapter); }
	brick out;

/*$TET$center$$footer*/
    void pass_a_brick(){
        if(have_a_brick && access(out)){
            out.brick_num = brick_num;
            have_a_brick = false;
            out.send();
        }
    }
    
    void take_a_brick(){
        if(access(_in) && !have_a_brick){
            brick_num = _in->brick_num;
            have_a_brick = true;
            _in->send();
            std::cout << "'центральный' рабочий №" << center_num << " принял кирпич №" << brick_num << std::endl;
        }
    }
    
    brick* _in;
    bool have_a_brick;
    int  brick_num;
    int center_num;
/*$TET$*/
};

#pragma templet last(in?brick)

struct last :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((last*)a)->on_in(*(brick*)m);}

	last(templet::engine&e) :last() {
		last::engines(e);
	}

	last() :templet::actor(false)
	{
/*$TET$last$last*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$last$engines*/
/*$TET$*/
	}

	inline void on_in(brick&m) {
/*$TET$last$in*/
        if(m.brick_num == 1){
            std::cout << "получен последний кирпич" << std::endl;
            stop();
        }
        else{
            std::cout << "получен кирпич #" << m.brick_num <<std::endl;
            m.send();
        }
/*$TET$*/
	}

	void in(brick&m) { m.bind(this, &on_in_adapter); }

/*$TET$last$$footer*/
/*$TET$*/
};

/*$TET$$footer*/

int main()
{
	templet::engine eng;

	first  a_first(eng);
	center a_center1(eng),a_center2(eng),a_center3(eng);
	last   a_last(eng);

    a_center1.in(a_first.out);
    a_center2.in(a_center1.out);
    a_center3.in(a_center2.out);
    a_last.in(a_center3.out);
    
    //a_last.in(a_first.out);
    
    a_first.last_brick_num = 2;
    
    a_center1.center_num = 1;
    a_center2.center_num = 2;
    a_center3.center_num = 3;
    
    eng.start();

	if (eng.stopped()) {
		std::cout << "кирпичи переложены в новое место";
		return EXIT_SUCCESS;
	}

	std::cout << "что-то сломалось" << std::endl;
	return EXIT_FAILURE;
}
/*$TET$*/
