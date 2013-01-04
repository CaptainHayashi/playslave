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

# Usually we want to work on the c99 standard, but some targets hide
# some POSIX library functions unless gnu99 is set, so we let this be
# changed on the command line too.
STD?=		c99

CFLAGS+=	-g --std=$(STD) `pkg-config --cflags $(PKGS)`
LIBS=		`pkg-config --libs $(PKGS)`

# High-level system
OBJS=		main.o player.o 
# Constants
OBJS+=		constants.o messages.o 
# Audio system
OBJS+=		audio.o audio_av.o audio_cb.o
# Code from elsewhere
OBJS+=		cuppa/cmd.o cuppa/constants.o cuppa/errors.o cuppa/io.o
OBJS+=		cuppa/messages.o cuppa/utils.o
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
