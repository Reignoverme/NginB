LIB_SRC = \
	main.cpp     \
	Acceptor.cpp \
	Channel.cpp  \
	EventLoop.cpp\
	Epoller.cpp  \
	Connection.cpp \
	Buffer.cpp \
	HTTPRequestHandler.cpp \
	Request.h

TEST_SRC = $(notdir $(LIB_SRC))
OBJS = $(patsubst %.cpp, %.o,$(TEST_SRC))

CXXFLAGS = -std=c++11 -Wall
LDFLAGS = -lboost_system

BINARIES = test

all: $(BINARIES)

$(BINARIES): $(LIB_SRC) 
	g++ $(CXXFLAGS) -o $@ $(filter %.cpp,$^) $(LDFLAGS)

clean:
	rm -f $(BINARIES) *.o *.a core

