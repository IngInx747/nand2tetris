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

using tType = int;
using tToken = pair<string, int>;
using tContainer = list<tToken>;

/* A safer front method for list in case list is empty */
tToken Front(const tContainer&);

/* Record lines to trace back where error happens */
vector<string> lineTracer;

struct Compiler {

	const static tType ERROR = -1;
	const static tType IDENTIFIER = 0;
	const static tType KEYWORD = 1;
	const static tType SYMBOL = 2;
	const static tType INT_CONSTANT = 3;
	const static tType STRING_CONSTANT = 4;

	static string stoken;
	static string stype;
	static string skind;

	static string className;
	static string functionName;

	static int argCnt;
	static int varCnt;
	static int staticCnt;
	static int fieldCnt;
	static int expressionListCnt;
	static int whileCnt;
	static int ifCnt;
	
	static unordered_map<tType, string> TokenTypeTable;
	static unordered_set<char> Symbols;
	static unordered_set<string> KeyWords;
	
	static unordered_map<string, vector<string> > SymbolTableSubroutineScope;
	static unordered_map<string, vector<string> > SymbolTableClassScope;
	const static int DEFINE_CLASS = 1;
	const static int DEFINE_METHOD = 2;

	static void define_symbol_class();
	static void define_symbol_subroutine();
	static unordered_map<string, vector<string> >::iterator search_symbol(string);

	static tContainer tokenizer(istream&);
	static tType tokenType(string);
	static void xml_proceed_and_check(tContainer&, tType, const string, int=0);
	static void xml_dynamic_type(tContainer&);

	static string vm_single_token(tType, string);
	static string vm_push_array(string);
	
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
};

const tType Compiler::ERROR;
const tType Compiler::IDENTIFIER;
const tType Compiler::KEYWORD;
const tType Compiler::SYMBOL;
const tType Compiler::INT_CONSTANT;
const tType Compiler::STRING_CONSTANT;
const int Compiler::DEFINE_CLASS;
const int Compiler::DEFINE_METHOD;

string Compiler::stoken;
string Compiler::stype;
string Compiler::skind;
string Compiler::className;
string Compiler::functionName;

int Compiler::argCnt = 0;
int Compiler::varCnt = 0;
int Compiler::staticCnt = 0;
int Compiler::fieldCnt = 0;
int Compiler::expressionListCnt = 0;
int Compiler::whileCnt = 0;
int Compiler::ifCnt = 0;
unordered_map<string, vector<string> > Compiler::SymbolTableSubroutineScope;
unordered_map<string, vector<string> > Compiler::SymbolTableClassScope;

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

		// for (auto s:Compiler::tokenizer(ifile)) cout << s.first << "#" << endl;

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

void Compiler::define_symbol_class() {
	int index{};
	if (skind=="this") index = fieldCnt++; // field->this
	else if (skind=="static") index = staticCnt++; // static
	else throw runtime_error("Error: wrong kind for class scope");
	SymbolTableClassScope.emplace(stoken,
		vector<string> {stype, skind, to_string(index)});
}

void Compiler::define_symbol_subroutine() {
	int index{};
	if (skind=="local") index = varCnt++; // var->local
	else if (skind=="argument") index = argCnt++; // var->local
	else throw runtime_error("Error: wrong kind for subroutine scope");
	SymbolTableSubroutineScope.emplace(stoken,
		vector<string> {stype, skind, to_string(index)});
}

