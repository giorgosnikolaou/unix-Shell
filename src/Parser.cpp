#include "Parser.hpp"

Parser::Parser() : scanner(Scanner()) {  }
Parser::~Parser() { }

Node* Parser::parse(const char* input) {
    scanner.scan(input);
    ahead = scanner.next();

    return commands();
}

void Parser::consume(ID id) {
    if (!ahead || !ahead->cmp(id)) 
        throw Exception("Parse error, expected " + Token::gname(id) + ", but got " + (ahead ? Token::gname(ahead->gid()) : "NULL"));
    
    ahead = scanner.next();
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
    if (consume_on_cond(QM) && ahead && !ahead->cmp(NL)) { // LL(1.5) :-)
        Command* com = command();
        coms->add(com);
        
        commands_tail(coms);
    }
}
void Parser::commands_end() {
    consume_on_cond(QM);
    consume(NL);
}

Command* Parser::command() {
    BasicCommand* com = cmd();
    return command_end((ahead && ahead->cmp(PIPE)) ? piped_cmd(com) : com);
}
Command* Parser::piped_cmd(BasicCommand* left) {
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
    if (ahead && (ahead->cmp(READ) || ahead->cmp(WRITE) || ahead->cmp(APPEND))) {
        IO* io = red();
        bc->add_io(io);
        cmd_prefix(bc);
    }
}
void Parser::cmd_suffix(BasicCommand* bc) {
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
    IO* io = red_op();

    Token* file = ahead;
    consume(WORD);

    io->set_file(file->gvalue());

    return io;
}
IO* Parser::red_op() {
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
    Token* arg = ahead;
    if (consume_on_cond(WORD)) {
        bc->add_arg(arg->gvalue());
        cmd_arg(bc);
    }
}
