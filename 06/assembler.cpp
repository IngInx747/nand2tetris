#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <regex>
#include <bitset>

using namespace std;

unordered_map<string,string> dests {
	pair<string,string> ("","000"),
	pair<string,string> ("M","001"),
	pair<string,string> ("D","010"),
	pair<string,string> ("MD","011"),
	pair<string,string> ("A","100"),
	pair<string,string> ("AM","101"),
	pair<string,string> ("AD","110"),
	pair<string,string> ("AMD","111")
};

unordered_map<string,string> jumps {
	pair<string,string> ("","000"),
	pair<string,string> ("JGT","001"),
	pair<string,string> ("JEQ","010"),
	pair<string,string> ("JGE","011"),
	pair<string,string> ("JLT","100"),
	pair<string,string> ("JNE","101"),
	pair<string,string> ("JLE","110"),
	pair<string,string> ("JMP","111")
};

unordered_map<string,string> comps = {
	pair<string,string> ("0","0101010"),
	pair<string,string> ("1","0111111"),
	pair<string,string> ("-1","0111010"),
	pair<string,string> ("D","0001100"),
    pair<string,string> ("A","0110000"),
    pair<string,string> ("M","1110000"),
    pair<string,string> ("!D","0001101"),
    pair<string,string> ("!A","0110001"),
    pair<string,string> ("!M","1110001"),
    pair<string,string> ("-D","0001111"),
    pair<string,string> ("-A","0110011"),
    pair<string,string> ("-M","1110011"),
    pair<string,string> ("D+1","0011111"),
    pair<string,string> ("A+1","0110111"),
    pair<string,string> ("M+1","1110111"),
    pair<string,string> ("D-1","0001110"),
    pair<string,string> ("A-1","0110010"),
    pair<string,string> ("M-1","1110010"),
    pair<string,string> ("D+A","0000010"),
    pair<string,string> ("D+M","1000010"),
    pair<string,string> ("D-A","0010011"),
    pair<string,string> ("D-M","1010011"),
    pair<string,string> ("A-D","0000111"),
    pair<string,string> ("M-D","1000111"),
    pair<string,string> ("D&A","00000000"),
    pair<string,string> ("D&M","1000000"),
    pair<string,string> ("D|A","0010101"),
    pair<string,string> ("D|M","1010101")
};

unordered_map<string,int> symbols = {
	pair<string,int> ("SP",0),
	pair<string,int> ("LCL",1),
	pair<string,int> ("ARG",2),
	pair<string,int> ("THIS",3),
	pair<string,int> ("THAT",4),
	pair<string,int> ("SCREEN",16384),
	pair<string,int> ("KBD",24576),
    pair<string,int> ("R0",0),
    pair<string,int> ("R1",1),
    pair<string,int> ("R1",1),
    pair<string,int> ("R2",2),
    pair<string,int> ("R3",3),
    pair<string,int> ("R4",4),
    pair<string,int> ("R5",5),
    pair<string,int> ("R6",6),
    pair<string,int> ("R7",7),
    pair<string,int> ("R8",8),
    pair<string,int> ("R9",9),
    pair<string,int> ("R10",10),
    pair<string,int> ("R11",11),
    pair<string,int> ("R12",12),
    pair<string,int> ("R13",13),
    pair<string,int> ("R14",14),
    pair<string,int> ("R15",15)
};

struct Parser {

	const static int NOT_COMMAND = 0;
	const static int A_COMMAND = 1;
	const static int C_COMMAND = 2;
	const static int L_COMMAND = 3;

	static void parser(ifstream& ifile);
	static int command_type(string const& cmd);
};

int main(int argc, char** argv) {

	if (argc < 2) {
		cerr << "Usage: ./assembler [filename]" << endl;
		exit(1);
	}

	string ifilename(argv[1]);//, ofilename(argv[2]);

	ifstream ifile(ifilename, std::ios::in);
	//ofstream ofile(ofilename, std::ios::out);

	if (!ifile) {
		cerr << "Cannot open: " << ifilename << endl;
		exit(1);
	}

	Parser::parser(ifile);

	ifile.close(); //ofile.close();

	return 0;
}

