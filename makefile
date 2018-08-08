ROOT=$(shell pwd)
SERVER=$(ROOT)/src
INCL=$(ROOT)/include
# CLIENT=$(ROOT)/client
# LOG=$(ROOT)/log
# POOL=$(ROOT)/data_pool
# COMM=$(ROOT)/comm
# LIB=$(ROOT)/lib
# WINDOW=$(ROOT)/window
# CONF=$(ROOT)/conf
# PLUGIN=$(ROOT)/plugin



SERVER_BIN=web_server


INCLUDE=-I$(INCL)
LDFLAGS= -lpthread

SERVER_OBJ=$(shell ls $(SERVER) | grep -E '\.cc$$' | sed 's/\.cc/\.o/')

CC=g++

.PHONY:all
all:$(SERVER_BIN)

# $(SERVER_BIN):$(SERVER_OBJ)
$(SERVER_BIN):$(SERVER_OBJ)
	@$(CC) -o $@ $^ $(LDFLAGS)
	@echo "linking [$^] to [$@] .....done"

%.o:$(SERVER)/%.cc
	@$(CC) -c  $< $(INCLUDE)
	@echo "comping [$^] to [$@]......done"

.PHONY:debug
debug:
	echo $(SERVER_OBJ)
.PHONY:clean
clean:
	rm -rf *.o $(SERVER_BIN)  output

