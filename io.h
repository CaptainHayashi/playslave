#ifndef IO_H
#define IO_H

void debug(int level, const char *format, ...);
void error(int code, const char *format, ...);
int input_waiting(void);

#endif // IO_H
