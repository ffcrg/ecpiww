#for cross compiling call "make -f <file> CROSS=1

INC= -I .   -I ../include

ifeq "$(CROSS)" "1"
    CC     = arm-linux-gnueabihf-gcc
    CPP    = arm-linux-gnueabihf-g++
    CFLAGS = -Wall
else
    CC     = gcc
    CPP    = g++
    CFLAGS = -Wall
endif

all:	 ecpiww 

		
ecpiww: 		ecpiww.o wmbus.o 
				$(CC) -o ecpiww ecpiww.o wmbus.o -lpthread 
				
ecpiww.o:		../src/energycam/ecpiww.c ../include/energycam/ecpiww.h 
				$(CC) $(CFLAGS) $(INC) -c ../src/energycam/ecpiww.c
							
wmbus.o:		../src/energycam/wmbus.c
				$(CC) $(CFLAGS) $(INC) -pthread -c ../src/energycam/wmbus.c

clean: 			
				@rm -f ecpiww ecpiww.o wmbus.o
				@echo Clean done
