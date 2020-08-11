/*--------------------------------------------------------------------------*/
/*  Copyright 2016 Sergei Vostokin                                          */
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

#include <string>
#include <list>
#include <map>
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

enum LTYPE{ FMARK, LMARK, TEXT, FAULTY };

const string lead_sign_beg = "/*$TET$";
const string lead_sign_end = "*/";
const string close_sign = "/*$TET$*/";

const char DELIM[] = " \t";

typedef list<string> Block;

string fmark(string&key){ return lead_sign_beg + key + lead_sign_end; }
string lmark(){ return close_sign; }

LTYPE linetype(string& line, string& key)
{
	size_t pos;
	if ((pos = line.find(close_sign)) != string::npos){
		// /*tet*/
		size_t ptoken = line.find_first_not_of(DELIM);
		if (ptoken != pos) return TEXT;

		size_t after_close_sign_pos = pos + close_sign.size();
		if (line.find_first_not_of(DELIM, after_close_sign_pos) != string::npos) return FAULTY;

		return LMARK;
	}
	else if ((pos = line.find(lead_sign_beg)) != string::npos){
		// /*tet$key*/
		size_t ptoken = line.find_first_not_of(DELIM);
		if (ptoken != pos) return TEXT;

		size_t key_pos = pos + lead_sign_beg.size();
		size_t after_key_pos = line.find(lead_sign_end, key_pos);

		if (after_key_pos == string::npos) return FAULTY;

		size_t after_mark_pos = after_key_pos + lead_sign_end.size();

		if (line.find_first_not_of(DELIM, after_mark_pos) != string::npos) return FAULTY;

		key = line.substr(key_pos, after_key_pos - key_pos);
		return FMARK;
	}
	else return TEXT;
}

