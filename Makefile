WARNINGS = -Wpedantic -pedantic-errors
WARNINGS += -Werror
WARNINGS += -Wall
WARNINGS += -Wextra
WARNINGS += -Wold-style-definition
WARNINGS += -Wcast-align
WARNINGS += -Wformat=2
WARNINGS += -Wlogical-op
WARNINGS += -Wmissing-declarations
WARNINGS += -Wmissing-include-dirs
WARNINGS += -Wmissing-prototypes
WARNINGS += -Wnested-externs
WARNINGS += -Wpointer-arith
WARNINGS += -Wredundant-decls
WARNINGS += -Wsequence-point
WARNINGS += -Wshadow
WARNINGS += -Wstrict-prototypes
WARNINGS += -Wswitch
WARNINGS += -Wundef
WARNINGS += -Wunreachable-code
WARNINGS += -Wunused-but-set-parameter
WARNINGS += -Wwrite-strings
WARNINGS += -Wdisabled-optimization
WARNINGS += -Wunsafe-loop-optimizations
WARNINGS += -Wfree-nonheap-object


CC = gcc
CFLAGS = -std=c89 -D_GNU_SOURCE
SOURCES = *.c
EXEC = txt2web


all: build

build:
	$(CC) -g $(SOURCES) $(CFLAGS) $(WARNINGS) -o $(EXEC)
	#$(CC) -g $(SOURCES) $(CFLAGS) -o $(EXEC)

run:
	./$(EXEC)
install: all
	cp -f $(EXEC) /usr/bin
	chmod 755 /usr/bin/$(EXEC)

debug:
	gdb -x gdbinit $(EXEC)

clean:
	rm $(EXEC)
