OPT = -O0 -g3
LIB_SOURCES1 = main.c 

LIB_SOURCES = $(addprefix src/, $(LIB_SOURCES1)) sqlite3/sqlite3.c
CC = gcc
TARGET = run
LIB_OBJECTS =$(LIB_SOURCES:.c=.o)
LDFLAGS= -L. $(OPT) # -Wl,--gc-sections -Wl,-s -fdata-sections -ffunction-sections
LIBS= libiron.a -lpthread -ldl -lGL -lGLEW -lm  -lopenal -licydb  -lX11
ALL= $(TARGET)
CFLAGS = -Isrc/ -I. -Iinclude/ -std=gnu11 -c $(OPT) -Werror=implicit-function-declaration -Wformat=0 -D_GNU_SOURCE -fdiagnostics-color  -Wwrite-strings -msse4.2 -Werror=maybe-uninitialized -DUSE_VALGRIND -DDEBUG -Wall

$(TARGET): $(LIB_OBJECTS) libiron.a
	$(CC) $(LDFLAGS) $(LIB_OBJECTS) $(LIBS) -o $@ 

all: $(ALL)

.PHONY: iron/libiron.a

iron/libiron.a:
	make -C iron

libiron.a: iron/libiron.a
	cp iron/libiron.a libiron.a

.c.o: $(HEADERS) $(LEVEL_CS)
	$(CC) $(CFLAGS) $< -o $@ -MMD -MF $@.depends

src/ttf_font.c: 
	xxd -i /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf > src/ttf_font.c
main.o: ttf_font.c

depend: h-depend
clean:
	rm -f $(LIB_OBJECTS) $(ALL) src/*.o.depends src/*.o 
.PHONY: test
test: $(TARGET)
	make -f makefile.test test

-include $(LIB_OBJECTS:.o=.o.depends)


