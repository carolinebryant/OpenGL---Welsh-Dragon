CC=clang
# Take out -g and add -O2 when not debuging
CFLAGS= -g -Wall -I/usr/include -I/usr/X11R6/include
LFLAGS=-L/usr/X11R6/lib -L/usr/lib64 -lX11 -lGL -lGLU -lglut -lm -lXmu -lXi

ARCHIVE_NAME = project2.tgz
ARCHIVE_COMMAND = tar -cvzf
UNARCHIVE_COMMAND = tar -xvzf
TEMP_FOLDER = /tmp/make_test_build/

ARCHIVE_FILES = main.c main.vert main.frag Makefile Readme.md WoodFloor-raw.ppm screenshot.png

OBJS =
MAIN_OBJ = main.o


#Store the latest git commit in $COMMIT
COMMIT = $(shell git log -n 1 --pretty="%h")

all: main
#Build types
#Our library files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $?
#Our test files
%.out: %.c
	$(CC) $(CFLAGS) -o $@ $? $(OBJS) -lm

.PHONY: test clean rebuild handin full_test

main: $(OBJS) $(MAIN_OBJ)
	$(CC) $(LFLAGS) -o main -g $(MAIN_OBJ) $(OBJS)

archive: $(ARCHIVE_FILES)
	@echo ==Archiving project files==
	@mkdir -p $(TEMP_FOLDER);
	$(ARCHIVE_COMMAND) $(TEMP_FOLDER)/$(ARCHIVE_NAME) $^
	@echo ==Testing submission compilation==
	@bash ./testHandin.sh "$(TEMP_FOLDER)" "$(UNARCHIVE_COMMAND)" "$(ARCHIVE_NAME)"

clean:
	@rm -f *.o
	@rm -f main
	@rm -f *.log

rebuild: clean main
