SRC = tap_eval.c tap_parser.c
OBJ = $(SRC:.c=.o)

LIB = TapParser
LIB_NAME = lib$(LIB).a

# gnu99 for strdup and strncasecmp
CFLAGS = -std=gnu99 -Wall -Werror

all: options lib

.PHONY: options
options:
	@echo c-tap-parser build options:
	@echo "CFLAGS = $(CFLAGS)"
	@echo "CC     = $(CC)"

.PHONY: lib
lib: $(OBJ)
	@echo ar rcs $(LIB_NAME) $(OBJ)
	@ar rcs $(LIB_NAME) $(OBJ)

.PHONY: testprog
testprog: lib
	@$(MAKE) -C test LIB=$(LIB)


.PHONY: test
test: testprog
	@test/test

.PHONY: clean
clean:
	@rm -f $(LIB_NAME) $(OBJ)
	@$(MAKE) -C test clean
