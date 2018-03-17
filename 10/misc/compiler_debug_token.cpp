#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <regex>
#include <cctype>

using namespace std;

typedef int tType;

struct Compiler {

	const static tType ERROR = -1;
	const static tType IDENTIFIER = 0;
	const static tType KEYWORD = 1;
	const static tType SYMBOL = 2;
	const static tType INT_CONSTANT = 3;
	const static tType STRING_CONSTANT = 4;

	static int indent_config;
	
	static unordered_map<tType, string> TokenTypeTable;
	static unordered_set<char> SymbolTable;
	static unordered_set<string> KeyWordTable;

	static list<string> tokenizer(istream&);
	static tType tokenType(string);
	static string xml_single_token(tType, string, int=0);
	static string xml_move_forward(list<string>&, tType, const string, int=0);
	
	static string compileDo(list<string>&, int=0);
	static string compileLet(list<string>&, int=0);
	static string compileIf(list<string>&, int=0);
	static string compileWhile(list<string>&, int=0);
	static string compileReturn(list<string>&, int=0);
	static string compileStatements(list<string>&, int=0);
	static string compileExpression(list<string>&, int=0);
	static string compileExpressionList(list<string>&, int=0);
	static string compileTerm(list<string>&, int=0);
	static string compileVarDec(list<string>&, int=0);
	static string compileParameterList(list<string>&, int=0);
	static string compileClass(list<string>&, int=0);
	static string compileClassVarDec(list<string>&, int=0);
	static string compileSubroutine(list<string>&, int=0);
	static string compileSubroutineCall(list<string>&, int=0);
	static string compileSubroutineBody(list<string>&, int=0);
};

const tType Compiler::IDENTIFIER;
const tType Compiler::KEYWORD;
const tType Compiler::SYMBOL;
const tType Compiler::INT_CONSTANT;
const tType Compiler::STRING_CONSTANT;

int Compiler::indent_config = 2;

unordered_map<tType, string> Compiler::TokenTypeTable = {
	pair<tType, string> (Compiler::IDENTIFIER, "identifier"),
	pair<tType, string> (Compiler::KEYWORD, "keyword"),
	pair<tType, string> (Compiler::SYMBOL, "symbol"),
	pair<tType, string> (Compiler::INT_CONSTANT, "integerConstant"),
	pair<tType, string> (Compiler::STRING_CONSTANT, "stringConstant")
};

unordered_set<char> Compiler::SymbolTable = {
	'{','}','(',')','[',']','.',',',';','=','+','-','*','/','~','&','|','<','>'
};

unordered_set<string> Compiler::KeyWordTable = {
	"class", "constructor", "function", "method", "field", "static",
	"var", "int", "char", "boolean", "void", "true", "false", "null",
	"this", "let", "do", "if", "else", "while", "return"
};

int main(int argc, char** argv) {

	ifstream ifile;
	vector<string> ifilenames;
	vector<string> codes;

	for (int i=1; i<argc; i++) ifilenames.emplace_back(argv[i]);

	for (string ifilename : ifilenames) {

		ifile.open(ifilename, std::ios::in);
		if (!ifile) { cerr << "Cannot open: " << ifilename << endl; continue; }

		// for (auto s:Compiler::tokenizer(ifile)) cout << s << "#" << endl;

		list<string> tokens = Compiler::tokenizer(ifile);
		string output = Compiler::compileClass(tokens);
		cout << output;

		ifile.close();
	}

	return 0;
}