int Parser::command_type(string const& line) {
	
	if (regex_match(line, regex ("@.*"))) return Parser::A_COMMAND;
	else if (regex_match(line, regex (".*[=;].*"))) return Parser::C_COMMAND;
	else if (regex_match(line, regex ("\\(.+\\)"))) return Parser::L_COMMAND;
	else return Parser::NOT_COMMAND;
}

void Parser::parser(ifstream& ifile) {

	if (!ifile) return;
	
	int cnt_line = 0;
	int cnt_cmd = 0;
	unique_ptr<char[]> cbuffer(new char [1024]);
	vector <string> lines;

	while (!ifile.eof()) {
		
		ifile.getline(cbuffer.get(), 256);
		string line(cbuffer.get());
		cnt_line++;

		// Parsing to commands
		// remove comment
		line = regex_replace(line, regex("/.*"), "");
		// remove whitespace
		//line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
		line.erase(
			remove_if(
				line.begin(),
				line.end(),
				[] (char ch) {
					return std::isspace<char> (ch, std::locale::classic());
				}
			),
			line.end()
		);
		// ignore an empty or space-pure line
		if (regex_match(line, regex("^\\s*$"))) { continue; }

		if (Parser::command_type(line) == Parser::L_COMMAND) {
			line = regex_replace(line, regex("\\(|\\)"), "");
			if (symbols.find(line) == symbols.end())
				symbols[line] = cnt_cmd; // update symbol table
		} else {
			cnt_cmd++;
			lines.push_back(line);
		}
	}
	
	int cnt_variables = 16;
	for (string line:lines) {

		int address;
		string dest, comp, jump, symbol, ans;

		switch (Parser::command_type(line)) {
			case Parser::A_COMMAND: {
				line = line.substr(line.find_first_of('@') + 1); // For safety
				unordered_map<string,int>::iterator umit = symbols.find(line);
				if (umit != symbols.end()) { // @R0(RAM), @JUMP_POINT(ROM)
					address = umit->second;
				} else if (regex_match(line, regex("^0$|^[1-9][0-9]*$"))) {
					address = atoi(line.c_str()); // @0(RAM)
				} else { // @i
					address = cnt_variables++;
					symbols[line] = address;
				}
				cout << "0" << bitset<15> (address) << endl;
				break;
			}
			case Parser::C_COMMAND: {
				ans = "111";
				if (regex_match(line, regex (".*=.*"))) {
					int pos = line.find_first_of("=");
					dest = line.substr(0, pos);
					comp = line.substr(pos + 1);
					unordered_map<string,string>::iterator umit;
					umit = comps.find(comp);
					if (umit != comps.end())
						ans += umit->second;
					else {
						cerr << "Unknown computation: " << comp << endl;
						exit(1);
					}
					umit = dests.find(dest);
					if (umit != dests.end())
						ans += umit->second;
					else {
						cerr << "Unknown destination: " << dest << endl;
						exit(1);
					}
					cout << ans + "000" << endl;
				} else if (regex_match(line, regex (".*;.*"))) {
					int pos = line.find_first_of(";");
					comp = line.substr(0, pos);
					jump = line.substr(pos + 1);
					unordered_map<string,string>::iterator umit = comps.find(comp);
					if (umit != comps.end())
						ans += umit->second;
					else {
						cerr << "Unknown computation: " << comp << endl;
						exit(1);
					}
					ans += "000";
					umit = jumps.find(jump);
					if (umit != jumps.end())
						ans += umit->second;
					else {
						cerr << "Unknown jump oprand: " << comp << endl;
						exit(1);
					}
					cout << ans << endl;
				}
				break;
			}
			default: {
				cerr << "Invalid command: " << line << endl;
				exit(1);
			}
		}
	}
}
