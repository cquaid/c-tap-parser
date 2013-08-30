SRC = tap_eval.c tap_parser.c
OBJ = ${SRC:.c=.o}

LIB = TapParser
LIB_NAME = lib${LIB}.so

test_SRC = test.c
test_OBJ = ${test_SRC:.c=.o}

# gnu99 for strdup and strncasecmp
CFLAGS = -std=gnu99 -Wall -Werror

lib_LDFLAGS = -shared -Wl,-soname,${LIB_NAME}
lib_CFLAGS = -fpic

test_LDFLAGS = -L. -l$(LIB)

all: options lib

.PHONY: options
options:
	@echo c-tap-parser build options:
	@echo "CFLAGS = ${CFLAGS}"
	@echo "CC     = ${CC}"

.PHONY: lib
lib:
	@echo CC -c ${SRC}
	@${CC} ${CFLAGS} ${lib_CFLAGS} -c ${SRC}
	@echo CC -o ${LIB_NAME}
	@${CC} -o ${LIB_NAME} ${OBJ} ${lib_LDFLAGS}

.PHONY: link_testprog
link_testprog: lib
	@echo CC -c ${test_SRC}
	@${CC} ${CFLAGS} -c ${test_SRC}
	@echo CC -o test
	@${CC} -o test ${test_OBJ} ${test_LDFLAGS}


.PHONY: testprog
testprog: ${test_OBJ} ${OBJ}
	@echo CC -o test
	@${CC} -o test ${test_OBJ} ${OBJ}


.PHONY: clean
clean:
	@rm -f test ${LIB_NAME}
	@rm -f $(OBJ) $(test_OBJ)
