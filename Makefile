
CC			= g++
CFLAGS		= -Wall -pedantic -fno-strict-aliasing

CRYSTAL     = crystal
CRYSTAL_L	= -lpthread -lrt -lgd -lpng -lz

CSECTION    = csection
CSECTION_L	= -lgd -lpng -lz

DATE		= `date +%Y-%m-%d-%H-%M`

#debug with gdb
ifdef DEBUG
	CFLAGS		+= -DUSE_DEBUG -g3 
else
	CFLAGS		+= -O3 -funroll-loops
endif

#profile with callgrind, works well with DEBUG mode
ifdef PROFILE
	CFLAGS		+= -pg
endif

#For profile directed optimizations. To use:
# 1. compile with PROFILE_GEN
# 2. run the program, generates .gcno and .gcda
# 3. compile with PROFILE_USE
ifdef PROFILE_GEN
	CFLAGS		+= -fprofile-generate
endif
ifdef PROFILE_USE
	CFLAGS		+= -fprofile-use
endif

all : $(CRYSTAL) $(CSECTION)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(CRYSTAL): $(CRYSTAL_O) $(CRYSTAL).cpp
	$(CC) $(LDFLAGS) $(CFLAGS) $(CRYSTAL_L) $(CRYSTAL_O) $(CRYSTAL).cpp -o $(CRYSTAL)

$(CSECTION): $(CSECTION_O) $(CSECTION).cpp
	$(CC) $(LDFLAGS) $(CFLAGS) $(CSECTION_L) $(CSECTION_O) $(CSECTION).cpp -o $(CSECTION)


clean:
	rm -f *.o $(CRYSTAL) $(CSECTION)
#	rm -f *~

fresh: clean all

run:
	./$(CRYSTAL)

profile:
	valgrind --tool=callgrind ./$(CRYSTAL)

video:
	ffmpeg -i $(DIR)/slope.%05d.png -b 20000k slope.mpg

