/*$TET$$header*/
// a sample file to start templet::state project
#define TEMPLET_STATE_TEST_IMPL
#include <syncmem.hpp>
/*$TET$*/

struct meta_name_of_my_class : public templet::meta::state {
	meta_name_of_my_class() {
/*$TET$$meta*/
	//prefix("template <typename T>"); 
	name("name_of_my_class");

	//enum action {_output,_update,_update_output,_save_update,_save_update_output};

	//def("method_with_returning_void", action::_save_update)
	//	.par("int x", "0")
	//	.par("int*y", "&y", "int y=0");

	//def("int", "method_returning_int", action::_output);

	skel("..\\GitHub\\templet\\bin\\skel.exe");
/*$TET$*/
	file(__FILE__);
}}; // create an instance of this class to activate the preprocessor
/*$TET$$footer*/
int main()
{

}
/*$TET$*/

