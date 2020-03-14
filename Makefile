PROG = feedback_parsort

all : $(PROG)

CC        =  gcc
CFLAGS    =  -g -Wall -O2 -fopenmp
CPPFLAGS  =  -DDEBUG
LDFLAGS   =  -g -lm -O2 -fopenmp
SRC_FOLDER = .
TARGET_FOLDER = bin

make_bin_folder:
	mkdir -p bin > /dev/null

feedback_parsort: main.c
	$(CC) -o $@ $^ $(CFLAGS)

clean :
	@rm -f *.o $(PROG) *.pgm
	@rm -rf $(TARGET_FOLDER)
