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
#include <stdexcept>

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
	
	static unordered_map<tType, string> TokenTypeTable;
	static unordered_set<char> Symbols;
	static unordered_set<string> KeyWords;

	static tContainer tokenizer(istream&);
	static tType tokenType(string);
	static string xml_single_token(tType, string);
	static string xml_write_and_check(tContainer&, tType, const string);
	static string xml_dynamic_type(tContainer&);
	
	static string compileDo(tContainer&);
	static string compileLet(tContainer&);
	static string compileIf(tContainer&);
	static string compileWhile(tContainer&);
	static string compileReturn(tContainer&);
	static string compileStatements(tContainer&);
	static string compileExpression(tContainer&);
	static string compileExpressionList(tContainer&);
	static string compileTerm(tContainer&);
	static string compileVarDec(tContainer&);
	static string compileParameterList(tContainer&);
	static string compileClass(tContainer&);
	static string compileClassVarDec(tContainer&);
	static string compileSubroutine(tContainer&);
	static string compileSubroutineCall(tContainer&);
	static string compileSubroutineBody(tContainer&);
};

const tType Compiler::ERROR;
const tType Compiler::IDENTIFIER;
const tType Compiler::KEYWORD;
const tType Compiler::SYMBOL;
const tType Compiler::INT_CONSTANT;
const tType Compiler::STRING_CONSTANT;

unordered_map<tType, string> Compiler::TokenTypeTable = {
	pair<tType, string> (Compiler::ERROR, ""),
	pair<tType, string> (Compiler::IDENTIFIER, "identifier"),
	pair<tType, string> (Compiler::KEYWORD, "keyword"),
	pair<tType, string> (Compiler::SYMBOL, "symbol"),
	pair<tType, string> (Compiler::INT_CONSTANT, "integerConstant"),
	pair<tType, string> (Compiler::STRING_CONSTANT, "stringConstant")
};

unordered_set<char> Compiler::Symbols = {
	'{','}','(',')','[',']','.',',',';','=','+','-','*','/','~','&','|','<','>'
};

unordered_set<string> Compiler::KeyWords = {
	"class", "constructor", "function", "method", "field", "static",
	"var", "int", "char", "boolean", "void", "true", "false", "null",
	"this", "let", "do", "if", "else", "while", "return"
};

/* Record lines to trace back where error happens */
vector<string> lineTracer;
/* Record new-declared className, unfunctioned under Jack */
unordered_set<string, pair<string, string> > SymbolTable;

int main(int argc, char** argv) {

	string output;
	ifstream ifile;
	ostream& ofile = cout;
	vector<string> ifilenames;
	vector<string> codes;

	for (int i=1; i<argc; i++) ifilenames.emplace_back(argv[i]);

	for (string ifilename : ifilenames) {

		ifile.open(ifilename, std::ios::in);
		if (!ifile) { cerr << "Cannot open: " << ifilename << endl; continue; }

		// for (auto s:Compiler::tokenizer(ifile)) cout << s << "#" << endl;

		tContainer tokens = move(Compiler::tokenizer(ifile));
			
		try {
			output = Compiler::compileClass(tokens);
		} catch (exception const& e) {
			cerr << e.what() << endl;
			ifile.close();
			exit(1);
		}
		
		ofile << output;
		ifile.close();
	}

	return 0;
}

tToken Front(const tContainer& tokens) {
	if (tokens.empty()) {
		cerr << "Incomplete file" << endl;
		throw runtime_error("Pop an empty list");
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

	while (getline(in, line)) {

		lineTracer.push_back(line);
		cnt_line++;

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
				if (c==' ' || c=='"' || c=='\n' || Symbols.count(c))
					i--; // to comprehense i++ each loop
			} else if (Symbols.count(c)) { // symbol
				token.push_back(c);
			} else if (c=='"') { // string constant
				token.push_back('"');
				do {
					c = line[++i];
					token.push_back(c);
				} while (c!='"' && i<line.size()-1);
			}
			if (token!="") tokens.emplace_back(token, cnt_line++);
		}
	}

	return tokens;
}

tType Compiler::tokenType(string token) {
	if (KeyWords.count(token)) return KEYWORD;
	else if (Symbols.count(token[0])) return SYMBOL;
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

string Compiler::xml_single_token(tType type, string token) {

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
		cerr << "Error type: " << token << endl;
		throw runtime_error("Unknown type");
	}
	return "<" + TokenTypeTable[type] + "> " +
		token + " </" + TokenTypeTable[type] + ">\n";
}

