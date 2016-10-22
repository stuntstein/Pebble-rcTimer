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

src/timer.c

*/

#include <pebble.h>
#include <utils/pebble-assist.h>
#include "timer.h"
#include "icons.h"
//#include "settings.h"
//#include "windows/win-vibrate.h"

#define TIMER_RESOLUTION 10  // 0.01 sec resolution 1sec * 100

typedef enum {
  TIMER_TYPE_STOPWATCH = 0,
  TIMER_TYPE_TIMER = 1,
} TimerType;

typedef struct _Timer {
  TimerType       type;
  AppTimer*       timer;
  uint32_t        length;     // if 0 the it is a stopwatch
  uint32_t        current_time;
  TimerStatus     status;
  TimerVibration  expired_vibration;
  uint32_t        vib_interval;
  uint32_t        before_expired_length;
  TimerCallback_t update_cb;
  TimerCallback_t expired_cb;
// helper var
  uint32_t        vib_interval_counter;   // count down to vib
  bool            vib_pre_done_time_reached;
} sTimer;


static void timer_tick(void* context);
static void timer_finish(sTimer* timer);
static void timer_schedule_tick(sTimer* timer);
static void timer_cancel_tick(sTimer* timer);
static void timer_completed_action(sTimer* timer);
static void timer_callback_update(sTimer* timer);
static void timer_callback_done(sTimer* timer);


static void timer_tick(void* context)
{
  DEBUG("%s\n",__func__);
  sTimer* timer = (sTimer*)context;
  timer->timer = NULL;
  switch (timer->type) {
    case TIMER_TYPE_STOPWATCH:
      timer->current_time += 1;
      break;
    case TIMER_TYPE_TIMER:
      timer->current_time -= 1;
      if (timer->current_time <= 0)
      {
        timer_finish(timer);
        return;
      }
  }

  timer_schedule_tick(timer);
  timer_callback_update(timer);

  // vibration interval
  if ( timer->current_time % timer->vib_interval == 0)
  {
    vibes_short_pulse();
    DEBUG("VIB");
  }


  // EOR warning
  if (timer->type == TIMER_TYPE_TIMER)
  {
    if (timer->current_time <= timer->before_expired_length)
    {
      // vibrate every 1sec
      if ( timer->current_time % TIMER_RESOLUTION  == 0)
      {
        vibes_short_pulse();
        DEBUG("VIB");
      }
    }
  }
}


static void timer_finish(sTimer* timer) {
  DEBUG("%s\n",__func__);
  timer->status = TIMER_STATUS_DONE;
  timer_cancel_tick(timer);
  timer_completed_action(timer);
  timer_callback_update(timer);
  timer_callback_done(timer);
}


static void timer_schedule_tick(sTimer* timer) {
  DEBUG("%s\n",__func__);
  timer->timer = app_timer_register(100, timer_tick, (void*)timer);  // 0.1 timer
}

static void timer_cancel_tick(sTimer* timer) {
  DEBUG("%s\n",__func__);
  if (timer->timer) {
    app_timer_cancel(timer->timer);
    timer->timer = NULL;
  }
}

static void timer_completed_action(sTimer* timer) {
  DEBUG("%s\n",__func__);
  switch (timer->expired_vibration) {
    case TIMER_VIBE_NONE:
      // Do nothing!
      break;
    case TIMER_VIBE_SHORT:
      vibes_short_pulse();
      break;
    case TIMER_VIBE_LONG:
      vibes_long_pulse();
      break;
    case TIMER_VIBE_DOUBLE: {
      const uint32_t seg[] = { 600, 200, 600 };
      VibePattern pattern = {
        .durations =  seg,
        .num_segments = ARRAY_LENGTH(seg)
      };
      vibes_enqueue_custom_pattern(pattern);
      break;
    }
    case TIMER_VIBE_TRIPLE: {
      const uint32_t seg[] = { 600, 200, 600, 200, 600 };
      VibePattern pattern = {
        .durations =  seg,
        .num_segments = ARRAY_LENGTH(seg)
      };
      vibes_enqueue_custom_pattern(pattern);
      break;
    }
    default:
      break;
  }
}


static void timer_callback_update(sTimer* timer)
{
  DEBUG("%s %X\n",__func__, (unsigned int)timer->update_cb.handler);
  if (timer->update_cb.handler != NULL )
  {
    timer->update_cb.handler(timer->update_cb.context);
  }
}

static void timer_callback_done(sTimer* timer)
{
  DEBUG("%s\n",__func__);
  if (timer->expired_cb.handler != NULL )
  {
    timer->expired_cb.handler(timer->expired_cb.context);
  }
}


/******************************************************************************
  Timer creator
******************************************************************************/
Timer timer_create(void) {
  DEBUG("%s\n",__func__);
  sTimer* t = malloc(sizeof(sTimer));
  memset((void*)t,sizeof(sTimer),0);
  return (Timer)t;
}

void timer_destroy(Timer timer)
{
  if(timer != NULL)
  {
    DEBUG("%s\n",__func__);

    sTimer *t = (sTimer*)timer;
    if (t->timer) {
      app_timer_cancel(t->timer);
    }
    free(timer);
    timer = NULL;
  }
}

