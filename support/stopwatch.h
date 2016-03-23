#ifndef CARYLL_SUPPORT_STOPWATCH_H
#define CARYLL_SUPPORT_STOPWATCH_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
// Windows
#include <windows.h>
#elif __MACH__
// OSX
#include <unistd.h>
#include <mach/mach_time.h>
#else
#include <unistd.h>
#endif
void time_now(struct timespec *tv);
void push_stopwatch(const char *reason, struct timespec *sofar);

#endif
