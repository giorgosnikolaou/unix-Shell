enum ID {
	PIPE,
	WRITE,
	READ,
	APPEND,
	BG,
	QM,
	NL,
	WORD,
};

#pragma once

#include <string>

#define string std::string

class Token {
	private:
		ID id;
		string value;
	public:
		Token(ID id, string value);
		void print();
		bool cmp(ID id);
		ID gid();
		string gvalue();
		static string gname(ID id);
};

