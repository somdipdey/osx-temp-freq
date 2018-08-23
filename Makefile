
CC     = gcc
CFLAGS = -O2 -Wall
INC    = -framework IOKit
PREFIX = /usr/local
EXEC   = osx-temp-freq

build : $(EXEC)

clean :
	rm $(EXEC)

$(EXEC) : smc.c
	$(CC) $(CFLAGS) $(INC) -o $@ $?

install : $(EXEC)
	install -v $(EXEC) $(PREFIX)/bin/$(EXEC)
