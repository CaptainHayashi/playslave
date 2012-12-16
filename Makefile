CC := clang
CFLAGS := -Wall -Wextra -Werror --std=c99 `pkg-config --cflags libavformat-011 libavcodec-011 ao`
LIBS := `pkg-config --libs libavformat-011 libavcodec-011 ao`

OBJS := main.o player.o io.o

main: $(OBJS) 
	$(CC) -o main $(OBJS) $(LIBS)

main.o: main.c
	$(CC) -c -o main.o main.c $(CFLAGS)

player.o: player.c
	$(CC) -c -o player.o player.c $(CFLAGS)

io.o: io.c
	$(CC) -c -o io.o io.c $(CFLAGS)
