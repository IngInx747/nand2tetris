#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <utility>
#include <regex>

using namespace std;

#define _DEBUG_

#ifdef _DEBUG_
#define _DEBUG_OUTPUT_(x) "// End of : "<<x<<"\n"
#else
#define _DEBUG_OUTPUT_(x) ""
#endif

unordered_map<string,string> VM_CMD_PSPO { /* push/pop + ** + dest */
	pair<string,string> ("push constant",   "@X\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push argument",   "@X\nD=A\n@ARG\nA=D+M\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push local",      "@X\nD=A\n@LCL\nA=D+M\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push static",     "@X\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push this",       "@X\nD=A\n@THIS\nA=D+M\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push that",       "@X\nD=A\n@THAT\nA=D+M\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push pointer",    "@X\nD=A\n@3\nA=D+A\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("push temp",       "@X\nD=A\n@R5\nA=D+A\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n"),
	pair<string,string> ("pop argument",    "@X\nD=A\n@ARG\nD=D+M\n@R15\nM=D\n@SP\nAM=M-1\nD=M\n@R15\nA=M\nM=D\n"),
	pair<string,string> ("pop local",       "@X\nD=A\n@LCL\nD=D+M\n@R15\nM=D\n@SP\nAM=M-1\nD=M\n@R15\nA=M\nM=D\n"),
	pair<string,string> ("pop static",      "@X\nD=A\n@R15\nM=D\n@SP\nAM=M-1\nD=M\n@R15\nA=M\nM=D\n"),
	pair<string,string> ("pop this",        "@X\nD=A\n@THIS\nD=D+M\n@R15\nM=D\n@SP\nAM=M-1\nD=M\n@R15\nA=M\nM=D\n"),
	pair<string,string> ("pop that",        "@X\nD=A\n@THAT\nD=D+M\n@R15\nM=D\n@SP\nAM=M-1\nD=M\n@R15\nA=M\nM=D\n"),
	pair<string,string> ("pop pointer",     "@X\nD=A\n@3\nD=D+A\n@R15\nM=D\n@SP\nAM=M-1\nD=M\n@R15\nA=M\nM=D\n"),
	pair<string,string> ("pop temp",        "@X\nD=A\n@R5\nD=D+A\n@R15\nM=D\n@SP\nAM=M-1\nD=M\n@R15\nA=M\nM=D\n"),
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
	pair<string,string> ("if-goto",         "@SP\nAM=M-1\nD=M\n@X\nD;JNE\n"),
};

unordered_map<string,string> VM_CMD_FUNC {
	pair<string,string> ("function",        "(X)\n"),

	pair<string,string> ("call",            "@RETURN_FUNC_X\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push return-addr
	                                        "@LCL\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push LCL
		                                    "@ARG\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push ARG
		                                    "@THIS\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push THIS
		                                    "@THAT\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push THAT
		                                    "@N_ARG\nD=A\n@5\nD=D+A\n@SP\nD=M-D\n@ARG\nM=D\n" // ARG = SP-n-5
		                                    "@SP\nD=M\n@LCL\nM=D\n" // LCL = SP
		                                    "@FUNC\n0;JMP\n(RETURN_FUNC_X)\n"), // goto func

	pair<string,string> ("return",          "@LCL\nD=M\n@R13\nM=D\n" // D = FRAME = LCL
		                                    "@5\nAD=D-A\nD=M\n@R14\nM=D\n" // D = *(FRAME-5), R14 = RET = *(FRAME-5)
		                                    "@SP\nAM=M-1\nD=M\n@ARG\nA=M\nM=D\n" // *ARG = pop()
		                                    "@ARG\nD=M+1\n@SP\nM=D\n" // SP = ARG + 1
	                                        "@R13\nAM=M-1\nD=M\n@THAT\nM=D\n" // THAT = *(--FRAME)
	                                        "@R13\nAM=M-1\nD=M\n@THIS\nM=D\n" // THIS = *(--FRAME)
	                                        "@R13\nAM=M-1\nD=M\n@ARG\nM=D\n" // ARG = *(--FRAME)
	                                        "@R13\nAM=M-1\nD=M\n@LCL\nM=D\n" // LCL = *(--FRAME)
	                                        "@R14\nA=M\n0;JMP\n") // goto RET
};

struct Parser {

	static int cnt_jmp;

	static void parser(ifstream& ifile, ostream& out, string info);
	static int command_type(string const& cmd);
};

int Parser::cnt_jmp = 0;

int main(int argc, char** argv) {

	if (argc < 2) {
		cerr << "Miss argument" << endl;
		exit(1);
	}

	vector<string> ifilenames; //, ofilename(argv[2]); R5

	for (int i=1; i<argc; i++)
		ifilenames.emplace_back(argv[i]);

	ifstream ifile;
	//ofstream ofile(ofilename, std::ios::out);

	cout << "@256\nD=A\n@SP\nM=D\n";
	cout << "@RETURN_Sys.init\nD=A\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push return-addr
	        "@LCL\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push LCL
		    "@ARG\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push ARG
		    "@THIS\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push THIS
		    "@THAT\nD=M\n@SP\nA=M\nM=D\n@SP\nM=M+1\n" // push THAT
		    "@5\nD=A\n@SP\nD=M-D\n@ARG\nM=D\n" // ARG = SP-0-5
		    "@SP\nD=M\n@LCL\nM=D\n" // LCL = SP
		    "@Sys.init\n0;JMP\n(RETURN_Sys.init)\n";
	for (string ifilename : ifilenames) {
		ifile.open(ifilename, std::ios::in);
		if (!ifile) cerr << "Cannot open: " << ifilename << endl;
		Parser::parser(ifile, cout, ifilename.substr(ifilename.find_last_of('/') + 1));
		ifile.close();
	}
	cout << "(END)\n@END\n0;JMP\n";
	//ofile.close();

	return 0;
}

void Parser::parser(ifstream& ifile, ostream& out, string info = "") {

	if (!ifile) return;
	
	unique_ptr<char[]> cbuffer(new char [1024]);

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
				out << VM_CMD_FUNC["return"] << _DEBUG_OUTPUT_(line);
			else if (VM_CMD_ARTM.find(line) != VM_CMD_ARTM.end()) // add, sub, etc.
				out << VM_CMD_ARTM[line] << _DEBUG_OUTPUT_(line);
			else if (VM_CMD_JUMP.find(line) != VM_CMD_JUMP.end()) // gt, lt, eq
				out << regex_replace(VM_CMD_JUMP[line], regex("(_J)"), to_string(cnt_jmp++)) << _DEBUG_OUTPUT_(line);
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
				out << "(" << func_name << ")\n";// << "@LCL\nD=M\n@SP\nM=D\n";
				int repeat_time = stoi(num_part);
				for (int i=0; i<repeat_time; i++)
					out << "@SP\nA=M\nM=0\n@SP\nM=M+1\n";
				out << _DEBUG_OUTPUT_(line);
			} else if (regex_search(cmd_part, regex("call"))) { // call + func.name + args
				string func_name = cmd_part.substr(cmd_part.find_first_of(" ") + 1); // function name
				string asmb = regex_replace(VM_CMD_FUNC["call"], regex("X"), to_string(cnt_jmp++));
				asmb = regex_replace(asmb, regex("FUNC"), func_name);
				asmb = regex_replace(asmb, regex("N_ARG"), num_part);
				out << asmb << _DEBUG_OUTPUT_(line);
			} else {
				if (VM_CMD_PSPO.find(cmd_part) != VM_CMD_PSPO.end()) { // push/pop
					if (cmd_part == "push static" || cmd_part == "pop static")
						num_part = info + "." + num_part;
					out << regex_replace(VM_CMD_PSPO[cmd_part], regex("X"), num_part) << _DEBUG_OUTPUT_(line);
				}
				else if (VM_CMD_CTRL.find(cmd_part) != VM_CMD_CTRL.end()) // goto label
					out << regex_replace(VM_CMD_CTRL[cmd_part], regex("X"), num_part) << _DEBUG_OUTPUT_(line);
				else
					cerr << "Invalid VM bi-command: " << line << endl;
			}
		}

	}
}
