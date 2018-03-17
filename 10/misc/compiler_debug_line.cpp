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
typedef pair<string, int> tToken;
typedef list<tToken> tContainer;

/* A safer front method for list in case list is empty */
tToken Front(const tContainer&);

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

	static tContainer tokenizer(istream&);
	static tType tokenType(string);
	static string xml_single_token(tType, string, int=0);
	static string xml_write_and_check(tContainer&, tType, const string, int=0);
	static string xml_dynamic_type(tContainer&, int=0);
	
	static string compileDo(tContainer&, int=0);
	static string compileLet(tContainer&, int=0);
	static string compileIf(tContainer&, int=0);
	static string compileWhile(tContainer&, int=0);
	static string compileReturn(tContainer&, int=0);
	static string compileStatements(tContainer&, int=0);
	static string compileExpression(tContainer&, int=0);
	static string compileExpressionList(tContainer&, int=0);
	static string compileTerm(tContainer&, int=0);
	static string compileVarDec(tContainer&, int=0);
	static string compileParameterList(tContainer&, int=0);
	static string compileClass(tContainer&, int=0);
	static string compileClassVarDec(tContainer&, int=0);
	static string compileSubroutine(tContainer&, int=0);
	static string compileSubroutineCall(tContainer&, int=0);
	static string compileSubroutineBody(tContainer&, int=0);
};

const tType Compiler::ERROR;
const tType Compiler::IDENTIFIER;
const tType Compiler::KEYWORD;
const tType Compiler::SYMBOL;
const tType Compiler::INT_CONSTANT;
const tType Compiler::STRING_CONSTANT;

int Compiler::indent_config = 2;