list<string> Compiler::tokenizer(istream& in) {

	/**
	* tokenize given file into recognized tokens
	* line comments and interline comments are removed
	*/
	
	char c;
	bool isInComment = false; // comment status
	string line, token, reserved;
	list<string> tokens;
	shared_ptr<char> cbuffer(new char [1024 + 1]);

	while (in.getline(cbuffer.get(), 1024)) {

		line = string(cbuffer.get());

		if (regex_search(line, regex("//"))) // remove // comment
			line = regex_replace(line, regex("//.*"), "");

		for (int i=0; i<line.size(); i++) {
			c = line[i];
			token = "";

			if (c=='/' && i<line.size()-1 && line[i+1]=='*')
				{ isInComment = true; i+=1; }
			else if (c=='*' && i<line.size()-1 && line[i+1]=='/')
				{ isInComment = false; i+=2; c = line[i]; }
			if (isInComment) continue; /** remove comment */

			if (isalpha(c) || isdigit(c) || c=='_') { // identifier
				while (isalpha(c) || isdigit(c) || c=='_') {
					token.push_back(c);
					c = line[++i];
				}
				if (c==' ' || c=='"' || c=='\n' || SymbolTable.count(c))
					i--; // to comprehense i++ each loop
			} else if (SymbolTable.count(c)) { // symbol
				token.push_back(c);
			} else if (c=='"') { // string constant
				token.push_back('"');
				do {
					c = line[++i];
					token.push_back(c);
				} while (c!='"' && i<line.size()-1);
			}
			if (token!="") tokens.push_back(token);
		}
	}

	return tokens;
}

tType Compiler::tokenType(string token) {
	if (KeyWordTable.count(token)) return KEYWORD;
	else if (SymbolTable.count(token[0])) return SYMBOL;
	else if (token[0]=='"' && token[token.size()-1]=='"')
		return STRING_CONSTANT;
	bool isInt = true, isIdentifier = true;
	for (char c:token) {
		if (!isdigit(c)) isInt = false;
		if (isspace(c)) isIdentifier = false;
	}
	if (isInt) return INT_CONSTANT;
	else if (isIdentifier) return IDENTIFIER;
	else return ERROR;
}

string Compiler::xml_single_token(tType type, string token, int ind) {
	if (type == SYMBOL) {
		switch (token.at(0)) {
			case '>': token = "&gt;"; break;
			case '<': token = "&lt;"; break;
			case '&': token = "&amp;"; break;
			default: break;
		}
	} else if (type == STRING_CONSTANT) {
		token = token.substr(1, token.size() - 2);
	} else if (type == ERROR) {
		cerr << "Error type: " << token << endl; exit(1);
	}
	return string(ind, ' ') + "<" + TokenTypeTable[type] + "> " +
		token + " </" + TokenTypeTable[type] + ">\n";
}

string Compiler::xml_move_forward(list<string>& tokens, tType type, const string expected_token, int ind) {
	string token = tokens.front();
	if (expected_token=="") {
		if (tokenType(token)!=type) {
			cerr << "Error: invalid " << TokenTypeTable[type] << ": " << token << endl;
			exit(1);
		}
	} else if (token!=expected_token) {
		cerr << "Error: \"" << token << "\" should be \"" << expected_token << "\"\n";
		exit(1);
	}
	tokens.pop_front();
	return xml_single_token(type, token, ind);
}

string Compiler::compileVarDec(list<string>& tokens, int ind) {

	/** var type varName(, "varName")* */

	string output = string(ind, ' ') + "<varDec>\n";

	output += xml_move_forward(tokens, KEYWORD, "var", ind + indent_config); // var
	output += xml_move_forward(tokens, tokenType(tokens.front()), "", ind + indent_config); // type
	output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config); // varName
	while (tokens.front()!=";") {
		output += xml_move_forward(tokens, SYMBOL, "", ind + indent_config);
		output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);
	}
	output += xml_move_forward(tokens, SYMBOL, "", ind + indent_config);
	output += string(ind, ' ') + "</varDec>\n";

	return output;
}

