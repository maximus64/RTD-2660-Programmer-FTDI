CPPFLAGS=-g -Wall
CFLAGS=-g -Wall
LDFLAGS=-g
LDLIBS= -lusb-1.0

rtd_programer: crc.o gff.o i2c.o main.o i2c_ftdi.o mpsse.o
	g++ $(LDFLAGS) -o rtd_programer crc.o gff.o i2c.o i2c_ftdi.o mpsse.o main.o $(LDLIBS)

mpsse.o: mpsse.c mpsse.h
	gcc $(CFLAGS) -c mpsse.c

i2c_ftdi.o: i2c_ftdi.c i2c_ftdi.h
	gcc $(CFLAGS) -c i2c_ftdi.c

crc.o: crc.cpp crc.h
	g++ $(CPPFLAGS) -c crc.cpp

i2c.o: i2c.cpp i2c.h
	g++ $(CPPFLAGS) -c i2c.cpp

gff.o: gff.cpp gff.h
	g++ $(CPPFLAGS) -c gff.cpp

main.o: main.cpp
	g++ $(CPPFLAGS) -c main.cpp

clean:
	rm -rf rtd_programer
	rm -rf *.o