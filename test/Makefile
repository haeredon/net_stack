test: main.o pcapng.o
	gcc -o test -g  main.o pcapng.o 


main.o : main.c
	gcc -g -c main.c  

pcapng.o : pcapng.c pcapng.h
	gcc -g -c pcapng.c -g 




#build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
#	$(CC) $(CFLAGS) $(SRCS-y) $(INCLUDES)  -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)
