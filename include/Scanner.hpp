enum State {
	INITIAL,
	STRING_STATE1,
	STRING_STATE2
};

#pragma once
#include <vector>
#include "ids.hpp"

#define vector std::vector

// // // // // // // // // // // // // // // // // // // //
//                      HOW TO USE                       //
// // // // // // // // // // // // // // // // // // // //
/*
	Scanner s = Scanner("./test_input.txt");

	while (s.next());

	s.print();
*/

class Scanner {
	public:
		Scanner();
		~Scanner();
		Token* next();
		void scan(string str);
		void print();

	private:
		string str;
		size_t i;

		vector<Token*> tokens;


		char ahead;
		void read();

		size_t ind;
		bool get_next();
		string read_str(char quotes);
		string read_word();
};

class Exception : public std::exception
{
	private:
		const string msg;

    public:
        Exception(const string& msg_);
        virtual const char* what() const throw ();
};
