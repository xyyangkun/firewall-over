CROSS=
#arm-hisiv200-linux-
#arm-linux-
CC=$(CROSS)gcc
CXX=$(CROSS)g++
LINK=$(CROSS)g++ -o
LIBRARY_LINK=$(CROSS)ar cr

OUT=./OUT
SRCDIR=./src

LOG4C=$(PWD)/extern_lib/log4c-1.2.4
#头文件
INCLUDE= -I ./include -I$(LOG4C)/src/


#库文件
LIBDIR= -L./lib -L$(LOG4C)/src/log4c/.libs/
LIBS = -ludt  -lpthread -llog4c
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
CPPOBJS=$(OUT)/wrap.o $(OUT)/cJSON.o $(OUT)/Checkclientalive.o $(OUT)/crc.o
wrap.c:wrap.h
cJSON.c:cJSON.h
Checkclientalive.cpp:Checkclientalive.h
crc.cpp:crc.h



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
	
ex:
	export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(PWD)/extern_lib/log4c-1.2.4/src/log4c/.libs/