string Compiler::compileTerm(list<string>& tokens, int ind) {

	/**
	* integerConstant | stringConstant | keywordConstant |
	* varName | varName '[' expression ']' | subroutineCall |
	* '(' expression ')' | unaryOp term
	*/

	string output = string(ind, ' ') + "<term>\n";
	list<string>::iterator it;

	if (tokenType(tokens.front()) == IDENTIFIER) {
		it = tokens.begin(); it++;
		if (*it==".")
			output += compileSubroutineCall(tokens, ind + indent_config);
		else if (*it=="[") {
			output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);
			output += xml_move_forward(tokens, SYMBOL, "[", ind + indent_config);
			output += compileExpression(tokens, ind + indent_config);
			output += xml_move_forward(tokens, SYMBOL, "]", ind + indent_config);
		} else {
			output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);
		}
	} else if (tokens.front() == "-" || tokens.front() == "~") {
		output += xml_move_forward(tokens, SYMBOL, "", ind + indent_config);
		output += compileTerm(tokens, ind + indent_config);
	} else if (tokens.front() == "(") {
		output += xml_move_forward(tokens, SYMBOL, "(", ind + indent_config);
		output += compileExpression(tokens, ind + indent_config);
		output += xml_move_forward(tokens, SYMBOL, ")", ind + indent_config);
	} else {
		output += xml_move_forward(tokens, tokenType(tokens.front()), "", ind + indent_config);
	}
	output += string(ind, ' ') + "</term>\n";

	return output;
}

string Compiler::compileExpression(list<string>& tokens, int ind) {

	/**
	* term (opr term)*
	* x+y*z-1~2/3
	*/

	string output = string(ind, ' ') + "<expression>\n";

	output += compileTerm(tokens, ind + indent_config);
	while (tokens.front().find_first_of("+-*/&|<>=")!=-1) {
		output += xml_move_forward(tokens, SYMBOL, "", ind + indent_config);
		output += compileTerm(tokens, ind + indent_config);
	} output += string(ind, ' ') + "</expression>\n";

	return output;
}

string Compiler::compileExpressionList(list<string>& tokens, int ind) {

	/**
	* (expression (',' expression)* )?
	* x+y, y+z, 1+2
	*/

	string output = string(ind, ' ') + "<expressionList>\n";

	while (tokens.front()!=")") {
		if (tokens.front()!=",") {
			output += compileExpression(tokens, ind + indent_config);
		} else {
			output += xml_move_forward(tokens, SYMBOL, ",", ind + indent_config);
			output += compileExpression(tokens, ind + indent_config);
		}
	} output += string(ind, ' ') + "</expressionList>\n";

	return output;
}

string Compiler::compileSubroutine(list<string>& tokens, int ind) {

	/**
	* ('constructor' | 'function' | 'method') ('void' | type)
	* subroutineName '(' parameterList ')' subroutineBody
	* type: 'int' | 'char' | 'boolean' | className
	* method void print(char c1, char c2) {..}
	*/

	string output = string(ind, ' ') + "<subroutineDec>\n";

	output += xml_move_forward(tokens, KEYWORD, "", ind + indent_config);
	if (KeyWordTable.count(tokens.front()))
		output += xml_move_forward(tokens, KEYWORD, "", ind + indent_config);
	else
		output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);
	output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, "(", ind + indent_config);
	output += compileParameterList(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, ")", ind + indent_config);
	// subroutineBody
	output += compileSubroutineBody(tokens, ind + indent_config);
	output += string(ind, ' ') + "</subroutineDec>\n";

	return output;
}

string Compiler::compileParameterList(list<string>& tokens, int ind) {

	/**
	* ((type varName) (',' type varName)*)?
	* int x, int y, myClass obj
	*/

	string output = string(ind, ' ') + "<parameterList>\n";

	while (tokens.front()!=")") {
		if (tokens.front()!=",") {
			output += xml_move_forward(tokens, tokenType(tokens.front()), "", ind + indent_config);
			output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);
		} else {
			output += xml_move_forward(tokens, SYMBOL, ",", ind + indent_config);
			output += xml_move_forward(tokens, tokenType(tokens.front()), "", ind + indent_config);
			output += xml_move_forward(tokens, IDENTIFIER, tokens.front(), ind + indent_config);
		}
	} output += string(ind, ' ') + "</parameterList>\n";

	return output;
}

