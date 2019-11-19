TARGETS=webServer format_string

CFLAGS=-Wall -g -O0

all: $(TARGETS)

webServer: webServer.c
	gcc $(CFLAGS) -o webServer webServer.c -lpthread

format_string: format_string.c
	gcc $(CFLAGS) -o format_string format_string.c -lpthread

clean:
	rm -f $(TARGETS)

