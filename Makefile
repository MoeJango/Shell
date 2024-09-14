all: witsshell test

witsshell: witsshell.c
	gcc -o witsshell witsshell.c

test: test.c
	gcc -o test test.c

clean:
	rm -f witsshell