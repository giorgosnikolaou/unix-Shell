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
		void print() const;
		bool cmp(ID id) const;
		ID gid() const;
		string gvalue() const;
		static string gname(ID id);
};

