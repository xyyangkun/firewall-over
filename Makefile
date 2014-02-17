CROSS=
#arm-hisiv200-linux-
#arm-linux-
CC=$(CROSS)gcc
CXX=$(CROSS)g++
LINK=$(CROSS)g++ -o
LIBRARY_LINK=$(CROSS)ar cr

OUT=./OUT
SRCDIR=./src

#头文件
INCLUDE= -I ./include


#库文件
LIBDIR= -L./lib
LIBS = -ludt  -lpthread
LDLIBS=$(LIBDIR) $(LIBS)

#编译选项
CXXFLAGS=-g
CXXFLAGS+= $(INCLUDE) $(LDLIBS) 
CFLAGS=-g
CFLAGS+=   $(INCLUDE) $(LDLIBS)

#目标
all: server client

$(OUT)/%.o:$(SRCDIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)
$(OUT)/%.o:$(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

CPPOBJS = 
CPPOBJS=$(OUT)/wrap.o $(OUT)/cJSON.o
wrap.c:wrap.h
cJSON.c:cJSON.h



server:$(OUT)/server.o $(CPPOBJS)
	$(LINK) $@ $< $(CPPOBJS) $(CXXFLAGS) 	
$(OUT)/server.o: $(SRCDIR)/server.cpp
	$(CXX) -c $< -o $@ $(CFLAGS)


client:$(OUT)/client.o $(CPPOBJS)
	$(LINK) $@ $< $(CPPOBJS)  $(CXXFLAGS)
$(OUT)/client.o: $(SRCDIR)/client.cpp
	$(CXX) -c $< $(CPPOBJS) -o $@ $(CXXFLAGS)

clean:
	rm $(OUT)/*
	rm server client
