# Compiler
CC = g++

# Paths
INCLUDE = ./include
SRC = ./src
MAIN = ./

# Defines 
DEFINES = 

# Compile options
CXXFLAGS = -Wall -g3 $(DEFINES) -I$(INCLUDE) -std=c++11 -pthread

# .o files needed
SRCS    := $(wildcard $(SRC)/*.cpp)
OBJS    := $(patsubst $(SRC)/%.cpp,$(SRC)/%.o,$(SRCS)) $(MAIN)/main.o

# Executable
EXEC = mysh

# Command line arguments
ARGS = 

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC)

clean:
	@rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ARGS)

time: $(EXEC)
	time ./$(EXEC) $(ARGS)
	
valgrind: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./$(EXEC) $(ARGS)