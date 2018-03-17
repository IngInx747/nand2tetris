#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <regex>

using namespace std;

int main(int argc, char** argv) {

	std::map<std::string,bool> args
		{std::pair<std::string,bool> ("no-comments", 0)};

	std::vector<std::string> ifilenames;
	std::string ofilename;
	std::fstream ifile, ofile;

	char* token;
	char c_buffer[1024];
	std::string line;

	if (argc<2) {
		cerr << "Need at least 1 arg\n";
		exit(1);
	}

	for (int i=1; i<argc; i++) {
		std::string str(argv[i]);
		if (std::regex_match(str, std::regex (".*\\.(in)$"))) // match *.in
			ifilenames.push_back(str);
		else if (args.find(str) != args.end())
			args[str] = 1; // received an option argument
		else {
			cerr << "File extension not accepted: " << str << endl;
		}
	}

	if (ifilenames.empty()) {
		cerr << "No valid input" << endl;
		exit(1);
	}

	for (std::string file:ifilenames) {

		ifile.open(file, std::ios::in);
		if (!ifile) {
			cerr << "Cannot open file: " << file << endl;
			continue;
		}

		// get .out filename
		//int pos = file.find_last_of("/");
		//if (pos != -1) file = file.substr(pos);
		strcpy(c_buffer, file.c_str());
		token = strtok(c_buffer, ".");
		ofilename = std::string(token) + ".out";
		ofile.open(ofilename, std::ios::out);

		while (!ifile.eof()) {
			getline(ifile, line);
			//cout << line << endl;
			if (args["no-comments"]) { // do trimming
				int index = line.find_first_of("/");
				//cout << index << endl;
				if (index != -1)
					line = line.substr(0, index);
			}
			if (std::regex_match(line, std::regex ("^\\s*$"))) {
				//cout << line << endl;
				continue; // ignore an empty or space-pure line
			}
			strcpy(c_buffer, line.c_str());
			token = strtok(c_buffer, " \t\r\n\v\f");
			while (token) {
				ofile << std::string (token);
				token = strtok(NULL, " ");
			} ofile << endl;
		}

		ifile.close(); ofile.close();
	}

	return 0;
}
