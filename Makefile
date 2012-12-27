.POSIX:

################################################################################
# Makefile for playslave

# Programs
CC?=		clang
TOUCH=		touch
RM=		rm

# Target
PROG=		playslave
WARNS?=		-Wall -Wextra -Werror -pedantic

# These are overridable because they will inevitably differ from
# system to system.  So much for portability...
AVFORMAT_PKG?=	libavformat1
AVCODEC_PKG?=	libavcodec1
PORTAUDIO_PKG?=	portaudio-2.0
PKGS=		$(PORTAUDIO_PKG) $(AVCODEC_PKG) $(AVFORMAT_PKG)

CFLAGS+=	-g --std=gnu99 `pkg-config --cflags $(PKGS)`
LIBS=		`pkg-config --libs $(PKGS)`

OBJS=		main.o
OBJS+=		cmd.o constants.o errors.o io.o messages.o player.o 
OBJS+=		audio.o audio_av.o audio_cb.o
OBJS+=		contrib/pa_ringbuffer.o

$(PROG): $(OBJS) 
	@echo "LD	$@"
	@$(CC) -o $@ $(OBJS) $(LIBS)

.c.o:
	@echo "CC	$@"
	@$(CC) -c -o $@ $< $(WARNS) $(CFLAGS) 

clean: FORCE
	@echo "CLEAN"
	@$(TOUCH) $(PROG) $(OBJS)
	@$(RM) $(PROG) $(OBJS)

FORCE:
