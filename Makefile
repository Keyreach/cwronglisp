all: main.exe

main.exe: vector.c vector.h main.c
	gcc -o main.exe vector.c main.c