string Compiler::xml_write_and_check(tContainer& tokens, tType type, const string expected_token) {

	/**
	* Write xml, check syntax given expected token, move forward
	*/

	string token = Front(tokens).first;
	int errorLine = Front(tokens).second;
	if (expected_token=="") {
		if (tokenType(token)!=type) {
			cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
			cerr << "Error: invalid " << TokenTypeTable[type] << ": " << token << endl;
			throw runtime_error("Incorrect token");
		}
	} else if (token!=expected_token) {
		cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
		cerr << "Error: \"" << token << "\" should be \"" << expected_token << "\"\n";
		throw runtime_error("Incorrect token");
	}
	tokens.pop_front();
	return xml_single_token(type, token);
}

string Compiler::xml_dynamic_type(tContainer& tokens) {

	/**
	* In case type is ambiguous. Eg. int var1; Square var2;
	*/

	string token = Front(tokens).first;
	tokens.pop_front();
	return xml_single_token(tokenType(token), token);
/**
	The reason I keep this unfunctioned codes segment is to keep type check function available.
	If compiler never sees a className it is undefined error.
	But Jack seems not to support #include or import father files.

	if (tokenType(token)==KEYWORD)
		return xml_single_token(KEYWORD, token);
	else if (tokenType(token)==IDENTIFIER && classNameTable.count(token))
		return xml_single_token(IDENTIFIER, token);
	else {
		int errorLine = Front(tokens).second;
		cerr << "Line " << errorLine << ": " << lineTracer[errorLine] << endl;
		cerr << "Error: Undefined type or class: \"" << token << "\"\n";
		exit(1);
	}*/
}

string Compiler::compileVarDec(tContainer& tokens) {

	/** var type varName(, "varName")* */

	string output = "<varDec>\n";

	output += xml_write_and_check(tokens, KEYWORD, "var"); // var
	output += xml_dynamic_type(tokens); // type
	output += xml_write_and_check(tokens, IDENTIFIER, ""); // varName
	while (Front(tokens).first!=";") {
		output += xml_write_and_check(tokens, SYMBOL, "");
		output += xml_write_and_check(tokens, IDENTIFIER, "");
	}
	output += xml_write_and_check(tokens, SYMBOL, "");
	output += "</varDec>\n";

	return output;
}

string Compiler::compileTerm(tContainer& tokens) {

	/**
	* integerConstant | stringConstant | keywordConstant |
	* varName | varName '[' expression ']' | '.' subroutineCall |
	* '(' expression ')' | unaryOp term
	*/

	string output = "<term>\n";
	tContainer::iterator it;

	if (tokenType(Front(tokens).first) == IDENTIFIER) {
		it = tokens.begin(); it++;
		if (it==tokens.end()) {
			cerr << "LL(2): Incomplete file" << endl;
			exit(1);
		}
		else if (it->first==".")
			output += compileSubroutineCall(tokens); // No keyword: subroutineCall
		else if (it->first=="[") {
			output += xml_write_and_check(tokens, IDENTIFIER, "");
			output += xml_write_and_check(tokens, SYMBOL, "[");
			output += compileExpression(tokens);
			output += xml_write_and_check(tokens, SYMBOL, "]");
		} else {
			output += xml_write_and_check(tokens, IDENTIFIER, "");
		}
	} else if (Front(tokens).first == "-" || Front(tokens).first == "~") {
		output += xml_write_and_check(tokens, SYMBOL, "");
		output += compileTerm(tokens);
	} else if (Front(tokens).first == "(") {
		output += xml_write_and_check(tokens, SYMBOL, "(");
		output += compileExpression(tokens);
		output += xml_write_and_check(tokens, SYMBOL, ")");
	} else {
		tType type = tokenType(Front(tokens).first);
		if (type==INT_CONSTANT)
			output += xml_write_and_check(tokens, INT_CONSTANT, "");
		else if (type==STRING_CONSTANT)
			output += xml_write_and_check(tokens, STRING_CONSTANT, "");
		else if (type==KEYWORD)
			output += xml_write_and_check(tokens, KEYWORD, "");
		else
			output += xml_write_and_check(tokens, ERROR, "");
	}
	output += "</term>\n";

	return output;
}

string Compiler::compileExpression(tContainer& tokens) {

	/**
	* term (opr term)*
	* x+y*z-1~2/3
	*/

	string output = "<expression>\n";

	output += compileTerm(tokens);
	while (Front(tokens).first.find_first_of("+-*/&|<>=")!=-1) {
		output += xml_write_and_check(tokens, SYMBOL, "");
		output += compileTerm(tokens);
	} output += "</expression>\n";

	return output;
}

