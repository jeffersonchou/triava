GCC = gcc

INCLUDE += -I./ -I../

CFLAGS += -DSVN_VER -DCOMPILE_DATE -Wall

LDFLAGS += -L./ \
					-L/usr/local/lib \
					-lcurl \
					-lm \
					-lncurses \

SRC = proxylist.c proxyqueue.c proxycurlwrapper.c proxyavprocess.c proxyfiledownload.c proxyinterface.c

LIBS = 

TARGET = libproxy.so

all:$(TARGET)

$(TARGET):
	$(GCC) -shared -fPIC -g -o $(TARGET) $(SRC) $(INCLUDE) $(CFLAGS) $(LIBS) $(LDFLAGS)

clean:
	rm -rf $(TARGET)