unordered_map<tType, string> Compiler::TokenTypeTable = {
	pair<tType, string> (Compiler::ERROR, ""),
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

vector<string> lineTracer; /* Record lines to trace back where error happens */
unordered_set<string> classNameTable; /* Record new-declared className, unfunctioned under Jack */

int main(int argc, char** argv) {

	ifstream ifile;
	vector<string> ifilenames;
	vector<string> codes;

	for (int i=1; i<argc; i++) ifilenames.emplace_back(argv[i]);

	for (string ifilename : ifilenames) {

		ifile.open(ifilename, std::ios::in);
		if (!ifile) { cerr << "Cannot open: " << ifilename << endl; continue; }

		// for (auto s:Compiler::tokenizer(ifile)) cout << s << "#" << endl;

		tContainer tokens = Compiler::tokenizer(ifile);
		string output = Compiler::compileClass(tokens);
		cout << output;

		ifile.close();
	}

	return 0;
}

tToken Front(const tContainer& tokens) {
	if (tokens.empty()) {
		cerr << "Incomplete file" << endl;
		exit(1);
	} return tokens.front();
}

tContainer Compiler::tokenizer(istream& in) {

	/**
	* tokenize given file into recognized tokens
	* line comments and interline comments are removed
	*/
	
	char c;
	bool isInComment = false; // comment status
	int cnt_line = 0;
	string line, token, reserved;
	tContainer tokens;
	shared_ptr<char> cbuffer(new char [1024 + 1]);

	while (in.getline(cbuffer.get(), 1024)) {

		line = string(cbuffer.get()); cnt_line++;
		lineTracer.push_back(line);

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
			if (token!="") tokens.emplace_back(token, cnt_line);
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

	/**
	* Write a single xml given token's type
	*/

	if (type == SYMBOL) {
		switch (token.at(0)) {
			case '>': token = "&gt;"; break;
			case '<': token = "&lt;"; break;
			case '&': token = "&amp;"; break;
			case '"': token = "&quat;"; break;
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

string Compiler::xml_write_and_check(tContainer& tokens, tType type, const string expected_token, int ind) {

	/**
	* Write xml, check syntax given expected token, move forward
	*/

	string token = Front(tokens).first;
	if (expected_token=="") {
		if (tokenType(token)!=type) {
			int errorLine = Front(tokens).second;
			cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
			cerr << "Error: invalid " << TokenTypeTable[type] << ": " << token << endl;
			exit(1);
		}
	} else if (token!=expected_token) {
		int errorLine = Front(tokens).second;
		cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
		cerr << "Error: \"" << token << "\" should be \"" << expected_token << "\"\n";
		exit(1);
	}
	tokens.pop_front();
	return xml_single_token(type, token, ind);
}

string Compiler::xml_dynamic_type(tContainer& tokens, int ind) {

	/**
	* In case type is ambiguous. Eg. int var1; Square var2;
	*/

	string token = Front(tokens).first;
	tokens.pop_front();
	return xml_single_token(tokenType(token), token, ind);
/**
	The reason I keep this unfunctioned codes segment is to keep type check function available.
	If compiler never sees a className it is undefined error.
	But Jack seems not to support #include or import father files.

	if (tokenType(token)==KEYWORD)
		return xml_single_token(KEYWORD, token, ind);
	else if (tokenType(token)==IDENTIFIER && classNameTable.count(token))
		return xml_single_token(IDENTIFIER, token, ind);
	else {
		int errorLine = Front(tokens).second;
		cerr << "Line " << errorLine << ": " << lineTracer[errorLine] << endl;
		cerr << "Error: Undefined type or class: \"" << token << "\"\n";
		exit(1);
	}*/
}

string Compiler::compileVarDec(tContainer& tokens, int ind) {

	/** var type varName(, "varName")* */

	string output = string(ind, ' ') + "<varDec>\n";

	output += xml_write_and_check(tokens, KEYWORD, "var", ind + indent_config); // var
	output += xml_dynamic_type(tokens, ind + indent_config); // type
	output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config); // varName
	while (Front(tokens).first!=";") {
		output += xml_write_and_check(tokens, SYMBOL, "", ind + indent_config);
		output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);
	}
	output += xml_write_and_check(tokens, SYMBOL, "", ind + indent_config);
	output += string(ind, ' ') + "</varDec>\n";

	return output;
}

string Compiler::compileTerm(tContainer& tokens, int ind) {

	/**
	* integerConstant | stringConstant | keywordConstant |
	* varName | varName '[' expression ']' | '.' subroutineCall |
	* '(' expression ')' | unaryOp term
	*/

	string output = string(ind, ' ') + "<term>\n";
	tContainer::iterator it;

	if (tokenType(Front(tokens).first) == IDENTIFIER) {
		it = tokens.begin(); it++;
		if (it==tokens.end()) {
			cerr << "LL(2): Incomplete file" << endl;
			exit(1);
		}
		else if (it->first==".")
			output += compileSubroutineCall(tokens, ind);
		else if (it->first=="[") {
			output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);
			output += xml_write_and_check(tokens, SYMBOL, "[", ind + indent_config);
			output += compileExpression(tokens, ind + indent_config);
			output += xml_write_and_check(tokens, SYMBOL, "]", ind + indent_config);
		} else {
			output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);
		}
	} else if (Front(tokens).first == "-" || Front(tokens).first == "~") {
		output += xml_write_and_check(tokens, SYMBOL, "", ind + indent_config);
		output += compileTerm(tokens, ind + indent_config);
	} else if (Front(tokens).first == "(") {
		output += xml_write_and_check(tokens, SYMBOL, "(", ind + indent_config);
		output += compileExpression(tokens, ind + indent_config);
		output += xml_write_and_check(tokens, SYMBOL, ")", ind + indent_config);
	} else {
		tType type = tokenType(Front(tokens).first);
		if (type==INT_CONSTANT)
			output += xml_write_and_check(tokens, INT_CONSTANT, "", ind + indent_config);
		else if (type==STRING_CONSTANT)
			output += xml_write_and_check(tokens, STRING_CONSTANT, "", ind + indent_config);
		else if (type==KEYWORD)
			output += xml_write_and_check(tokens, KEYWORD, "", ind + indent_config);
		else
			output += xml_write_and_check(tokens, ERROR, "", ind + indent_config);
	}
	output += string(ind, ' ') + "</term>\n";

	return output;
}

string Compiler::compileExpression(tContainer& tokens, int ind) {

	/**
	* term (opr term)*
	* x+y*z-1~2/3
	*/

	string output = string(ind, ' ') + "<expression>\n";

	output += compileTerm(tokens, ind + indent_config);
	while (Front(tokens).first.find_first_of("+-*/&|<>=")!=-1) {
		output += xml_write_and_check(tokens, SYMBOL, "", ind + indent_config);
		output += compileTerm(tokens, ind + indent_config);
	} output += string(ind, ' ') + "</expression>\n";

	return output;
}

string Compiler::compileExpressionList(tContainer& tokens, int ind) {

	/**
	* (expression (',' expression)* )?
	* x+y, y+z, 1+2
	*/

	string output = string(ind, ' ') + "<expressionList>\n";

	while (Front(tokens).first!=")") {
		if (Front(tokens).first!=",") {
			output += compileExpression(tokens, ind + indent_config);
		} else {
			output += xml_write_and_check(tokens, SYMBOL, ",", ind + indent_config);
			output += compileExpression(tokens, ind + indent_config);
		}
	} output += string(ind, ' ') + "</expressionList>\n";

	return output;
}

string Compiler::compileSubroutine(tContainer& tokens, int ind) {

	/**
	* ('constructor' | 'function' | 'method') ('void' | type)
	* subroutineName '(' parameterList ')' subroutineBody
	* type: 'int' | 'char' | 'boolean' | className
	* method void print(char c1, char c2) {..}
	*/

	string output = string(ind, ' ') + "<subroutineDec>\n";

	output += xml_write_and_check(tokens, KEYWORD, "", ind + indent_config);
	if (KeyWordTable.count(Front(tokens).first))
		output += xml_write_and_check(tokens, KEYWORD, "", ind + indent_config);
	else
		output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);
	output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, "(", ind + indent_config);
	output += compileParameterList(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, ")", ind + indent_config);
	// subroutineBody
	output += compileSubroutineBody(tokens, ind + indent_config);
	output += string(ind, ' ') + "</subroutineDec>\n";

	return output;
}

