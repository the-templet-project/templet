/*$TET$$header*/
#include "pch.h"
#include <iostream>

#include <syncmem.hpp>

#include <set>
#include <thread>
#include <atomic>
/*$TET$*/

template <typename T>
class scanner : public templet::state {
/*$TET$scanner$header$*/
public:
	scanner(bool master, templet::write_ahead_log&l) : state(l), is_master(master), ready_to_compute(false) { init(); }
public:
	void scan() {
		srand((unsigned)time(0));
		unsigned index = 0;

		if (is_master) {
			for (auto& element : array)share_element(index++, element);
			set_ready_to_compute();
		}

		while (!ready_to_compute) update();

		while (get_not_scanned(index, rand())) {
			on_scan(array[index]);
			put_scanned(index, array[index]);
		}
	}
	std::vector<T> array;
protected:
	virtual void on_scan(T&element) = 0;
	virtual void on_save(const T&element, std::ostream&, bool scanned) = 0;
	virtual void on_load(T&element, std::istream&, bool scanned) = 0;
private:
/*$TET$*/
	void set_ready_to_compute() {
		update(_set_ready_to_compute, [this]() {
/*$TET$scanner$set_ready_to_compute$update*/
			not_scanned.clear();
			for (unsigned i = 0; i < array.size(); i++)not_scanned.insert(i);
			ready_to_compute = true;
/*$TET$*/
		});
	}
	void share_element(unsigned index, T&element) {
		update(_share_element, [&](std::ostream&out) {
/*$TET$scanner$share_element$save*/
			out << index; on_save(element, out, false);
/*$TET$*/
		},
		[this](std::istream&in) {
/*$TET$scanner$share_element$update*/
			unsigned index; T element;
			in >> index; on_load(element, in, false);

			if (array.size() <= index) array.resize(index + 1);
			array[index] = element;
/*$TET$*/
		});
	}
	bool get_not_scanned(unsigned& index, int random) {
		update();
/*$TET$scanner$get_not_scanned$output*/
		if (not_scanned.size() == 0) return false;
		int selected = random % not_scanned.size();
		auto it = not_scanned.begin(); for (int i = 0; i != selected; i++, it++) {} index = *it;
		return true;
/*$TET$*/
	}
	void put_scanned(unsigned index, T&element) {
		update(_put_scanned, [&](std::ostream&out) {
/*$TET$scanner$put_scanned$save*/
			out << index; on_save(element, out, true);
/*$TET$*/
		},
		[this](std::istream&in) {
/*$TET$scanner$put_scanned$update*/
			unsigned index; T element;
			in >> index; on_load(element, in, true);

			if (not_scanned.find(index) != not_scanned.end()) {//if(not_scanned.contains(index))
				array[index] = element; not_scanned.erase(index);
			}
/*$TET$*/
		});
	}
private:
	enum {
		_set_ready_to_compute,
		_share_element,
		_put_scanned
	};
	void on_init() override {
		{ scanner::set_ready_to_compute(); }
		{ T element; scanner::share_element(0, element); }
		{ T element; scanner::put_scanned(0, element); }
	}
/*$TET$scanner$footer$*/
private:
	std::set<unsigned> not_scanned;
	bool is_master;
	bool ready_to_compute;
/*$TET$*/
};

/*$TET$$footer*/
struct sqware_type {
	int N;
	int NxN;
};

class calc_sqware_scanner : public  scanner<sqware_type> {
public:
	calc_sqware_scanner(bool master, templet::write_ahead_log&wal) :scanner(master, wal) {}
private:
	void on_scan(sqware_type&e) override { e.NxN = e.N * e.N; }
	void on_save(const sqware_type&e, std::ostream&out, bool scanned) override {
		if (scanned) out << e.NxN; else out << e.N;
	}
	void on_load(sqware_type&e, std::istream&in, bool scanned) override {
		if (scanned) in >> e.NxN; else in >> e.N;
	}
};

int main()
{
	std::atomic_int PID = 0;
	int NUM_THREADS = 10;
	int ARRAY_SIZE = 10;

	templet::write_ahead_log wal;
	std::vector<std::thread> threads(NUM_THREADS);

	for (auto& t : threads)t = std::thread([&] { int pid = PID++;
	//////////////// inside a 'process' ////////////////
	calc_sqware_scanner scanner_object(pid == 0, wal);

	if (pid == 0) {
		scanner_object.array.resize(ARRAY_SIZE);
		for (int i = 0; i < 10; i++) scanner_object.array[i].N = i;
	}

	//scanner_object.scan();

	if (pid == 0) {// .. or any other pid < NUM_THREADS 
		for (int i = 0; i < ARRAY_SIZE; i++)
			std::cout << i << " * " << i << " = "
			<< scanner_object.array[i].NxN
			<< std::endl;
	}
	////////////////////////////////////////////////////
	}); for (auto& t : threads) t.join();
	std::cout << "Success!" << std::endl;
}
/*$TET$*/

