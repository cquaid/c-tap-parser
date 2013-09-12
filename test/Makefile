SRC = test.c
OBJ = $(SRC:.c=.o)

LIB ?= TapParser
LIB_NAME = lib$(LIB).a

CLFLAGS = -std=gnu99 -Wall -Werror -I../
LDFLAGS = -static -L../ -l$(LIB)

all: test

.PHONY: test
test: $(OBJ)
	@echo CC -o test
	@$(CC) -o test $(OBJ) $(LDFLAGS)

.PHONY: clean
clean:
	@rm -f test $(OBJ)