/******************************************************************************
  Timer action
******************************************************************************/
void timer_start(Timer timer)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer *t = (sTimer*)timer;
  timer_cancel_tick(t);
  t->status = TIMER_STATUS_RUNNING;
  timer_schedule_tick(t);
}


void timer_pause(Timer timer)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer *t = (sTimer*)timer;
  timer_cancel_tick(t);
  t->status = TIMER_STATUS_PAUSED;
}

void timer_resume(Timer timer)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer *t = (sTimer*)timer;
  if(t->current_time == 0)
  {
    t->status = TIMER_STATUS_STOPPED;
    timer_cancel_tick(t);
  }
  else
  {
    t->status = TIMER_STATUS_RUNNING;
    timer_schedule_tick(t);
  }
}

void timer_stop(Timer timer)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer* t = (sTimer*)timer;
  timer_cancel_tick(t);
  t->status = TIMER_STATUS_STOPPED;
}

void timer_reset(Timer timer)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  timer_stop(timer);
  memset(timer,0,sizeof(sTimer));
  return;
}

/******************************************************************************
 Set Timer length
******************************************************************************/
void timer_set_length(Timer timer, uint32_t length)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer* t = (sTimer*)timer;
  if(t->status == TIMER_STATUS_STOPPED)
  {
    t->length = t->current_time = length * TIMER_RESOLUTION; // counter at 10 ms resolution
    if(t->current_time == 0)
    {
      t->type = TIMER_TYPE_STOPWATCH;
    }
    else
    {
      t->type = TIMER_TYPE_TIMER;
    }
  }
}

/******************************************************************************
  Get status
******************************************************************************/
TimerStatus timer_get_status(Timer timer)
{
  if(timer==NULL)
    return TIMER_STATUS_STOPPED;

  DEBUG("%s\n",__func__);
  return ((sTimer*)timer)->status;
}

/******************************************************************************
  Get current timer
******************************************************************************/
uint32_t timer_get_time(Timer timer)
{
  if(timer==NULL)
    return 0;

  DEBUG("%s\n",__func__);
  return ((sTimer*)timer)->current_time;
}
/******************************************************************************
  Set timer expire warning length
******************************************************************************/
void timer_set_before_expire_warning_length(Timer timer, uint32_t length)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer* t = (sTimer*)timer;
  if(t->status == TIMER_STATUS_STOPPED)
  {
    t->before_expired_length = length * TIMER_RESOLUTION; // Counter at 10 ms resolution
  }
}

/******************************************************************************
  Set timer vibration
******************************************************************************/
void timer_set_interval_vibration(Timer timer, uint32_t interval)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer* t = (sTimer*)timer;
  if(t->status == TIMER_STATUS_STOPPED)
  {
    t->vib_interval = t->vib_interval_counter = interval * TIMER_RESOLUTION; // counter at 10 ms resolution
  }
}

void timer_set_expired_vibration(Timer timer, TimerVibration vib)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer* t = (sTimer*)timer;
  if(t->status == TIMER_STATUS_STOPPED)
  {
    t->expired_vibration = vib;
  }
}

/******************************************************************************
  register callbacks
******************************************************************************/
void timer_register_expired_cb(Timer timer, TimerCbHandler callback, void *context)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer* t = (sTimer*)timer;
  if(t->status == TIMER_STATUS_STOPPED)
  {
    t->expired_cb.handler = callback;
    t->expired_cb.context = context;
  }
}

void timer_register_update_cb(Timer timer, TimerCbHandler callback, void *context)
{
  if(timer==NULL)
    return;

  DEBUG("%s\n",__func__);

  sTimer* t = (sTimer*)timer;
  if(t->status == TIMER_STATUS_STOPPED)
  {
    t->update_cb.handler = callback;
    t->update_cb.context = context;
  }
}


/******************************************************************************
  String help functions
******************************************************************************/
char* timer_vibe_str(TimerVibration vibe, bool shortStr) {
  switch (vibe) {
    case TIMER_VIBE_NONE:
      return "None";
    case TIMER_VIBE_SHORT:
      return shortStr ? "Short" : "Short Pulse";
    case TIMER_VIBE_LONG:
      return shortStr ? "Long" : "Long Pulse";
    case TIMER_VIBE_DOUBLE:
      return shortStr ? "Double" : "Double Pulse";
    case TIMER_VIBE_TRIPLE:
      return shortStr ? "Triple" : "Triple Pulse";
//    case TIMER_VIBE_SOLID:
//      return shortStr ? "Solid" : "Continous";
    case TIMER_VIBE_MAX:
      return "";
  }
  return "";
}

void timer_time_str(uint32_t timer_time, char* str, int str_len) {

  int seconds = timer_time % 60;
  int minutes = timer_time / 60;

  snprintf(str, str_len, "%02d:%02d", minutes, seconds);
}

void timer_time_str_ms(uint32_t timer_time, bool ShowMinutes, char* str, int str_len) {

  int millisec = timer_time % TIMER_RESOLUTION;
  int seconds = (timer_time / TIMER_RESOLUTION) % 60;
  int minutes = timer_time / (60 * TIMER_RESOLUTION);

  if (ShowMinutes)
    snprintf(str, str_len, "%2d:%02d.%01d", minutes, seconds, millisec);
  else
    snprintf(str, str_len, "%2d.%01d", seconds, millisec);
}


