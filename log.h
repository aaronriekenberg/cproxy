#ifndef LOG_H
#define LOG_H

extern void proxyLogSetThreadName(const char* threadName);

extern void proxyLog(const char* format, ...);

#endif
