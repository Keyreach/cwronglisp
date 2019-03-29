CC=gcc
EXECUTABLE=main.exe

all: main.exe

$(EXECUTABLE): vector.c main.c
	$(CC) -o $@ $^

test: $(EXECUTABLE)
	./$(EXECUTABLE) < test.wl
