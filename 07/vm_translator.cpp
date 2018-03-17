#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <utility>
#include <regex>

using namespace std;

unordered_map<string,string> VM_CMD_PSPO {
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

unordered_map<string,string> VM_CMD_ARTM {
	pair<string,string> ("add",             "@SP\nAM=M-1\nD=M\n@SP\nAM=M-1\nM=D+M\n@SP\nM=M+1\n"),                                    
	pair<string,string> ("sub",             "@SP\nAM=M-1\nD=M\n@SP\nAM=M-1\nM=M-D\n@SP\nM=M+1\n"),
	pair<string,string> ("neg",             "@SP\nA=M-1\nM=-M\n"),
	pair<string,string> ("and",             "@SP\nAM=M-1\nD=M\n@SP\nAM=M-1\nM=D&M\n@SP\nM=M+1\n"),
	pair<string,string> ("or",              "@SP\nAM=M-1\nD=M\n@SP\nAM=M-1\nM=D|M\n@SP\nM=M+1\n"),
	pair<string,string> ("not",             "@SP\nA=M-1\nM=!M\n"),
};

unordered_map<string,string> VM_CMD_CTRL { /* Replace _J with an integer as order */
	pair<string,string> ("eq",              "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@FALSE_J\nD;JNE\n@SP\nA=M-1\nM=-1\n@TRUE_J\n0;JMP\n(FALSE_J)\n@SP\nA=M-1\nM=0\n(TRUE_J)\n"),
	pair<string,string> ("gt",              "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@FALSE_J\nD;JLE\n@SP\nA=M-1\nM=-1\n@TRUE_J\n0;JMP\n(FALSE_J)\n@SP\nA=M-1\nM=0\n(TRUE_J)\n"),
	pair<string,string> ("lt",              "@SP\nAM=M-1\nD=M\nA=A-1\nD=M-D\n@FALSE_J\nD;JGE\n@SP\nA=M-1\nM=-1\n@TRUE_J\n0;JMP\n(FALSE_J)\n@SP\nA=M-1\nM=0\n(TRUE_J)\n")
};

struct Parser {

	static void parser(ifstream& ifile);
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

	Parser::parser(ifile);

	ifile.close(); //ofile.close();

	return 0;
}

void Parser::parser(ifstream& ifile) {

	if (!ifile) return;
	
	int cnt_jmp = 0;
	unique_ptr<char[]> cbuffer(new char [1024]);
	vector <string> lines;

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

		if (s_pos == -1) {
			if (VM_CMD_ARTM.find(line) != VM_CMD_ARTM.end())
				cout << VM_CMD_ARTM[line];
			else if (VM_CMD_CTRL.find(line) != VM_CMD_CTRL.end())
				cout << regex_replace(VM_CMD_CTRL[line], regex("(_J)"), to_string(cnt_jmp++));
			else
				cerr << "Invalid VM uni-command: " << line << endl;
		} else {
			string num_part = line.substr(s_pos + 1);
			string cmd_part = line.substr(0, s_pos);
			if (VM_CMD_PSPO.find(cmd_part) != VM_CMD_PSPO.end())
				cout << "@" + num_part + "\n" + VM_CMD_PSPO[cmd_part];
			else
				cerr << "Invalid VM bi-command: " << line << endl;
		}

	}

	cout << "(END)\n@END\n0;JMP\n";
}