int main(int argc, char* argv[])
{
	bool isRelease = false, helpme = false, place_bottom = false, find_markup = false;
	string infile, outfile, skeleton;

	for (int i = 1; i < argc; i++) {
		char *cmd = argv[i];

		if (cmd[0] == '-') {
			switch (cmd[1]) {
			case 'O':case 'o':
				outfile = string(argv[i + 1]);
				break;
			case 'I':case 'i':
				infile = string(argv[i + 1]);
				break;
			case 'S':case 's':
				skeleton = string(argv[i + 1]);
				break;
			case 'R':case 'r':
				isRelease = true;
				break;
			case 'H':case 'h':
				helpme = true;
				break;
			case 'B':case 'b':
				place_bottom = true;
				break;
			}
		}
	}

	if (helpme || argc==1){
		cout << "Templet skeleton processor. Copyright Sergei Vostokin, 2016" << endl
			<< "Usage:  skel <options>\n" << endl
			<< " -s -S <skeleton file name> -- if not defined, the skel.exe tries to find out" << endl
			<< "       the first markup block, when print it's key or NOBLOCKS to stdout" << endl
			<< " -i -I <input file>" << endl
			<< " -o -O <output file> -- assumed to be the same as an input file, if not defined" << endl
			<< " -r -R -- use no markup in realease version of the output file" << endl
			<< " -b -B -- place blocks that have no pair in the skeleton into the bottom of" << endl
			<< "          the output file;" << endl
			<< "          assumed to be ON, if one file is used as input/output file" << endl
			<< " -h -H -- print this screen" << endl;

	}

	if (outfile.empty()){ outfile = infile; place_bottom = true; }
	if (infile.empty())	{
		cout << "ERROR: bad param(s): input file must be defined\n";
		return EXIT_FAILURE;
	}

	if (skeleton.empty()) find_markup = true;

	map<string, Block> block_map;

	{
		ifstream file(infile.c_str());

		int count = 0;
		bool read_block = false;
		string key;
		Block block;

		if (!file){ cout << "ERROR: could not open input file '" << infile << "'"; return EXIT_FAILURE; }
		
		string line;
		getline(file, line); count++;
		
		while (file){
			switch (linetype(line, key)){
			case FMARK:
				if (find_markup) {
					cout << key;
					return 0;
				}
				if (read_block) { 
					cout << "ERROR: " << fmark(key) << " inside a block in ihe input file '" << infile << "' at line #" << count;
					return EXIT_FAILURE; 
				}
				read_block = true;
				break;
			case LMARK:
				if (!read_block) {
					cout << "ERROR: " << lmark() << " outside a block in ihe input file '" << infile << "' at line #" << count;
					return EXIT_FAILURE;
				}
				read_block = false;
				{
					map<string, Block>::const_iterator it = block_map.find(key);
					if (it != block_map.end()){
						cout << "ERROR: the key '"<<key<<"' is not unique: input file '" << infile << "' at line #" << count;
						return EXIT_FAILURE;
					}
				}
				block_map[key] = block;
				block.clear();
				break;
			case TEXT:
				if (read_block)block.push_back(line);
				break;
			case FAULTY: cout << "ERROR: a faulty block found in input file '"<< infile<<"' at line #"<<count; return EXIT_FAILURE;
			}
			getline(file, line); count++;
		}
	}

	if (find_markup) {
		cout << "NOBLOCKS";
		return EXIT_SUCCESS; 
	}

	{
		string key;
		
		bool read_block = false;
		bool have_substituted_block=false;

		int count = 0;
		ifstream ifile(skeleton.c_str());
		ofstream ofile(outfile.c_str());

		if (!ifile){ cout << "ERROR: could not open skeleton file '" <<skeleton<< "for reading"; return EXIT_FAILURE; }
		if (!ofile){ cout << "ERROR: could not open output file '" << outfile << "for writing"; return EXIT_FAILURE; }

		string line;
		getline(ifile, line); count++;

		while (ifile){
			map<string, Block>::iterator it;
			
			switch (linetype(line, key)){
			case FMARK:
				if (read_block) {
					cout << "ERROR: skeleton is corrupted! The mark " << fmark(key) << " is inside a block in ihe skeleton file '" << skeleton << "' at line #" << count;
					return EXIT_FAILURE;
				}
				read_block = true;
				if(!isRelease) ofile << lead_sign_beg << key << lead_sign_end << endl;
				
				it=block_map.find(key);
				if (it != block_map.end()){
					have_substituted_block = true;
					list<string>::const_iterator its;
					for (its = (*it).second.begin(); its != (*it).second.end(); its++){
						ofile << (*its) << endl;
					}
					block_map.erase(it);
				}else
					have_substituted_block = false;
				break;
			case LMARK:
				if (!read_block) {
					cout << "ERROR: skeleton is corrupted! The mark " << lmark() << " is outside a block in ihe skeleton file '" << skeleton << "' at line #" << count;
					return EXIT_FAILURE;
				}
				read_block = false;
				if (!isRelease) ofile << close_sign << endl;
				break;
			case TEXT:
				if (!read_block || (read_block && !have_substituted_block)) ofile << line << endl;
				break;
			case FAULTY: cout << "ERROR: skeleton is corrupted! A faulty block found in skeleton file '" << skeleton << "' at line #" << count; return EXIT_FAILURE;
			}
			getline(ifile, line); count++;
		}

		if (!block_map.empty()){
			cout << "WARNING: not all blocks from the input file '" << infile << "' have a pair in the skeleton file '" << skeleton <<"'" << endl;
			cout << "         see the bottom of the output file '" << outfile << "'" << endl;
			if (place_bottom){
				map<string, Block>::const_iterator it;
				for (it = block_map.begin(); it != block_map.end(); it++){
					ofile << lead_sign_beg << (*it).first << lead_sign_end << endl;
					list<string>::const_iterator its;
					for (its = (*it).second.begin(); its != (*it).second.end(); its++)
						ofile << (*its) << endl;
					ofile << close_sign << endl;
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
