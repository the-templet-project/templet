/*--------------------------------------------------------------------------*/
/*  Copyright 2021 Sergei Vostokin                                          */
/*                                                                          */
/*  Licensed under the Apache License, Version 2.0 (the "License");         */
/*  you may not use this file except in compliance with the License.        */
/*  You may obtain a copy of the License at                                 */
/*                                                                          */
/*  http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                          */
/*  Unless required by applicable law or agreed to in writing, software     */
/*  distributed under the License is distributed on an "AS IS" BASIS,       */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/*  See the License for the specific language governing permissions and     */
/*  limitations under the License.                                          */
/*--------------------------------------------------------------------------*/

#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
	if (argc != 2) {
		cout <<
			"  ___ " << endl <<
			" |_>_|   The Temlet Project SDK (http://templet.ssau.ru)" << endl <<
			" |_>_|   Actor-task model processor  v.0.9.0" << endl <<
			"TEMPLET  Copyright 2021, Sergei Vostokin" << endl << endl;
	
		cout
			<< "Usage: acta <input file with the actor-task model pragmas>" << endl << endl

			<< "the actor-task model pragmas syntax:" << endl << endl

			<< "#pragma templet ['!'] id ['(' field {',' field} ')']" << endl << endl
			<< "field :: = id '!' id |" << endl
			<< "           id '?' id |" << endl
			<< "           id ':' id ['.' id]" << endl;

		return EXIT_FAILURE;
	}

	int ret;
	
	string acta_exe(argv[0]);
	string skel_exe;
	string cgen_exe;

	string arg_file(argv[1]);
	
	string backup_file;
	string cgen_file;
	string command;

	backup_file = arg_file + ".bak";
	cgen_file = arg_file + ".cgn";

	backup_file = "\"" + backup_file + "\"";
	cgen_file = "\"" + cgen_file + "\"";
	arg_file = "\"" + arg_file + "\"";

	skel_exe = acta_exe;
	cgen_exe = acta_exe;

	string::size_type namepos = acta_exe.rfind("acta");
	skel_exe[namepos] = 's'; skel_exe[namepos + 1] = 'k'; skel_exe[namepos + 2] = 'e'; skel_exe[namepos + 3] = 'l';
	cgen_exe[namepos] = 'c'; cgen_exe[namepos + 1] = 'g'; cgen_exe[namepos + 2] = 'e'; cgen_exe[namepos + 3] = 'n';

	if (system(NULL) == 0) {
		cout << "ERROR: command processor is not detected";
		return EXIT_FAILURE;
	};

	command = cgen_exe + " " + arg_file + " " + cgen_file;
	ret = system(command.c_str());

	if (ret == EXIT_FAILURE) {
#if defined(_WIN32) || defined(_WIN64) 
		command = "del " + cgen_file;
#else
		command = "rm " + cgen_file;
#endif
		system(command.c_str());

		return EXIT_FAILURE;
	}

#if defined(_WIN32) || defined(_WIN64) 
	command = "copy " + arg_file + " " + backup_file;
#else
	command = "cp " + arg_file + " " + backup_file;
#endif
	ret = system(command.c_str());

	if (ret == EXIT_FAILURE) {
		cout << "ERROR: failed to backup " << arg_file << " to " << backup_file;
		return EXIT_FAILURE;
	}

	command = skel_exe + " -i " + arg_file + " -s " + cgen_file;
	ret = system(command.c_str());

	if (ret == EXIT_FAILURE) {
#if defined(_WIN32) || defined(_WIN64)
		command = "copy " + backup_file + " " + arg_file;
#else	
		command = "cp " + backup_file + " " + arg_file;
#endif
		system(command.c_str());
	}

#if defined(_WIN32) || defined(_WIN64)
	command = "del " + cgen_file;
#else
	command = "rm " + cgen_file;
#endif
	system(command.c_str());

	if(ret == EXIT_SUCCESS) cout << "Ok";
	
	return ret;
}