string Compiler::compileExpressionList(tContainer& tokens) {

	/**
	* (expression (',' expression)* )?
	* x+y, y+z, 1+2
	*/

	string output = "<expressionList>\n";

	while (Front(tokens).first!=")") {
		if (Front(tokens).first!=",") {
			output += compileExpression(tokens);
		} else {
			output += xml_write_and_check(tokens, SYMBOL, ",");
			output += compileExpression(tokens);
		}
	} output += "</expressionList>\n";

	return output;
}

string Compiler::compileSubroutine(tContainer& tokens) {

	/**
	* ('constructor' | 'function' | 'method') ('void' | type)
	* subroutineName '(' parameterList ')' subroutineBody
	* type: 'int' | 'char' | 'boolean' | className
	* method void print(char c1, char c2) {..}
	*/

	string output = "<subroutineDec>\n";

	output += xml_write_and_check(tokens, KEYWORD, "");
	if (KeyWords.count(Front(tokens).first))
		output += xml_write_and_check(tokens, KEYWORD, "");
	else
		output += xml_write_and_check(tokens, IDENTIFIER, "");
	output += xml_write_and_check(tokens, IDENTIFIER, "");
	output += xml_write_and_check(tokens, SYMBOL, "(");
	output += compileParameterList(tokens);
	output += xml_write_and_check(tokens, SYMBOL, ")");
	// subroutineBody
	output += compileSubroutineBody(tokens);
	output += "</subroutineDec>\n";

	return output;
}

string Compiler::compileParameterList(tContainer& tokens) {

	/**
	* ((type varName) (',' type varName)*)?
	* int x, int y, myClass obj
	*/

	string output = "<parameterList>\n";

	while (Front(tokens).first!=")") {
		if (Front(tokens).first!=",") {
			output += xml_dynamic_type(tokens); // type
			output += xml_write_and_check(tokens, IDENTIFIER, "");
		} else {
			output += xml_write_and_check(tokens, SYMBOL, ",");
			output += xml_dynamic_type(tokens); // type
			output += xml_write_and_check(tokens, IDENTIFIER, Front(tokens).first);
		}
	} output += "</parameterList>\n";

	return output;
}

string Compiler::compileSubroutineCall(tContainer& tokens) {

	/**
	* subroutineName '(' expressionList ')' |
	* (className|varName) '.' subroutineName '(' expressionList ')'
	* print(x+y, z)
	* sys.println(str1, str2)
	*/

	string output = xml_write_and_check(tokens, IDENTIFIER, "");
	if (Front(tokens).first==".") {
		output += xml_write_and_check(tokens, SYMBOL, ".");
		output += xml_write_and_check(tokens, IDENTIFIER, "");
		output += xml_write_and_check(tokens, SYMBOL, "(");
		output += compileExpressionList(tokens);
		output += xml_write_and_check(tokens, SYMBOL, ")");
	} else if (Front(tokens).first=="(") {
		output += xml_write_and_check(tokens, SYMBOL, "(");
		output += compileExpressionList(tokens);
		output += xml_write_and_check(tokens, SYMBOL, ")");
	}

	return output;
}

string Compiler::compileSubroutineBody(tContainer& tokens) {

	/**
	* '{' varDec* statements '}'
	*/

	string output = "<subroutineBody>\n";

	output += xml_write_and_check(tokens, SYMBOL, "{");
	while (Front(tokens).first=="var")
		output += compileVarDec(tokens);
	output += compileStatements(tokens);
	output += xml_write_and_check(tokens, SYMBOL, "}");
	output += "</subroutineBody>\n";

	return output;
}

string Compiler::compileStatements(tContainer& tokens) {

	/**
	* statement*
	*/

	string output = "<statements>\n";

	while (Front(tokens).first!="}") {
		string tmp = Front(tokens).first;
		if (tmp=="let") output += compileLet(tokens);
		else if (tmp=="do") output += compileDo(tokens);
		else if (tmp=="if") output += compileIf(tokens);
		else if (tmp=="while") output += compileWhile(tokens);
		else if (tmp=="return") output += compileReturn(tokens);
		else { xml_write_and_check(tokens, ERROR, ""); }
	} output += "</statements>\n";

	return output;
}

string Compiler::compileLet(tContainer& tokens) {

	/**
	* 'let' varName('[' expression ']')? '=' expression ';'
	*/

	string output = "<letStatement>\n";

	output += xml_write_and_check(tokens, KEYWORD, "let");
	output += xml_write_and_check(tokens, IDENTIFIER, "");

	if (Front(tokens).first=="[") {
		output += xml_write_and_check(tokens, SYMBOL, "[");
		output += compileExpression(tokens);
		output += xml_write_and_check(tokens, SYMBOL, "]");
		output += xml_write_and_check(tokens, SYMBOL, "=");
	} else if (Front(tokens).first=="=") {
		output += xml_write_and_check(tokens, SYMBOL, "=");
	}

	output += compileExpression(tokens);
	output += xml_write_and_check(tokens, SYMBOL, ";");
	output += "</letStatement>\n";

	return output;
}

