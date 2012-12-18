#ifndef IO_H
#define IO_H

#include "errors.h"

void		debug     (int level, const char *format,...);
enum error	error(enum error code, const char *format,...);
int		input_waiting(void);

#endif				/* !IO_H */