unordered_map<string, vector<string> >::iterator
Compiler::search_symbol(string symbol) {
	unordered_map<string, vector<string> >::iterator it;
	it = SymbolTableSubroutineScope.find(symbol);
	if (it != SymbolTableSubroutineScope.end())
		return it;
	it = SymbolTableClassScope.find(symbol);
	if (it != SymbolTableClassScope.end())
		return it;
	cerr << "Error: Undefined symbol " + symbol << endl;
	throw runtime_error("Undefined symbol");
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
			if (token!="") tokens.emplace_back(token, cnt_line);
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

void Compiler::xml_proceed_and_check(
	tContainer& tokens, tType type,
	const string expected_token,
	int DEFINE) {

	/**
	* Write xml, check syntax given expected token, move forward
	*/

	string token = Front(tokens).first;
	int errorLine = Front(tokens).second;

	if (expected_token=="") {
		if (tokenType(token)!=type) {
			cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
			cerr << "Error: invalid " << TokenTypeTable[type] << ": " << token << endl;
			throw runtime_error("Incorrect type");
		}
	} else if (token!=expected_token) {
		cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
		cerr << "Error: \"" << token << "\" should be \"" << expected_token << "\"\n";
		throw runtime_error("Incorrect token");
	}

	stoken = token;

	if (DEFINE == DEFINE_CLASS) {
		if (SymbolTableClassScope.find(stoken)!=SymbolTableClassScope.end()) {
			cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
			cerr << "Redefinition: " + stoken << endl;
			throw runtime_error("Redefinition");
		}
		define_symbol_class();
	} else if (DEFINE == DEFINE_METHOD) {
		if (SymbolTableSubroutineScope.find(stoken)!=SymbolTableSubroutineScope.end()) {
			cerr << "Line " << errorLine << ": " << lineTracer[errorLine - 1] << endl;
			cerr << "Redefinition: " + stoken << endl;
			throw runtime_error("Redefinition");
		}
		define_symbol_subroutine();
	}
	
	tokens.pop_front();
}

void Compiler::xml_dynamic_type(tContainer& tokens) {

	/**
	* In case type is ambiguous. Eg. int var1; Square var2;
	*/
	string token = Front(tokens).first;
	tType type = tokenType(token);
	if (type == KEYWORD || type == IDENTIFIER) stype = token;
	xml_proceed_and_check(tokens, type, "", 0);
}

string Compiler::vm_push_array(string symbol) {

	/**
	* Push array
	*/

	unordered_map<string, vector<string> >::iterator it;
	it = SymbolTableSubroutineScope.find(symbol);
	if (it != SymbolTableSubroutineScope.end())
		return "push " + it->second[1] + " " + it->second[2] + "\n";
	it = SymbolTableClassScope.find(symbol);
	if (it != SymbolTableClassScope.end())
		return "push this " + it->second[2] + "\n";
	cerr << "Error: Undefined symbol " + symbol << endl;
	throw runtime_error("Undefined symbol");
}

string Compiler::vm_single_token(tType type, string token) {

	/**
	* Write a single vm given token's type
	*/

	string output;

	if (type == KEYWORD) {
		if (token == "true")
			output += "push constant 1\nneg\n";
		else if (token == "false" || token == "null")
			output += "push constant 0\n";
		else if (token == "this")
			output += "push pointer 0\n";
	} else if (type == INT_CONSTANT) {
		output += "push constant " + token + "\n";
	} else if (type == STRING_CONSTANT) {
		token = token.substr(1, token.size() - 2);
		output += "push constant " + to_string(token.size()) + "\n";
		output += "call String.new 1\n";
		for (int i=0; i<token.size(); i++) {
			output += "push constant "
				+ to_string(static_cast<int>(token[i]))
				+ "\n" + "call String.appendChar 2\n";
		}
	} else throw runtime_error("In-VM-izable single token");

	return output;
}

string Compiler::compileVarDec(tContainer& tokens) {

	/** var type varName(, "varName")* */

	string output = ""; skind = "local"; // var->local

	xml_proceed_and_check(tokens, KEYWORD, "var"); // var
	xml_dynamic_type(tokens); // type
	xml_proceed_and_check(tokens, IDENTIFIER, "", DEFINE_METHOD); // varName
	while (Front(tokens).first!=";") {
		xml_proceed_and_check(tokens, SYMBOL, ",");
		xml_proceed_and_check(tokens, IDENTIFIER, "", DEFINE_METHOD);
	}
	xml_proceed_and_check(tokens, SYMBOL, ";");

	return output;
}

string Compiler::compileTerm(tContainer& tokens) {

	/**
	* integerConstant | stringConstant | keywordConstant |
	* varName | varName '[' expression ']' | '.' subroutineCall |
	* '(' expression ')' | unaryOp term
	*/

	string output = "";
	string token = Front(tokens).first;
	tContainer::iterator it;

	if (tokenType(token) == IDENTIFIER) {
		it = tokens.begin(); it++;
		if (it==tokens.end()) {
			cerr << "LL(2): Incomplete file" << endl;
			throw runtime_error("Pop an empty list");
		} else if (it->first==".") {
			output += compileSubroutineCall(tokens); // No keyword: subroutineCall
		} else if (it->first=="[") {
			xml_proceed_and_check(tokens, IDENTIFIER, "");
			output += vm_push_array(stoken);
			xml_proceed_and_check(tokens, SYMBOL, "[");
			output += compileExpression(tokens);
			output += "add\npop pointer 1\npush that 0\n";
			xml_proceed_and_check(tokens, SYMBOL, "]");
		} else {
			xml_proceed_and_check(tokens, IDENTIFIER, "");
			auto it = search_symbol(stoken);
			output += "push " + it->second[1] + " " + it->second[2] + "\n";
		}
	} else if (token == "-" || token == "~") {
		string unaryOp = token;
		xml_proceed_and_check(tokens, SYMBOL, "");
		output += compileTerm(tokens);
		if (unaryOp == "-")
			output += "neg\n";
		else
			output += "not\n";
	} else if (token == "(") {
		xml_proceed_and_check(tokens, SYMBOL, "(");
		output += compileExpression(tokens);
		xml_proceed_and_check(tokens, SYMBOL, ")");
	} else {
		tType type = tokenType(token);
		if (type==INT_CONSTANT)
			xml_proceed_and_check(tokens, INT_CONSTANT, "");
		else if (type==STRING_CONSTANT)
			xml_proceed_and_check(tokens, STRING_CONSTANT, "");
		else if (type==KEYWORD)
			xml_proceed_and_check(tokens, KEYWORD, "");
		else
			xml_proceed_and_check(tokens, ERROR, "");
		output += vm_single_token(type, stoken);
	}

	return output;
}

string Compiler::compileStatements(tContainer& tokens) {

	/**
	* statement*
	*/

	string output = "";

	while (Front(tokens).first!="}") {
		string tmp = Front(tokens).first;
		if (tmp=="let") output += compileLet(tokens);
		else if (tmp=="do") output += compileDo(tokens);
		else if (tmp=="if") output += compileIf(tokens);
		else if (tmp=="while") output += compileWhile(tokens);
		else if (tmp=="return") output += compileReturn(tokens);
		else { xml_proceed_and_check(tokens, ERROR, ""); }
	}

	return output;
}

string Compiler::compileLet(tContainer& tokens) {

	/**
	* 'let' varName('[' expression ']')? '=' expression ';'
	*/

	string output = "", token;

	xml_proceed_and_check(tokens, KEYWORD, "let");
	xml_proceed_and_check(tokens, IDENTIFIER, "");
	string letVarName = stoken;

	token = Front(tokens).first;
	if (token=="[") {
		output += vm_push_array(letVarName);
		xml_proceed_and_check(tokens, SYMBOL, "[");
		output += compileExpression(tokens);
		xml_proceed_and_check(tokens, SYMBOL, "]");
		output += "add\n";
		xml_proceed_and_check(tokens, SYMBOL, "=");
		output += compileExpression(tokens);
		output += "pop temp 0\npop pointer 1\npush temp 0\npop that 0\n";
	} else if (token=="=") {
		xml_proceed_and_check(tokens, SYMBOL, "=");
		output += compileExpression(tokens);
		auto it = search_symbol(letVarName);
		output += "pop " + it->second[1] + " " + it->second[2] + "\n";
	} xml_proceed_and_check(tokens, SYMBOL, ";");

	return output;
}

string Compiler::compileDo(tContainer& tokens) {

	/**
	* 'do' subroutineCall ';'
	*/

	string output = "";
	xml_proceed_and_check(tokens, KEYWORD, "do");
	output += compileSubroutineCall(tokens);
	output += "pop temp 0\n";
	xml_proceed_and_check(tokens, SYMBOL, ";");

	return output;
}

string Compiler::compileIf(tContainer& tokens) {

	/**
	* 'if' '(' expression ')' '{' statements '}' ('else' '{' statements '}')?
	*/

	string output = "";
	int subIfFlag = ifCnt++;

	xml_proceed_and_check(tokens, KEYWORD, "if");
	// (Expression)
	xml_proceed_and_check(tokens, SYMBOL, "(");
	output += compileExpression(tokens);
	xml_proceed_and_check(tokens, SYMBOL, ")");
	output += "not\n";
	output += "if-goto " + functionName + ".IF.TRUE." + to_string(subIfFlag) + "\n";
	// {Statements}
	xml_proceed_and_check(tokens, SYMBOL, "{");
	output += compileStatements(tokens);
	xml_proceed_and_check(tokens, SYMBOL, "}");
	// (else {statememts})?
	if (Front(tokens).first=="else") {
		output += "goto " + functionName + ".IF.FALSE." + to_string(subIfFlag) + "\n";
		output += "label " + functionName + ".IF.TRUE." + to_string(subIfFlag) + "\n";
		xml_proceed_and_check(tokens, KEYWORD, "else");
		xml_proceed_and_check(tokens, SYMBOL, "{");
		output += compileStatements(tokens);
		output += "label " + functionName + ".IF.FALSE." + to_string(subIfFlag) + "\n";
		xml_proceed_and_check(tokens, SYMBOL, "}");
	} else {
		output += "label " + functionName + ".IF.TRUE." + to_string(subIfFlag) + "\n";
	}

	return output;
}

string Compiler::compileWhile(tContainer& tokens) {

	/**
	* 'while' '(' expression ')' '{' statements '}'
	*/

	string output = "";
	int subWhileFlag = whileCnt++;
	output += "label " + functionName + ".WHILE.BEGIN." + to_string(subWhileFlag) + "\n";

	xml_proceed_and_check(tokens, KEYWORD, "while");
	// (expression)
	xml_proceed_and_check(tokens, SYMBOL, "(");
	output += compileExpression(tokens);
	xml_proceed_and_check(tokens, SYMBOL, ")");
	output += "not\nif-goto " + functionName +".WHILE.END." + to_string(subWhileFlag) + "\n";
	// {statements}
	xml_proceed_and_check(tokens, SYMBOL, "{");
	output += compileStatements(tokens);
	output += "goto " + functionName + ".WHILE.BEGIN." + to_string(subWhileFlag) + "\n";
	output += "label " + functionName + ".WHILE.END." + to_string(subWhileFlag) + "\n";
	xml_proceed_and_check(tokens, SYMBOL, "}");

	return output;
}

string Compiler::compileReturn(tContainer& tokens) {

	/**
	* 'return' expression? ';'
	*/

	string output = "";

	xml_proceed_and_check(tokens, KEYWORD, "return");
	// Expression?
	if (Front(tokens).first!=";")
		output += compileExpression(tokens);
	else
		output += "push constant 0\n";
	output += "return\n";
	xml_proceed_and_check(tokens, SYMBOL, ";");

	return output;
}

string Compiler::compileExpression(tContainer& tokens) {

	/**
	* term (opr term)*
	* x+y*z-1~2/3
	*/

	string output = ""; string op;

	output += compileTerm(tokens);
	while (Front(tokens).first.find_first_of("+-*/&|<>=")!=-1) {
		op = Front(tokens).first;
		xml_proceed_and_check(tokens, SYMBOL, "");
		output += compileTerm(tokens);
		if (op == "+") output += "add\n";
		else if (op == "-") output += "sub\n";
		else if (op == "*") output += "call Math.multiply 2\n";
		else if (op == "/") output += "call Math.divide 2\n";
		else if (op == "&") output += "and\n";
		else if (op == "|") output += "or\n";
		else if (op == "<") output += "lt\n";
		else if (op == ">") output += "gt\n";
		else if (op == "=") output += "eq\n";
	}

	return output;
}

string Compiler::compileExpressionList(tContainer& tokens) {

	/**
	* (expression (',' expression)* )?
	* x+y, y+z, 1+2
	*/

	string output = ""; expressionListCnt = 0;

	while (Front(tokens).first!=")") {
		if (Front(tokens).first==",")
			xml_proceed_and_check(tokens, SYMBOL, ",");
		output += compileExpression(tokens);
		expressionListCnt++;
	}

	return output;
}

string Compiler::compileParameterList(tContainer& tokens) {

	/**
	* ((type varName) (',' type varName)*)?
	* int x, int y, myClass obj
	*/

	string output = ""; skind = "argument";

	while (Front(tokens).first!=")") {
		if (Front(tokens).first!=",") { // first arg
			xml_dynamic_type(tokens); // type
			xml_proceed_and_check(tokens, IDENTIFIER, "", DEFINE_METHOD);
		} else { // rest args
			xml_proceed_and_check(tokens, SYMBOL, ",");
			xml_dynamic_type(tokens); // type
			xml_proceed_and_check(tokens, IDENTIFIER, "", DEFINE_METHOD);
		}
	}

	return output;
}

string Compiler::compileSubroutine(tContainer& tokens) {

	/**
	* ('constructor' | 'function' | 'method') ('void' | type)
	* subroutineName '(' parameterList ')' subroutineBody
	* type: 'int' | 'char' | 'boolean' | className
	* method void print(char c1, char c2) {..}
	*/

	string output = "", subRoutineFlag = "";
	int lclCnt = 0, fieldCnt = 0;
	whileCnt = ifCnt = varCnt = argCnt = 0;
	SymbolTableSubroutineScope.clear();

	string token = Front(tokens).first; // constructor, method, function
	if (token == "method") {
		stoken = "this"; stype = className; skind = "argument";
		define_symbol_subroutine(); // can't be wrong
		subRoutineFlag = "METHOD";
	} else if (token == "constructor") {
		subRoutineFlag = "CONSTRUCTOR";
	} else {
		subRoutineFlag = "FUNCTION";
	}
	xml_proceed_and_check(tokens, KEYWORD, "");
	
	// (void|type) subroutineName (parameterList)
	xml_dynamic_type(tokens);
	xml_proceed_and_check(tokens, IDENTIFIER, "");
	// xxx.yyy
	functionName = className + "." + stoken;
	output += "// ===== " + functionName + " =====\n";
	xml_proceed_and_check(tokens, SYMBOL, "(");
	output += compileParameterList(tokens);
	xml_proceed_and_check(tokens, SYMBOL, ")");
	// subroutineBody '{' varDec* statements '}'
	xml_proceed_and_check(tokens, SYMBOL, "{");

	while (Front(tokens).first=="var")
		output += compileVarDec(tokens);
	// var->local
	for (auto& p:SymbolTableSubroutineScope)
		if (p.second[1] == "local")
			lclCnt++;
	output += "function " + functionName + " " + to_string(lclCnt) + "\n";
	if (subRoutineFlag == "METHOD") {
		output += "push argument 0\npop pointer 0\n";
	} else if (subRoutineFlag == "CONSTRUCTOR") {
		// field->this
		for (auto& p:SymbolTableClassScope)
			if (p.second[1] == "this")
				fieldCnt++;
		output += "push constant " + to_string(fieldCnt) + "\n";
		output += "call Memory.alloc 1\npop pointer 0\n";
	}
	output += compileStatements(tokens);
	xml_proceed_and_check(tokens, SYMBOL, "}");

	return output + "// ----- " + functionName + " -----\n";;
}

string Compiler::compileSubroutineCall(tContainer& tokens) {

	/**
	* subroutineName '(' expressionList ')' |
	* (className|varName) '.' subroutineName '(' expressionList ')'
	* print(x+y, z)
	* sys.println(str1, str2)
	*/

	string output = "";
	string subClassName, subFunctionName, subIndex, subName, token;
	bool subMethodFlag = false;

	xml_proceed_and_check(tokens, IDENTIFIER, "");
	subClassName = stoken;
	token = Front(tokens).first;

	if (token==".") {
		xml_proceed_and_check(tokens, SYMBOL, ".");
		xml_proceed_and_check(tokens, IDENTIFIER, "");
		subFunctionName = stoken;
		// Check whether subClassName is a class name or an instance
		unordered_map<string, vector<string> >::iterator it;
		it = SymbolTableSubroutineScope.find(subClassName);
		if (it!=SymbolTableSubroutineScope.end()) {
			subMethodFlag = true;
			subClassName = it->second[0];
			subIndex = it->second[2];
			subName = subClassName + "." + subFunctionName;
			output += "push local " + subIndex + "\n";
		}
		it = SymbolTableClassScope.find(subClassName);
		if (it!=SymbolTableClassScope.end()) {
			subMethodFlag = true;
			subClassName = it->second[0];
			subIndex = it->second[2];
			subName = subClassName + "." + subFunctionName;
			output += "push this " + subIndex + "\n";
		} else {
			subName = subClassName + "." + subFunctionName;
		}
		xml_proceed_and_check(tokens, SYMBOL, "(");
		output += compileExpressionList(tokens);
		xml_proceed_and_check(tokens, SYMBOL, ")");
		if (subMethodFlag)
			output += "call " + subName + " " + to_string(expressionListCnt+1) + "\n";
		else
			output += "call " + subName + " " + to_string(expressionListCnt) + "\n";
	} else if (token=="(") {
		subName = className + "." + subClassName;
		xml_proceed_and_check(tokens, SYMBOL, "(");
		output += "push pointer 0\n";
		output += compileExpressionList(tokens);
		output += "call " + subName + " " + to_string(expressionListCnt+1) + "\n";
		xml_proceed_and_check(tokens, SYMBOL, ")");
	}

	return output;
}

string Compiler::compileClass(tContainer& tokens) {

	/**
	* 'class' className '{' classVarDec* subroutineDec* '}'
	*/

	string output = "";
	staticCnt = fieldCnt = 0;
	SymbolTableClassScope.clear();

	xml_proceed_and_check(tokens, KEYWORD, "class");
	xml_proceed_and_check(tokens, IDENTIFIER, ""); // className
	className = stoken;
	output += "// ========== " + className + " ==========\n";
	xml_proceed_and_check(tokens, SYMBOL, "{");
	// classVarDec*
	while ((Front(tokens).first=="static" || Front(tokens).first=="field"))
		output += compileClassVarDec(tokens);
	// subroutineDec*
	while (Front(tokens).first=="constructor" ||
		Front(tokens).first=="function" ||
		Front(tokens).first=="method")
		output += compileSubroutine(tokens);
	xml_proceed_and_check(tokens, SYMBOL, "}");

	return output + "// ---------- " + className + " ----------\n";
}

string Compiler::compileClassVarDec(tContainer& tokens) {

	/**
	* ('static'|'field') type varName(',' varName)*';'
	* field int/myClass v1, v2;
	*/

	string output = ""; skind = Front(tokens).first; // static|field
	if (skind == "field") skind = "this";

	xml_proceed_and_check(tokens, KEYWORD, "");
	xml_dynamic_type(tokens); // type
	xml_proceed_and_check(tokens, IDENTIFIER, "", DEFINE_CLASS); // varName
	while (Front(tokens).first!=";") { // (, varName)
		xml_proceed_and_check(tokens, SYMBOL, ",");
		xml_proceed_and_check(tokens, IDENTIFIER, "", DEFINE_CLASS);
	}
	xml_proceed_and_check(tokens, SYMBOL, ";");

	return output;
}
