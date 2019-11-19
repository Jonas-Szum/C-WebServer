TARGETS=homework5 thread_example format_string

CFLAGS=-Wall -g -O0

all: $(TARGETS)

homework5: homework5.c
	gcc $(CFLAGS) -o homework5 homework5.c -lpthread

thread_example: thread_example.c
	gcc $(CFLAGS) -o thread_example thread_example.c -lpthread

format_string: format_string.c
	gcc $(CFLAGS) -o format_string format_string.c -lpthread

clean:
	rm -f $(TARGETS)

