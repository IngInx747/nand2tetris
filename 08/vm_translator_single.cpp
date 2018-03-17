#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <utility>
#include <regex>

using namespace std;

unordered_map<string,string> VM_CMD_PSPO { /* push/pop + ** + dest */
	pair<string,string> ("push constant",   "D=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push argument",   "D=A\n@ARG\nA=D+M\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push local",      "D=A\n@LCL\nA=D+M\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push static",     "D=A\n@16\nA=D+A\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push this",       "D=A\n@THIS\nA=D+M\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push that",       "D=A\n@THAT\nA=D+M\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push pointer",    "D=A\n@3\nA=D+A\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push temp",       "D=A\n@5\nA=D+A\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("pop argument",    "D=A\n@ARG\nD=D+M\n@R5\nM=D\n@SP\nAM=M-1\nD=M\n@R5\nA=M\nM=D\n"),
	pair<string,string> ("pop local",       "D=A\n@LCL\nD=D+M\n@R5\nM=D\n@SP\nAM=M-1\nD=M\n@R5\nA=M\nM=D\n"),
	pair<string,string> ("pop static",      "D=A\n@16\nD=D+A\n@R5\nM=D\n@SP\nAM=M-1\nD=M\n@R5\nA=M\nM=D\n"),
	pair<string,string> ("pop this",        "D=A\n@R3\nD=D+M\n@R5\nM=D\n@SP\nAM=M-1\nD=M\n@R5\nA=M\nM=D\n"),
	pair<string,string> ("pop that",        "D=A\n@R4\nD=D+M\n@R5\nM=D\n@SP\nAM=M-1\nD=M\n@R5\nA=M\nM=D\n"),
	pair<string,string> ("pop pointer",     "D=A\n@3\nD=D+A\n@R5\nM=D\n@SP\nAM=M-1\nD=M\n@R5\nA=M\nM=D\n"),
	pair<string,string> ("pop temp",        "D=A\n@5\nD=D+A\n@R5\nM=D\n@SP\nAM=M-1\nD=M\n@R5\nA=M\nM=D\n"),
};

unordered_map<string,string> VM_CMD_ARTM { /* single oprand */
	pair<string,string> ("add",             "@SP\nAM=M-1\nD=M\n@SP\nAM=M-1\nM=D+M\n@SP\nM=M+1\n"),                                    
	pair<string,string> ("sub",             "@SP\nAM=M-1\nD=M\n@SP\nAM=M-1\nM=M-D\n@SP\nM=M+1\n"),
	pair<string,string> ("neg",             "@SP\nA=M-1\nM=-M\n"),
	pair<string,string> ("and",             "@SP\nAM=M-1\nD=M\n@SP\nAM=M-1\nM=D&M\n@SP\nM=M+1\n"),
	pair<string,string> ("or",              "@SP\nAM=M-1\nD=M\n@SP\nAM=M-1\nM=D|M\n@SP\nM=M+1\n"),
	pair<string,string> ("not",             "@SP\nA=M-1\nM=!M\n"),
};

unordered_map<string,string> VM_CMD_JUMP { /* single oprand, replace _J with an integer as order */
	pair<string,string> ("eq",              "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@FALSE_J\nD;JNE\n@SP\nA=M-1\nM=-1\n"
		                                    "@TRUE_J\n0;JMP\n(FALSE_J)\n@SP\nA=M-1\nM=0\n(TRUE_J)\n"),
	pair<string,string> ("gt",              "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@FALSE_J\nD;JLE\n@SP\nA=M-1\nM=-1\n"
		                                    "@TRUE_J\n0;JMP\n(FALSE_J)\n@SP\nA=M-1\nM=0\n(TRUE_J)\n"),
	pair<string,string> ("lt",              "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@FALSE_J\nD;JGE\n@SP\nA=M-1\nM=-1\n"
		                                    "@TRUE_J\n0;JMP\n(FALSE_J)\n@SP\nA=M-1\nM=0\n(TRUE_J)\n")
};

unordered_map<string,string> VM_CMD_CTRL { /* oprand + branch */
	pair<string,string> ("label",           "(X)\n"),
	pair<string,string> ("goto",            "@X\n0;JMP\n"),
	pair<string,string> ("if-goto",         "@SP\nAM=M-1\nD=M\nM=0\n@X\nD;JNE\n"),
};

unordered_map<string,string> VM_CMD_FUNC {
	pair<string,string> ("function",        "(X)\n"),
	pair<string,string> ("return",          "@LCL\nD=M\n@FRAME\nM=D\n@5\nD=D-A\nA=D\nD=M\n@RETURN\nM=D\n@SP\n"
	                                        "A=M-1\nD=M\n@ARG\nA=M\nM=D\n@ARG\nD=M\n@SP\nM=D+1\n@FRAME\nAM=M-1\nD=M\n"
	                                        "@THAT\nM=D\n@FRAME\nAM=M-1\nD=M\n@THIS\nM=D\n@FRAME\nAM=M-1\nD=M\n"
	                                        "@ARG\nM=D\n@FRAME\nAM=M-1\nD=M\n@LCL\nM=D\n@RETURN\nA=M\n0;JMP\n"),
	pair<string,string> ("call",            "@RETURN_FUNC_X\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n@LCL\nD=M\n@SP\n"
		                                    "A=M\nM=D\n@SP\nM=M+1\n@ARG\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n@THIS\nD=M\n"
		                                    "@SP\nA=M\nM=D\n@SP\nM=M+1\n@THAT\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n@SP\n"
		                                    "D=M\n@N_ARG\nD=D-A\n@5\nD=D-A\n@ARG\nM=D\n@SP\nD=M\n@LCL\nM=D\n@_FUNC\n"
		                                    "0;JMP\n(RETURN_FUNC_X)\n")
};

struct Parser {

	static void parser(ifstream& ifile, ostream& out);
	static int command_type(string const& cmd);
};

int main(int argc, char** argv) {

	if (argc < 2) {
		cerr << "Miss argument" << endl;
		exit(1);
	}

	string ifilename(argv[1]);//, ofilename(argv[2]);

	ifstream ifile(ifilename, std::ios::in);
	//ofstream ofile(ofilename, std::ios::out);

	if (!ifile) {
		cerr << "Cannot open: " << ifilename << endl;
		exit(1);
	}

	Parser::parser(ifile, cout);

	ifile.close(); //ofile.close();

	return 0;
}

void Parser::parser(ifstream& ifile, ostream& out) {

	if (!ifile) return;
	
	int cnt_jmp = 0;
	unique_ptr<char[]> cbuffer(new char [1024]);
	//vector <string> lines;

	out << "@256\nD=A\n@SP\nM=D\n";

	while (!ifile.eof()) {
		
		ifile.getline(cbuffer.get(), 256);
		string line(cbuffer.get());

		// Parsing to commands
		// remove comment
		line = regex_replace(line, regex("/.*"), "");
		// remove front blanks
		line = regex_replace(line, regex("^\\s+"), "");
		// remove back blanks
		line = regex_replace(line, regex("\\s+$"), "");
		
		// ignore an empty or space-pure line
		if (regex_match(line, regex("^\\s*$"))) { continue; }

		int s_pos = line.find_last_of(" ");

		if (s_pos == -1) { /* Arithmetic instruction and return has 1 oprand */
			if (line == "return")
				out << VM_CMD_FUNC["return"];
			else if (VM_CMD_ARTM.find(line) != VM_CMD_ARTM.end()) // add, sub, etc.
				out << VM_CMD_ARTM[line];
			else if (VM_CMD_JUMP.find(line) != VM_CMD_JUMP.end()) // gt, lt, eq
				out << regex_replace(VM_CMD_JUMP[line], regex("(_J)"), to_string(cnt_jmp++));
			else
				cerr << "Invalid VM uni-command: " << line << endl;
		} else {
			string num_part = line.substr(s_pos + 1); // 0, IF_TRUE, etc.
			string cmd_part = line.substr(0, s_pos); // pop local, goto-if, etc. or function func_name
			//cout << cmd_part << "|\n";
			if (regex_search(cmd_part, regex("label")))
				out << "(" << num_part << ")\n";
			else if (regex_search(cmd_part, regex("function"))) { // function + func.name + repeat_time
				string func_name = cmd_part.substr(cmd_part.find_first_of(" ") + 1); // function name
				out << "(" << func_name << ")\n";
				int repeat_time = stoi(num_part);
				for (int i=0; i<repeat_time; i++)
					out << "@0\n" + VM_CMD_PSPO["push constant"];;
			} else if (regex_search(cmd_part, regex("call"))) { // call + func.name + args
				string func_name = cmd_part.substr(cmd_part.find_first_of(" ") + 1); // function name
				string asmb = regex_replace(VM_CMD_FUNC["call"], regex("X"), to_string(cnt_jmp++));
				asmb = regex_replace(asmb, regex("FUNC"), func_name);
				asmb = regex_replace(asmb, regex("N_ARG"), num_part);
				out << asmb;
			} else {
				if (VM_CMD_PSPO.find(cmd_part) != VM_CMD_PSPO.end()) // push/pop
					out << "@" + num_part + "\n" + VM_CMD_PSPO[cmd_part];
				else if (VM_CMD_CTRL.find(cmd_part) != VM_CMD_CTRL.end()) // goto label
					out << regex_replace(VM_CMD_CTRL[cmd_part], regex("X"), num_part);
				else
					cerr << "Invalid VM bi-command: " << line << endl;
			}
		}

	}

	out << "(END)\n@END\n0;JMP\n";
}
