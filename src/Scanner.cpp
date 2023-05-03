#include <iostream>
#include <string>
#include <glob.h>

#include <cassert>
#include <regex>

#include "Scanner.hpp"


Exception::Exception(const string& msg_) : msg(msg_) { }
const char* Exception::what() const throw () { return msg.c_str(); }


void Scanner::read() {
	ahead = str[i++];
	// fprintf(stderr, "%c", ahead);
}

Scanner::Scanner() {  }

Scanner::~Scanner() {
	for (Token* t : tokens)
		delete t;
}

void Scanner::scan(string str_) {

	str = str_;
	i = 0;
	ind = 0;

	for (Token* t : tokens)
		delete t;

	for (size_t i = 0, size = tokens.size(); i < size; i++)
		tokens.pop_back();

	read();

	while (get_next());
}

Token* Scanner::next() {
	if (ind >= tokens.size())
		return NULL;

	return tokens[ind++];
}

void Scanner::print() {
	for (Token* token : tokens)
		token->print();
}

static bool is_whitespace(char ch) {
	return ch == ' ' || ch == '\t' || ch == '\r';
}

static bool is_key(char ch) {
	return ch == '|' || ch == '<' || ch == '>' || ch == ';' || ch == '&';
}

string Scanner::read_str(char quotes) {
	string ret = "";
	read();

	bool escaped = false;

	while (ahead != quotes || escaped) {
		if (str.length() < i) 
			throw Exception("Missing closing quotes\n");
		
		if (ahead == '*' || ahead == '?') {
			ret += '\\';
			ret += ahead;
		}
		else
			ret += ahead;

		escaped = ahead == '\\' && !escaped;
		read();
	}

	return ret;
}

string Scanner::read_word() {	
	string ret = "";

	
	std::smatch res;  
	const std::regex exp(R"(\$[a-zA-Z_][a-zA-Z_0-9]*)");
	string my_str = "";

	bool escaped = false;

	// account for env vars and replace
	while (!(is_whitespace(ahead) || is_key(ahead) || ahead == '\n') || escaped) {
		if (ahead == '$' && !escaped) {
			my_str = str.substr(i - 1);
			
			if (std::regex_search(my_str, res, exp)) {
				string env_var = res[0];
				env_var = env_var.substr(1);

				for (size_t i = 0; i < env_var.length(); i++)
					read();

				char* rep = std::getenv(env_var.data());
				ret += rep ? rep : "";
			}
			else
				ret += ahead;
		}
		else if ((ahead == '"' || ahead == '\'') && !escaped)
			ret += read_str(ahead);
		else 
			ret += ahead;
		
		escaped = ahead == '\\' && !escaped;
		read();
	}

	return ret;
}

bool Scanner::get_next() {

	while (is_whitespace(ahead)) 
		read();

	if (i > str.length())
		return false;

	Token* ret = NULL;
	int add = 1;

	if (ahead == '|') {
		ret = new Token(PIPE, "|");
		read();	
	}
	else if (ahead == '<') {
		ret = new Token(READ, "<");
		read();
	}
	else if (ahead == '>') {
		ret = (str[i] == '>') ? read(), new Token(APPEND, ">>") : new Token(WRITE, ">");
		read();
	}
	else if (ahead == ';') {
		ret = new Token(QM, ";");
		read();
	}
	else if (ahead == '&') {
		ret = new Token(BG, "&");
		read();
	}
	else if (ahead == '\n') {
		ret = new Token(NL, "new line");
		read();
	}
	else {
		// glob here and append to vector with tokens
		string to_glob = read_word();

		glob_t gstruct;
		add = glob(to_glob.data(), GLOB_ERR , NULL, &gstruct);

		if(add) {
			if (add != GLOB_NOMATCH && add != GLOB_MARK) 
				throw Exception("Error while globing!\n");
			
			ret = new Token(WORD, to_glob);
		}
		else {
			
			char** found = gstruct.gl_pathv;
			while(*found) {
				tokens.push_back(new Token(WORD, *found));
				found++;
			}
		}

		globfree(&gstruct);
	}

	if (add)
		tokens.push_back(ret);

	return true; // return token
}

