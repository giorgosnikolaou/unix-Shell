static const char* name[] = { "PIPE", "WRITE", "READ", "APPEND", "BG", "QM", "NL", "WORD" };

#include <iostream>
#include "ids.hpp"

Token::Token(ID id_, string value_) : id(id_), value(value_) { }
void Token::print() const {
	fprintf(stderr, "ID: %10s,	value: %s\n", name[id], value.data());
}
bool Token::cmp(ID id_) const { return id == id_; }
ID Token::gid() const { return id; }
string Token::gvalue() const { return value;}
string Token::gname(ID id) {
	printf("id: %d\n", id);
	return name[id];
}

