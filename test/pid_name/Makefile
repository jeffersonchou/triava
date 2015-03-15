CC=${CROSS_TOOLS}gcc
AR=${CROSS_TOOLS}ar
STRIP=${CROSS_TOOLS}strip

CFLAGS = -DHAVE_CONFIG_H -fPIC

OBJS = ./pidof.o

TARGET = pidof

%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS)

all:  $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)


clean:
	rm -f *.o $(TARGET) 

