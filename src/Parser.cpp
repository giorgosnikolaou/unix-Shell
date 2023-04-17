#include "Parser.hpp"

Parser::Parser() : scanner(Scanner()) {  }
Parser::~Parser() { }

Node* Parser::parse(const char* input) {
    scanner.scan(input);
    ahead = scanner.next();
    
    // ahead->print();
    // scanner.print();

    return commands();
}

void Parser::consume(ID id) {
    if (!ahead || !ahead->cmp(id)) 
        throw Exception("Parse error, expected " + Token::gname(id) + ", but got " + (ahead ? Token::gname(ahead->gid()) : "NULL"));
    
    ahead = scanner.next();

    // if (ahead)
    //     ahead->print();
    // printf("\tConsumed: %s\tAhead: %s\n", Token::gname(id).data(), ahead ? Token::gname(ahead->gid()).data() : "");
}

bool Parser::consume_on_cond(ID id) {
    if (!ahead)
        return false;

    if (ahead->cmp(id)) {
        consume(id);
        return true;
    }

    return false;
        
}

Commands* Parser::commands() {
    // printf("commands\n");
    Commands* coms = new Commands();
    
    Command* com = command();
    coms->add(com);

    commands_tail(coms);
    commands_end();

    if (ahead)
        throw Exception("More input found when it shoudln't!");

    return coms;
}
void Parser::commands_tail(Commands* coms) {
    // printf("commands_tail\n");
    if (consume_on_cond(QM) && ahead && !ahead->cmp(NL)) { // LL(1.5) :-)
        Command* com = command();
        coms->add(com);
        
        commands_tail(coms);
    }
}
void Parser::commands_end() {
    // printf("commands_end\n");
    consume_on_cond(QM);
    consume(NL);
}

Command* Parser::command() {
    // printf("command\n");
    BasicCommand* com = cmd();
    return command_end((ahead && ahead->cmp(PIPE)) ? piped_cmd(com) : com);
}
Command* Parser::piped_cmd(BasicCommand* left) {
    // printf("piped_cmd1\n");
    if (consume_on_cond(PIPE)) {
        BasicCommand* right = cmd();
        return new Pipe(left, piped_cmd(right));
    }

    SubPipe* ret = new SubPipe(left);
    delete left;
    return ret;
}

Command* Parser::command_end(Command* command) {
    command->bg = consume_on_cond(BG);
    return command;
}

BasicCommand* Parser::cmd() {
    // printf("cmd\n");

    BasicCommand* bc = new BasicCommand();

    cmd_prefix(bc);

    Token* executable = ahead;
    consume(WORD);

    bc->set_exe(executable->gvalue());
    
    cmd_suffix(bc);

    bc->build();

    return bc;
}
void Parser::cmd_prefix(BasicCommand* bc) {
    // printf("cmd_prefix\n");
    if (ahead && (ahead->cmp(READ) || ahead->cmp(WRITE) || ahead->cmp(APPEND))) {
        IO* io = red();
        bc->add_io(io);
        cmd_prefix(bc);
    }
}
void Parser::cmd_suffix(BasicCommand* bc) {
    // printf("cmd_suffix\n");
    if (ahead && (ahead->cmp(READ) || ahead->cmp(WRITE) || ahead->cmp(APPEND))) {
        IO* io = red();
        bc->add_io(io);
        cmd_suffix(bc);
    }
    else if (ahead && ahead->cmp(WORD)) {
        cmd_arg(bc);
        cmd_suffix(bc);
    }
}

IO* Parser::red() {
    // printf("red\n");
    IO* io = red_op();

    Token* file = ahead;
    consume(WORD);

    io->set_file(file->gvalue());

    return io;
}
IO* Parser::red_op() {
    // printf("red_op\n");
    if (consume_on_cond(READ)) {
        return new In();
    }
    else if (consume_on_cond(WRITE)) {
        return new Out();
    }

    consume(APPEND);

    return new Out(true);
}

void Parser::cmd_arg(BasicCommand* bc) {
    // printf("cmd_arg\n");
    Token* arg = ahead;
    if (consume_on_cond(WORD)) {
        bc->add_arg(arg->gvalue());
        cmd_arg(bc);
    }
}
