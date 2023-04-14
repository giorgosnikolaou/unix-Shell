#pragma once

#include "Scanner.hpp"
#include "Classes.hpp"

class Parser {
    private:
        Scanner scanner;
        Token* ahead;
        void consume(ID id);
        bool consume_on_cond(ID id);

        Commands* commands();
        void commands_tail(Commands* coms);
        void commands_end();

        Command* command();
        Command* piped_cmd(BasicCommand* left);
        Command* piped_cmd(Pipe* left);

        BasicCommand* cmd();
        void cmd_prefix(BasicCommand* bc);
        void cmd_suffix(BasicCommand* bc);

        IO* red();
        IO* red_op();

        void cmd_arg(BasicCommand* bc);        

    public:
        Parser();
        ~Parser();
        Node* parse(const char* input);
};