string Compiler::compileDo(tContainer& tokens) {

	/**
	* 'do' subroutineCall ';'
	*/

	string output = "<doStatement>\n";
	output += xml_write_and_check(tokens, KEYWORD, "do");
	output += compileSubroutineCall(tokens);
	output += xml_write_and_check(tokens, SYMBOL, ";");
	output += "</doStatement>\n";

	return output;
}

string Compiler::compileIf(tContainer& tokens) {

	/**
	* 'if' '(' expression ')' '{' statements '}' ('else' '{' statements '}')?
	*/

	string output = "<ifStatement>\n";

	output += xml_write_and_check(tokens, KEYWORD, "if");
	// (Expression)
	output += xml_write_and_check(tokens, SYMBOL, "(");
	output += compileExpression(tokens);
	output += xml_write_and_check(tokens, SYMBOL, ")");
	// {Statements}
	output += xml_write_and_check(tokens, SYMBOL, "{");
	output += compileStatements(tokens);
	output += xml_write_and_check(tokens, SYMBOL, "}");
	// (else {statememts})?
	if (Front(tokens).first=="else") {
		output += xml_write_and_check(tokens, KEYWORD, "else");
		output += xml_write_and_check(tokens, SYMBOL, "{");
		output += compileStatements(tokens);
		output += xml_write_and_check(tokens, SYMBOL, "}");
	} output += "</ifStatement>\n";

	return output;
}

string Compiler::compileWhile(tContainer& tokens) {

	/**
	* 'while' '(' expression ')' '{' statements '}'
	*/

	string output = "<whileStatement>\n";

	output += xml_write_and_check(tokens, KEYWORD, "while");
	// (expression)
	output += xml_write_and_check(tokens, SYMBOL, "(");
	output += compileExpression(tokens);
	output += xml_write_and_check(tokens, SYMBOL, ")");
	// {statements}
	output += xml_write_and_check(tokens, SYMBOL, "{");
	output += compileStatements(tokens);
	output += xml_write_and_check(tokens, SYMBOL, "}");
	output += "</whileStatement>\n";

	return output;
}

string Compiler::compileReturn(tContainer& tokens) {

	/**
	* 'return' expression? ';'
	*/

	string output = "<returnStatement>\n";

	output += xml_write_and_check(tokens, KEYWORD, "return");
	// Expression?
	if (Front(tokens).first!=";")
		output += compileExpression(tokens);
	output += xml_write_and_check(tokens, SYMBOL, ";");
	output += "</returnStatement>\n";

	return output;
}

string Compiler::compileClass(tContainer& tokens) {

	/**
	* 'class' className '{' classVarDec* subroutineDec* '}'
	*/

	string output = "<class>\n";
	output += xml_write_and_check(tokens, KEYWORD, "class");
/*
	string className = Front(tokens).first;
	if (classNameTable.count(className)) {
		int errorLine = Front(tokens).second;
		cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
		cerr << "Redefinition of class: " << className << endl;
		exit(1);
	} classNameTable.insert(className);
*/
	output += xml_write_and_check(tokens, IDENTIFIER, "");
	output += xml_write_and_check(tokens, SYMBOL, "{");
	// classVarDec*
	while ((Front(tokens).first=="static" || Front(tokens).first=="field"))
		output += compileClassVarDec(tokens);
	// subroutineDec*
	while (Front(tokens).first=="constructor" ||
		Front(tokens).first=="function" ||
		Front(tokens).first=="method")
		output += compileSubroutine(tokens);
	output += xml_write_and_check(tokens, SYMBOL, "}");
	output += "</class>\n";

	return output;
}

string Compiler::compileClassVarDec(tContainer& tokens) {

	/**
	* ('static'|'field') type varName(',' varName)*';'
	* field int/myClass v1, v2;
	*/

	string output = "<classVarDec>\n";

	output += xml_write_and_check(tokens, KEYWORD, "");
	output += xml_dynamic_type(tokens); // type
	output += xml_write_and_check(tokens, IDENTIFIER, ""); // varName
	while (Front(tokens).first!=";") { // (, varName)
		output += xml_write_and_check(tokens, SYMBOL, ",");
		output += xml_write_and_check(tokens, IDENTIFIER, Front(tokens).first);
	}
	output += xml_write_and_check(tokens, SYMBOL, ";");
	output += "</classVarDec>\n";

	return output;
}
