//===================================================================
// Timer library
// measuring the elapsed CPU time for C programs
// coded by Takuya Kida
//
// 2018/10/24 ver 1.0
/*
Usage:
 1. Include this into your own C program as a header file.
      include "timer.h"
 2. Declare timer variable at the head of a function you want to measure.
      timer running_time;
 3. Call start_timer() with the pointer of the timer variable where you want to start measurement.
      start_timer(&running_time);
 4. Call stop_timer() with the pointer of the timer variable where you want to stop measurement.
      stop_timer(&running_time);
 5. Call show_timer() with the pointer of the timer variable where you want to show the result.
      show_timer(&running_time);
      stop_timer() must be called before calling this function.

Note:
 start_timer() can be called after calling stop_timer().
 In that case, the timer restarts and the measurement time is summed.
 If you want to reset the timer, please call reset_timer() with the pointer of the timer variable.
*/

#pragma once

#include <stdio.h>
#include <time.h>

typedef enum {
  Init = 0,
  Run,
  Stop
} timer_status;

typedef struct _TIMER {
  clock_t start;
  clock_t end;
  timer_status status;
} timer;

static void start_timer(timer *timer)
{
  switch (timer->status) {
  case Init:
	timer->start = clock();
	timer->status = Run;
	break;
  case Run:
	fprintf(stderr, "timer error: timer is already running");
	break;
  case Stop:
	timer->status = Run;
	break;
  }
};
static void stop_timer(timer *timer) {
  if (timer->status == Run) {
	timer->end = clock();
	timer->status = Stop;
  }
};
static void reset_timer(timer *timer) {
  timer->status = Init;
}
static void show_timer(timer *timer, const char *title) {
  double sum;

  switch (timer->status) {
  case Init:
	fprintf(stderr, "timer error: timer didn't start");
	break;
  case Run:
	fprintf(stderr, "timer error: timer is still running");
	break;
  case Stop:
	sum = ((double)(timer->end - timer->start)/CLOCKS_PER_SEC);
	printf("%s = %3.4f sec\n", title, sum);
	break;
  }
};
