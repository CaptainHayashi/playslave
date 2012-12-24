.POSIX:

# Programs
CC=		clang
TOUCH=		touch
RM=		rm

# Target
PROG=		main
WARNS?=		-Wall -Wextra -Werror

PKGS=		libavformat1 libavcodec1 portaudio-2.0
CFLAGS+=	-g --std=c99 `pkg-config --cflags $(PKGS)`
LIBS=		`pkg-config --libs $(PKGS)`

OBJS=		 main.o player.o io.o cmd.o audio.o audio_av.o

$(PROG): $(OBJS) 
	@echo "LINK	$@"
	@$(CC) -o $@ $(OBJS) $(LIBS)

.c.o:
	@echo "CC	$@"
	@$(CC) -c -o $@ $< $(WARNS) $(CFLAGS) 

clean: FORCE
	@$(TOUCH) $(PROG) $(OBJS)
	@$(RM) $(PROG) $(OBJS)

FORCE:
