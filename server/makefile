C := gcc
CFLAGS := -Wall -Wextra
LDFLAGS := -lwiringPi -lpthread

all: main cam

main: server_main.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

cam: server_camera.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f main cam

