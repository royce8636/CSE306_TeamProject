CC := gcc
CFLAGS := -Wall -Wextra
LDFLAGS := -lSDL2 -ljpeg

all: main cam

main: client_main.c
	$(CC) $(CFLAGS) $^ -o $@

cam: client_camera.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f main cam

