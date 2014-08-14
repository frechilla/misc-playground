CC:=g++
CFLAGS:= -I.. -g -O0 -Wall -std=c++11 -DDEBUG -D_REENTRANT 
LDFLAGS:=
LIBS:=

SOURCES = $(wildcard *.cpp)
OBJS := $(SOURCES:.cpp=.o)
BINARIES := $(patsubst %.cpp,%,$(SOURCES))

all: $(patsubst %.cpp,%,$(SOURCES))

$(BINARIES): %: %.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

$(OBJS): %.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

force:
	$(MAKE) clean_all
	$(MAKE)

clean:
	rm -f $(OBJS)

clean_all:
	rm -f $(OBJS); rm -f $(BINARIES)

# Tell make that "all" etc. are phony targets, i.e. they should not be confused
# with files of the same names.
.PHONY: all clean clean_all force
