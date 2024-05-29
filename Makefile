SRC=src
OBJ=obj
EXECSRC=executable_src
BIN=bin
INCDIR=include
TESTSRC=test/src
TESTBIN=test/bin

CC=gcc
OPT=-O0
DEPFLAGS=-MP -MD
CFLAGS=-Wall -Werror -g $(foreach D, $(INCDIR), -I$(D)) $(OPT) $(DEPFLAGS)

SRCFILES=$(foreach D, $(SRC), $(wildcard $(D)/*.c))
OBJFILES=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCFILES))
DEPFILES=$(patsubst $(SRC)/%.c, $(OBJ)/%.d, $(SRCFILES))
EXECSRCFILES=$(foreach D, $(EXECSRC), $(wildcard $(D)/*/*.c))
BINFILES=$(patsubst $(EXECSRC)/%/%.c, $(BIN)/%, $(EXECSRCFILES))

TESTSRCFILES=$(foreach D, $(TESTSRC), $(wildcard $(D)/*.c))
TESTBINFILES=$(patsubst $(TESTSRC)/%.c, $(TESTBIN)/%, $(TESTSRCFILES))

build: $(OBJFILES)

build_cli: $(EXECSRC)/cli/cli.c $(OBJFILES)
	$(CC) -g -o$(BIN)/cli $^ -I$(INCDIR) -Wall -Werror $(OPT)

build_client: $(EXECSRC)/client/client.c $(OBJFILES)
	$(CC) -g -o$(BIN)/cli $^ -I$(INCDIR) -Wall -Werror $(OPT)

build_test:$(TESTBINFILES)

$(TESTBIN)/%_test: $(TESTSRC)/%_test.c $(OBJFILES)
	$(CC) -o $@ $^ -I$(INCDIR) -Wall -Werror

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	rm -rf $(OBJFILES) $(DEPFILES) $(TESTBINFILES)

-include $(DEPFILES)