string Compiler::compileParameterList(tContainer& tokens, int ind) {

	/**
	* ((type varName) (',' type varName)*)?
	* int x, int y, myClass obj
	*/

	string output = string(ind, ' ') + "<parameterList>\n";

	while (Front(tokens).first!=")") {
		if (Front(tokens).first!=",") {
			output += xml_dynamic_type(tokens, ind + indent_config); // type
			output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);
		} else {
			output += xml_write_and_check(tokens, SYMBOL, ",", ind + indent_config);
			output += xml_dynamic_type(tokens, ind + indent_config); // type
			output += xml_write_and_check(tokens, IDENTIFIER, Front(tokens).first, ind + indent_config);
		}
	} output += string(ind, ' ') + "</parameterList>\n";

	return output;
}

string Compiler::compileSubroutineCall(tContainer& tokens, int ind) {

	/**
	* subroutineName '(' expressionList ')' |
	* (className|varName) '.' subroutineName '(' expressionList ')'
	* print(x+y, z)
	* sys.println(str1, str2)
	*/

	string output = xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);
	if (Front(tokens).first==".") {
		output += xml_write_and_check(tokens, SYMBOL, ".", ind + indent_config);
		output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);
		output += xml_write_and_check(tokens, SYMBOL, "(", ind + indent_config);
		output += compileExpressionList(tokens, ind + indent_config);
		output += xml_write_and_check(tokens, SYMBOL, ")", ind + indent_config);
	} else if (Front(tokens).first=="(") {
		output += xml_write_and_check(tokens, SYMBOL, "(", ind + indent_config);
		output += compileExpressionList(tokens, ind + indent_config);
		output += xml_write_and_check(tokens, SYMBOL, ")", ind + indent_config);
	}

	return output;
}

string Compiler::compileSubroutineBody(tContainer& tokens, int ind) {

	/**
	* '{' varDec* statements '}'
	*/

	string output = string(ind, ' ') + "<subroutineBody>\n";

	output += xml_write_and_check(tokens, SYMBOL, "{", ind + indent_config);
	while (Front(tokens).first=="var")
		output += compileVarDec(tokens, ind + indent_config);
	output += compileStatements(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, "}", ind + indent_config);
	output += string(ind, ' ') + "</subroutineBody>\n";

	return output;
}

string Compiler::compileStatements(tContainer& tokens, int ind) {

	/**
	* statement*
	*/

	string output = string(ind, ' ') + "<statements>\n";

	while (Front(tokens).first!="}") {
		string tmp = Front(tokens).first;
		if (tmp=="let") output += compileLet(tokens, ind + indent_config);
		else if (tmp=="do") output += compileDo(tokens, ind + indent_config);
		else if (tmp=="if") output += compileIf(tokens, ind + indent_config);
		else if (tmp=="while") output += compileWhile(tokens, ind + indent_config);
		else if (tmp=="return") output += compileReturn(tokens, ind + indent_config);
		else { xml_write_and_check(tokens, ERROR, "", 0); }
	} output += string(ind, ' ') + "</statements>\n";

	return output;
}

string Compiler::compileLet(tContainer& tokens, int ind) {

	/**
	* 'let' varName('[' expression ']')? '=' expression ';'
	*/

	string output = string(ind, ' ') + "<letStatement>\n";

	output += xml_write_and_check(tokens, KEYWORD, "let", ind + indent_config);
	output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);

	if (Front(tokens).first=="[") {
		output += xml_write_and_check(tokens, SYMBOL, "[", ind + indent_config);
		output += compileExpression(tokens, ind + indent_config);
		output += xml_write_and_check(tokens, SYMBOL, "]", ind + indent_config);
		output += xml_write_and_check(tokens, SYMBOL, "=", ind + indent_config);
	} else if (Front(tokens).first=="=") {
		output += xml_write_and_check(tokens, SYMBOL, "=", ind + indent_config);
	}

	output += compileExpression(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, ";", ind + indent_config);
	output += string(ind, ' ') + "</letStatement>\n";

	return output;
}

