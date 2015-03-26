CC=${CROSS_TOOLS}gcc
AR=${CROSS_TOOLS}ar
STRIP=${CROSS_TOOLS}strip

CFLAGS = -I../include -I./proxy
CFLAGS += -pipe -g  -ggdb -Wshadow  -Wconversion   -pthread -Wall
LIBS = -lz -pthread -lc -lnetfilter -lssl -lcrypto -L./proxy -lproxy

OBJS = actions.o \
	cgi.o \
	cgiedit.o \
	cgisimple.o \
	deanimate.o \
	encode.o \
	errlog.o \
	filters.o \
	gateway.o \
	jbsockets.o \
	jcc.o \
	list.o \
	loadcfg.o \
	loaders.o \
	miscutil.o \
	parsers.o \
	pcrs.o \
	ssplit.o \
	urlmatch.o

TARGET = Triava

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

all:  $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

install:
	cp $(TARGET) ${INSTALL_DIR}/
	mkdir -p ${INSTALL_DIR}/
	cp etc ${INSTALL_DIR}/ -rf
	cp TriavaLoad.sh ${INSTALL_DIR}/ -f

clean:
	rm -f *.o $(TARGET) *.d