string Compiler::compileSubroutineCall(list<string>& tokens, int ind) {

	/**
	* subroutineName '(' expressionList ')' |
	* (className|varName) '.' subroutineName '(' expressionList ')'
	* print(x+y, z)
	* sys.println(str1, str2)
	*/

	string output = xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);
	if (tokens.front()==".") {
		output += xml_move_forward(tokens, SYMBOL, ".", ind + indent_config);
		output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);
		output += xml_move_forward(tokens, SYMBOL, "(", ind + indent_config);
		output += compileExpressionList(tokens, ind + indent_config);
		output += xml_move_forward(tokens, SYMBOL, ")", ind + indent_config);
	} else if (tokens.front()=="(") {
		output += xml_move_forward(tokens, SYMBOL, "(", ind + indent_config);
		output += compileExpressionList(tokens, ind + indent_config);
		output += xml_move_forward(tokens, SYMBOL, ")", ind + indent_config);
	}

	return output;
}

string Compiler::compileSubroutineBody(list<string>& tokens, int ind) {

	/**
	* '{' varDec* statements '}'
	*/

	string output = string(ind, ' ') + "<subroutineBody>\n";

	output += xml_move_forward(tokens, SYMBOL, "{", ind + indent_config);
	while (tokens.front()=="var")
		output += compileVarDec(tokens, ind + indent_config);
	output += compileStatements(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, "}", ind + indent_config);
	output += string(ind, ' ') + "</subroutineBody>\n";

	return output;
}

string Compiler::compileStatements(list<string>& tokens, int ind) {

	/**
	* statement*
	*/

	string output = string(ind, ' ') + "<statements>\n";

	while (tokens.front()!="}") {
		string tmp = tokens.front();
		if (tmp=="let") output += compileLet(tokens, ind + indent_config);
		else if (tmp=="do") output += compileDo(tokens, ind + indent_config);
		else if (tmp=="if") output += compileIf(tokens, ind + indent_config);
		else if (tmp=="while") output += compileWhile(tokens, ind + indent_config);
		else if (tmp=="return") output += compileReturn(tokens, ind + indent_config);
		else { cerr << "Error statements: " << tokens.front(); exit(1); }
	} output += string(ind, ' ') + "</statements>\n";

	return output;
}

string Compiler::compileLet(list<string>& tokens, int ind) {

	/**
	* 'let' varName('[' expression ']')? '=' expression ';'
	*/

	string output = string(ind, ' ') + "<letStatement>\n";

	output += xml_move_forward(tokens, KEYWORD, "let", ind + indent_config);
	output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);

	if (tokens.front()=="[") {
		output += xml_move_forward(tokens, SYMBOL, "[", ind + indent_config);
		output += compileExpression(tokens, ind + indent_config);
		output += xml_move_forward(tokens, SYMBOL, "]", ind + indent_config);
		output += xml_move_forward(tokens, SYMBOL, "=", ind + indent_config);
	} else if (tokens.front()=="=") {
		output += xml_move_forward(tokens, SYMBOL, "=", ind + indent_config);
	}

	output += compileExpression(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, ";", ind + indent_config);
	output += string(ind, ' ') + "</letStatement>\n";

	return output;
}

string Compiler::compileDo(list<string>& tokens, int ind) {

	/**
	* 'do' subroutineCall ';'
	*/

	string output = string(ind, ' ') + "<doStatement>\n";
	output += xml_move_forward(tokens, KEYWORD, "do", ind + indent_config);
	output += compileSubroutineCall(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, ";", ind + indent_config);
	output += string(ind, ' ') + "</doStatement>\n";

	return output;
}

