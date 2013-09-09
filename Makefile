SRC = tap_eval.c tap_parser.c
OBJ = $(SRC:.c=.o)

LIB = TapParser
LIB_NAME = lib$(LIB).a

test_SRC = test.c
test_OBJ = $(test_SRC:.c=.o)

# gnu99 for strdup and strncasecmp
CFLAGS = -std=gnu99 -Wall -Werror

test_LDFLAGS = -static -L. -l$(LIB)

all: options lib

.PHONY: options
options:
	@echo c-tap-parser build options:
	@echo "CFLAGS = $(CFLAGS)"
	@echo "CC     = $(CC)"

.PHONY: lib
lib:
	@echo CC -c $(SRC)
	@$(CC) $(CFLAGS) -c $(SRC)
	@echo ar rcs $(LIB_NAME) $(OBJ)
	@ar rcs $(LIB_NAME) $(OBJ)

.PHONY: testprog
testprog: lib $(test_OBJ)
	@echo CC -o test
	@$(CC) -o test $(test_OBJ) $(test_LDFLAGS)


.PHONY: clean
clean:
	@rm -f test $(LIB_NAME)
	@rm -f $(OBJ) $(test_OBJ)
