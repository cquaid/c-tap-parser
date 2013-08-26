SRC = tap_eval.c tap_parser.c
OBJ = ${SRC:.c=.o}

test_SRC = test.c $(SRC)
test_OBJ = ${test_SRC:.c=.o}

# gnu99 for strdup and strncasecmp
CFLAGS = -std=gnu99 -Wall -Werror

all: options ${OBJ}

.PHONY: options
options:
	@echo c-tap-parser build options:
	@echo "CFLAGS = ${CFLAGS}"
	@echo "CC     = ${CC}"


testprog: ${test_OBJ}
	@echo "CC -o test"
	@${CC} -o test ${test_OBJ}


.PHONY: clean
clean:
	@rm -f test
	@rm -f $(OBJ) $(test_OBJ)