string Compiler::compileIf(list<string>& tokens, int ind) {

	/**
	* 'if' '(' expression ')' '{' statements '}' ('else' '{' statements '}')?
	*/

	string output = string(ind, ' ') + "<ifStatement>\n";

	output += xml_move_forward(tokens, KEYWORD, "if", ind + indent_config);
	// (Expression)
	output += xml_move_forward(tokens, SYMBOL, "(", ind + indent_config);
	output += compileExpression(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, ")", ind + indent_config);
	// {Statements}
	output += xml_move_forward(tokens, SYMBOL, "{", ind + indent_config);
	output += compileStatements(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, "}", ind + indent_config);
	// (else {statememts})?
	if (tokens.front()=="else") {
		output += xml_move_forward(tokens, KEYWORD, "else", ind + indent_config);
		output += xml_move_forward(tokens, SYMBOL, "{", ind + indent_config);
		output += compileStatements(tokens, ind + indent_config);
		output += xml_move_forward(tokens, SYMBOL, "}", ind + indent_config);
	} output += string(ind, ' ') + "</ifStatement>\n";

	return output;
}

string Compiler::compileWhile(list<string>& tokens, int ind) {

	/**
	* 'while' '(' expression ')' '{' statements '}'
	*/

	string output = string(ind, ' ') + "<whileStatement>\n";

	output += xml_move_forward(tokens, KEYWORD, "while", ind + indent_config);
	// (expression)
	output += xml_move_forward(tokens, SYMBOL, "(", ind + indent_config);
	output += compileExpression(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, ")", ind + indent_config);
	// {statements}
	output += xml_move_forward(tokens, SYMBOL, "{", ind + indent_config);
	output += compileStatements(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, "}", ind + indent_config);
	output += string(ind, ' ') + "</whileStatement>\n";

	return output;
}

string Compiler::compileReturn(list<string>& tokens, int ind) {

	/**
	* 'return' expression? ';'
	*/

	string output = string(ind, ' ') + "<returnStatement>\n";

	output += xml_move_forward(tokens, KEYWORD, "return", ind + indent_config);
	// Expression?
	if (tokens.front()!=";")
		output += compileExpression(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, ";", ind + indent_config);
	output += string(ind, ' ') + "</returnStatement>\n";

	return output;
}

string Compiler::compileClass(list<string>& tokens, int ind) {

	/**
	* 'class' className '{' classVarDec* subroutineDec* '}'
	*/

	string output = string(ind, ' ') + "<class>\n";
	output += xml_move_forward(tokens, KEYWORD, "class", ind + indent_config);
	output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, "{", ind + indent_config);
	// classVarDec*
	while (tokens.front()=="static" || tokens.front()=="field")
		output += compileClassVarDec(tokens, ind + indent_config);
	// subroutineDec*
	while (tokens.front()=="constructor" ||
		tokens.front()=="function" ||
		tokens.front()=="method")
		output += compileSubroutine(tokens, ind + indent_config);
	output += xml_move_forward(tokens, SYMBOL, "}", ind + indent_config);
	output += string(ind, ' ') + "</class>\n";

	return output;
}

string Compiler::compileClassVarDec(list<string>& tokens, int ind) {

	/**
	* ('static'|'field') type varName(',' varName)*';'
	* field int/myClass v1, v2;
	*/

	string output = string(ind, ' ') + "<classVarDec>\n";

	output += xml_move_forward(tokens, KEYWORD, "", ind + indent_config);
	output += xml_move_forward(tokens, tokenType(tokens.front()), "", ind + indent_config);
	output += xml_move_forward(tokens, IDENTIFIER, "", ind + indent_config); // varName
	while (tokens.front()!=";") { // (, varName)
		output += xml_move_forward(tokens, SYMBOL, ",", ind + indent_config);
		output += xml_move_forward(tokens, IDENTIFIER, tokens.front(), ind + indent_config);
	}
	output += xml_move_forward(tokens, SYMBOL, ";", ind + indent_config);
	output += string(ind, ' ') + "</classVarDec>\n";

	return output;
}
