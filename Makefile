program: program.c program-utils.c
	gcc -o program program.c program-utils.c -lm
clean:
	rm -f program program.o program-utils.o