OPT = -O0 -g3
LIB_SOURCES1 = main.c crypto.c

LIB_SOURCES = $(addprefix src/, $(LIB_SOURCES1)) sqlite3/sqlite3.c
CC = gcc
TARGET = run
LIB_OBJECTS =$(LIB_SOURCES:.c=.o)
LDFLAGS= -L. $(OPT)
LIBS= libiron.a libcrypto.a -lpthread -ldl  
ALL= $(TARGET)
CFLAGS = -Isrc/ -I. -Iinclude/ -std=gnu11 -c $(OPT) -Werror=implicit-function-declaration -Wformat=0 -D_GNU_SOURCE -fdiagnostics-color  -Wwrite-strings -msse4.2 -Werror=maybe-uninitialized -DUSE_VALGRIND -DDEBUG -Wall

$(TARGET): $(LIB_OBJECTS) libiron.a
	$(CC) $(LDFLAGS) $(LIB_OBJECTS) $(LIBS) -o $(TARGET)

all: $(ALL)

.PHONY: iron/libiron.a

iron/libiron.a:
	make -C iron wasm

libiron.a: iron/libiron.a
	cp iron/libiron.a libiron.a
libiron.bc: libiron.a
	cp libiron.a libiron.bc

libcrypto.bc:
	./build_crypto_wasm.sh
libcrypto.a:
	./build_crypto_x64.sh
.c.o: $(HEADERS) $(LEVEL_CS)
	$(CC) $(CFLAGS) $< -o $@ -MMD -MF $@.depends

release: OPT = -O4 -g0
release: all

wasm_release: OPT = -flto=thin -O3 -g0
wasm_release: wasm
wasm: CC = emcc
wasm: CFLAGS = -c -Isrc/ -I. -Iinclude/ -Isubmodule_openssl/include
wasm: LDFLAGS= -s WASM=1 -s USE_GLFW=3 --gc-sections 
wasm: LIBS= libiron.bc libcrypto.bc -lpthread -ldl
wasm: TARGET = run.js
wasm: libiron.bc
wasm: libcrypto.bc
wasm: all


depend: h-depend
clean:
	rm -f $(LIB_OBJECTS) $(ALL) src/*.o.depends src/*.o
	rm libcrypto.bc libiron.bc
.PHONY: test
test: $(TARGET)
	make -f makefile.test test

-include $(LIB_OBJECTS:.o=.o.depends)


