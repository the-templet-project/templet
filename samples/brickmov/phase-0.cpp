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
-- решение задачи с числом рабочих >2
-- более сложные действия с предусловием в виде произвольного локального состояния актора
-- этап тестирования базовой логики работы алгоритма в немасштабируемом варианте со всеми типами акторов

фаза 2
------
-- алгорим в масштабируемом виде
-- выделение задач в коде действий акторов
-- тестирование работоспособности алгорима при произвольном порядке завершения задач

фаза 3
------
-- исследование потенциального ускорения алгоритма при параллельном выполнении задач 
*/

const int NUMBER_OF_BRICKS = 2;

#include <templet.hpp>
#include <cmath>
#include <iostream>
#include <ctime>

class brick : public templet::message {
public:
	brick(templet::actor*a=0, templet::message_adaptor ma=0) :templet::message(a, ma) {}
	int brick_ID;
};
/*$TET$*/

#pragma templet !source(out!brick)

struct source :public templet::actor {
	static void on_out_adapter(templet::actor*a, templet::message*m) {
		((source*)a)->on_out(*(brick*)m);}

	source(templet::engine&e) :source() {
		source::engines(e);
	}

	source() :templet::actor(true),
		out(this, &on_out_adapter)
	{
/*$TET$source$source*/
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$source$engines*/
/*$TET$*/
	}

	void start() {
/*$TET$source$start*/
        on_out(out);
/*$TET$*/
	}

	inline void on_out(brick&m) {
/*$TET$source$out*/
       if(access(out) && number_of_bricks > 0){ 
           out.brick_ID = number_of_bricks--;
           out.send(); 
           
           std::cout << "the source worker takes a brick #" << out.brick_ID << " from the pile" << std::endl;
        }
/*$TET$*/
	}

	brick out;

/*$TET$source$$footer*/
    int number_of_bricks;
/*$TET$*/
};

#pragma templet destination(in?brick)

struct destination :public templet::actor {
	static void on_in_adapter(templet::actor*a, templet::message*m) {
		((destination*)a)->on_in(*(brick*)m);}

	destination(templet::engine&e) :destination() {
		destination::engines(e);
	}

	destination() :templet::actor(false)
	{
/*$TET$destination$destination*/
        number_of_bricks = 0;
/*$TET$*/
	}

	void engines(templet::engine&e) {
		templet::actor::engine(e);
/*$TET$destination$engines*/
/*$TET$*/
	}

	inline void on_in(brick&m) {
/*$TET$destination$in*/
        number_of_bricks++;
        if(m.brick_ID == 1) stop(); else  m.send();
        
        std::cout << "the destination worker receives a brick #" << m.brick_ID << std::endl;
/*$TET$*/
	}

	void in(brick&m) { m.bind(this, &on_in_adapter); }

/*$TET$destination$$footer*/
    int number_of_bricks;
/*$TET$*/
};

/*$TET$$footer*/

int main()
{
	templet::engine eng;

	source source_worker(eng);
	destination  destination_worker(eng);

    destination_worker.in(source_worker.out);
    
    source_worker.number_of_bricks = NUMBER_OF_BRICKS;
    
    eng.start();

	if (eng.stopped()) {
		std::cout << "all " << destination_worker.number_of_bricks 
            << " bricks were moved to a new place. done !!!";
		return EXIT_SUCCESS;
	}

	std::cout << "something broke (((" << std::endl;
	return EXIT_FAILURE;
}
/*$TET$*/