string Compiler::compileDo(tContainer& tokens, int ind) {

	/**
	* 'do' subroutineCall ';'
	*/

	string output = string(ind, ' ') + "<doStatement>\n";
	output += xml_write_and_check(tokens, KEYWORD, "do", ind + indent_config);
	output += compileSubroutineCall(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, ";", ind + indent_config);
	output += string(ind, ' ') + "</doStatement>\n";

	return output;
}

string Compiler::compileIf(tContainer& tokens, int ind) {

	/**
	* 'if' '(' expression ')' '{' statements '}' ('else' '{' statements '}')?
	*/

	string output = string(ind, ' ') + "<ifStatement>\n";

	output += xml_write_and_check(tokens, KEYWORD, "if", ind + indent_config);
	// (Expression)
	output += xml_write_and_check(tokens, SYMBOL, "(", ind + indent_config);
	output += compileExpression(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, ")", ind + indent_config);
	// {Statements}
	output += xml_write_and_check(tokens, SYMBOL, "{", ind + indent_config);
	output += compileStatements(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, "}", ind + indent_config);
	// (else {statememts})?
	if (Front(tokens).first=="else") {
		output += xml_write_and_check(tokens, KEYWORD, "else", ind + indent_config);
		output += xml_write_and_check(tokens, SYMBOL, "{", ind + indent_config);
		output += compileStatements(tokens, ind + indent_config);
		output += xml_write_and_check(tokens, SYMBOL, "}", ind + indent_config);
	} output += string(ind, ' ') + "</ifStatement>\n";

	return output;
}

string Compiler::compileWhile(tContainer& tokens, int ind) {

	/**
	* 'while' '(' expression ')' '{' statements '}'
	*/

	string output = string(ind, ' ') + "<whileStatement>\n";

	output += xml_write_and_check(tokens, KEYWORD, "while", ind + indent_config);
	// (expression)
	output += xml_write_and_check(tokens, SYMBOL, "(", ind + indent_config);
	output += compileExpression(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, ")", ind + indent_config);
	// {statements}
	output += xml_write_and_check(tokens, SYMBOL, "{", ind + indent_config);
	output += compileStatements(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, "}", ind + indent_config);
	output += string(ind, ' ') + "</whileStatement>\n";

	return output;
}

string Compiler::compileReturn(tContainer& tokens, int ind) {

	/**
	* 'return' expression? ';'
	*/

	string output = string(ind, ' ') + "<returnStatement>\n";

	output += xml_write_and_check(tokens, KEYWORD, "return", ind + indent_config);
	// Expression?
	if (Front(tokens).first!=";")
		output += compileExpression(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, ";", ind + indent_config);
	output += string(ind, ' ') + "</returnStatement>\n";

	return output;
}

string Compiler::compileClass(tContainer& tokens, int ind) {

	/**
	* 'class' className '{' classVarDec* subroutineDec* '}'
	*/

	string output = string(ind, ' ') + "<class>\n";
	output += xml_write_and_check(tokens, KEYWORD, "class", ind + indent_config);
/*
	string className = Front(tokens).first;
	if (classNameTable.count(className)) {
		int errorLine = Front(tokens).second;
		cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
		cerr << "Redefinition of class: " << className << endl;
		exit(1);
	} classNameTable.insert(className);
*/
	output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, "{", ind + indent_config);
	// classVarDec*
	while ((Front(tokens).first=="static" || Front(tokens).first=="field"))
		output += compileClassVarDec(tokens, ind + indent_config);
	// subroutineDec*
	while (Front(tokens).first=="constructor" ||
		Front(tokens).first=="function" ||
		Front(tokens).first=="method")
		output += compileSubroutine(tokens, ind + indent_config);
	output += xml_write_and_check(tokens, SYMBOL, "}", ind + indent_config);
	output += string(ind, ' ') + "</class>\n";

	return output;
}

string Compiler::compileClassVarDec(tContainer& tokens, int ind) {

	/**
	* ('static'|'field') type varName(',' varName)*';'
	* field int/myClass v1, v2;
	*/

	string output = string(ind, ' ') + "<classVarDec>\n";

	output += xml_write_and_check(tokens, KEYWORD, "", ind + indent_config);
	output += xml_dynamic_type(tokens, ind + indent_config); // type
	output += xml_write_and_check(tokens, IDENTIFIER, "", ind + indent_config); // varName
	while (Front(tokens).first!=";") { // (, varName)
		output += xml_write_and_check(tokens, SYMBOL, ",", ind + indent_config);
		output += xml_write_and_check(tokens, IDENTIFIER, Front(tokens).first, ind + indent_config);
	}
	output += xml_write_and_check(tokens, SYMBOL, ";", ind + indent_config);
	output += string(ind, ' ') + "</classVarDec>\n";

	return output;
}
