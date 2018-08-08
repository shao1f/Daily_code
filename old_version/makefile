ROOT_PATH=$(shell pwd)
LDFLAGS=-lpthread
FLAGS=#-D_DEBUG_#-g

CC=gcc
BIN=myhttpd
SRC=$(shell ls *.c)
OBJ=$(SRC:.c=.o)

CGI=cgi

.PHONY:all cgi
all:$(BIN)
$(BIN):$(OBJ)
	@echo "Linking [$^] to [$@]"
	@$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Linking done..."
%.o:%.c
	@echo "Compling [$<] to [$@]"
	@$(CC) -c $<
	@echo "Compling done..."

.PHONY:clean
clean:
	@rm -rf *.o $(BIN) 


