SRC = test.c test_log.c
OBJ = $(SRC:.c=.o)

LIB ?= TapParser
LIB_NAME = lib$(LIB).a

CFLAGS = -std=gnu99 -Wall -Werror -I$(CURDIR)/..
LDFLAGS = -static -L$(CURDIR)/.. -l$(LIB)

all: test

.PHONY: test
test: $(OBJ)
	@echo CC -o test
	@$(CC) -o test $(OBJ) $(LDFLAGS)

.PHONY: clean
clean:
	@rm -f test $(OBJ)
