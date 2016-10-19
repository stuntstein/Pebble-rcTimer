/*

Multi Timer v3.4
http://matthewtole.com/pebble/multi-timer/

----------------------

The MIT License (MIT)
Copyright © 2013 - 2015 Matthew Tole
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

--------------------

src/timer.h

*/

#pragma once

#include <pebble.h>

typedef enum {
  TIMER_VIBE_NONE,
  TIMER_VIBE_SHORT,
  TIMER_VIBE_LONG,
  TIMER_VIBE_DOUBLE,
  TIMER_VIBE_TRIPLE,
  TIMER_VIBE_MAX,
} TimerVibration;

typedef enum {
  TIMER_STATUS_STOPPED = 0,
  TIMER_STATUS_RUNNING = 1,
  TIMER_STATUS_PAUSED = 2,
  TIMER_STATUS_DONE = 3,
} TimerStatus;

typedef void (*TimerCbHandler)(void* context);

typedef struct TimerCallback_t
{
  TimerCbHandler handler;
  void* context;
}TimerCallback_t;

typedef void* (Timer);

#define TIMER_REPEAT_INFINITE 100


// Timer creator
Timer timer_create(void);
void timer_destroy(Timer timer);

// Timer action
void timer_start(Timer timer);
void timer_pause(Timer timer);
void timer_resume(Timer timer);
void timer_reset(Timer timer);

// Get timer info
TimerStatus timer_get_status(Timer timer);
uint32_t    timer_get_time(Timer timer);
uint32_t    timer_get_resolution(Timer timer);

// Set Timer length
void timer_set_length(Timer timer, uint32_t length);

//Set timer expire warning length
void timer_set_before_expire_warning_length(Timer timer, uint32_t length);

// Set timer vibration
void timer_set_interval_vibration(Timer timer, uint32_t interval);
void timer_set_expired_vibration(Timer timer, TimerVibration vib);

// register callbacks
void timer_register_expired_cb(Timer timer, TimerCbHandler callback, void *context);
void timer_register_update_cb(Timer timer, TimerCbHandler callback, void *context);

// String help functions
char* timer_vibe_str(TimerVibration vibe, bool shortStr);
void timer_time_str(uint32_t timer_time, char* str, int str_len);
void timer_time_str_ms(uint32_t timer_time, bool ShowMinutes, char* str, int str_len);
