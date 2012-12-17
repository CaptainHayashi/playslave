#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


const char     *ERRORS[] = {
	"FileNotFound",
	"InvalidStateChange",
	"InvalidFormat",
	"NeedOutputDevice",
};

void 
debug(int level, const char *format,...)
{
	va_list		ap;
	va_start(ap, format);

	level = (int)level;

	fprintf(stderr, "DEBUG ");
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");

	va_end(ap);
}

void 
error(int code, const char *format,...)
{
	va_list		ap;
	va_start(ap, format);

	fprintf(stderr, "ERROR %s ", ERRORS[code]);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");

	va_end(ap);
}

int 
input_waiting(void)
{
	fd_set		rfds;
	struct timeval	tv;

	/* Watch stdin (fd 0) to see when it has input. */
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	/* Stop checking immediately. */
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	return select(1, &rfds, NULL, NULL, &tv);
}
