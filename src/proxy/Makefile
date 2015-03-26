CC=arm-linux-androideabi-gcc

INCLUDE += -I./ -I../ -I../../include/

#CFLAGS += -DSVN_VER -DCOMPILE_DATE -Wall
CFLAGS += -pipe -g  -ggdb -Wshadow  -Wconversion   -pthread -Wall

LDFLAGS += -L./ \
					-lcurl \
					-lm \

SRC = proxylist.c proxyqueue.c proxycurlwrapper.c proxyavprocess.c proxyfiledownload.c proxyinterface.c

LIBS = 

TARGET = libproxy.so

all:$(TARGET)

$(TARGET):
	$(CC) -shared -fPIC -g -o $(TARGET) $(SRC) $(INCLUDE) $(CFLAGS) $(LIBS) $(LDFLAGS)

clean:
	rm -rf $(TARGET)
