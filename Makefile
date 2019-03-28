all: main.exe

main.exe: astlist.c main.c
	gcc -o main.exe astlist.c main.c
