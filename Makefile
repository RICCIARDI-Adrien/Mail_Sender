CC = gcc
CCFLAGS = -W -Wall

BINARY = mail-sender

all:
	$(CC) $(CCFLAGS) Main.c -lcurl -o $(BINARY)

clean:
	rm -f $(BINARY)
