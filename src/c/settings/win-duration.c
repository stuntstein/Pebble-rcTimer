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

src/windows/win-duration.c

*/

#include <pebble.h>

#include <utils/pebble-assist.h>
#include <utils/bitmap-loader.h>

#include "../timer.h"
#include "../icons.h"

#include "win-duration.h"

#define MODE_MINUTES 0
#define MODE_SECONDS 1


static void window_load(Window* window);
static void window_unload(Window* window);
static void layer_update(Layer* me, GContext* ctx);
static void layer_action_bar_click_config_provider(void *context);
static void action_bar_layer_down_handler(ClickRecognizerRef recognizer, void *context);
static void action_bar_layer_up_handler(ClickRecognizerRef recognizer, void *context);
static void action_bar_layer_select_handler(ClickRecognizerRef recognizer, void *context);
static void update_timer_length(void);

static Window* s_window;
static Layer* s_layer;
static ActionBarLayer* s_action_bar;
static uint32_t s_duration = 0;
static DurationCallback s_callback;
static char *s_header;

static int s_mode = MODE_MINUTES;
static int s_minutes = 0;
static int s_seconds = 0;

static GFont s_font_duration;

HEAP_CHECK;

void win_duration_init(void) {
  HEAP_CHECK_START();
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  HEAP_CHECK_STOP();
}

void win_duration_deinit(void) {
  HEAP_CHECK_START();
  window_destroy(s_window);
  HEAP_CHECK_STOP();
}

void win_duration_show(uint16_t duration, DurationCallback callback, bool ShowMinutes, char *header) {
  s_duration = duration;
  s_callback = callback;
  window_stack_push(s_window, true);
  s_mode = ShowMinutes ? MODE_MINUTES : MODE_SECONDS;
  s_minutes = s_duration / 60;
  s_seconds = s_duration % 60;
  layer_mark_dirty(s_layer);
  s_header = header;
}

static void window_load(Window* window) {
  s_layer = layer_create_fullscreen(s_window);
  layer_set_update_proc(s_layer, layer_update);
  layer_add_to_window(s_layer, s_window);

  s_action_bar = action_bar_layer_create();
#if !defined(PBL_PLATFORM_APLITE)
  action_bar_layer_set_background_color(s_action_bar, GColorBlue);
#endif  
  action_bar_layer_add_to_window(s_action_bar, s_window);
  action_bar_layer_set_click_config_provider(s_action_bar, layer_action_bar_click_config_provider);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS,ICON_RECT_ACTION_INC));
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_UP_DOWN));
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_ACTION_DEC));

//  s_font_duration = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_AUDI_70_BOLD));
  s_font_duration = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
}

static void window_unload(Window* window) {
  s_callback(s_duration);
  action_bar_layer_destroy(s_action_bar);
  layer_destroy(s_layer);
//  fonts_unload_custom_font(s_font_duration);
}

static void layer_update(Layer* me, GContext* ctx) {

  char summary_str[32];
  GRect focus;
  GRect bounds = layer_get_bounds(me);
#if defined(PBL_PLATFORM_APLITE)
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_text_color(ctx, GColorBlack);
#else
  graphics_context_set_fill_color(ctx, GColorGreen);
  graphics_context_set_text_color(ctx, GColorBlack);
#endif
  graphics_context_set_stroke_color(ctx, GColorBlack);


  // Place focus
  switch (s_mode) {
    case MODE_MINUTES:
      focus = (GRect){
#if defined(PBL_PLATFORM_APLITE)
        .origin = { 44, 101 },
        .size = { 24, 2 }};
#else
  #if defined(PBL_ROUND)
        .origin = { 70, 77 },
  #else
        .origin = { 44, 77 },
  #endif
        .size = { 24,24 }};
#endif
    break;
    case MODE_SECONDS:
      focus = (GRect){
#if defined(PBL_PLATFORM_APLITE)
        .origin = { 71, 101 },
        .size = { 24, 2 }};
#else
  #if defined(PBL_ROUND)
        .origin = { 97, 77 },
  #else
        .origin = { 71, 77 },
  #endif
        .size = { 24, 24 }};
#endif
    break;
  }

// Header
  graphics_draw_text(ctx, s_header, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      (GRect){
#if defined(PBL_ROUND)
        .origin = { 10, 25 },
#else
        .origin = { 0, 15 },
#endif
        .size = { bounds.size.w - ACTION_BAR_WIDTH - 3, 40 }
      },
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);

// Draw focus rectangle
  graphics_fill_rect(ctx, focus, 0, GCornerNone);

  timer_time_str(s_duration, summary_str, 32);

  graphics_draw_text(ctx, summary_str, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), 
      (GRect){
        .origin = { 0, 70 },
        .size = { bounds.size.w - ACTION_BAR_WIDTH - 20, 40 }
      },
    GTextOverflowModeFill, GTextAlignmentRight, NULL);
  

}

static void layer_action_bar_click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, action_bar_layer_down_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, action_bar_layer_up_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, action_bar_layer_select_handler);
}

static void action_bar_layer_down_handler(ClickRecognizerRef recognizer, void *context) {
  switch (s_mode) {
    case MODE_MINUTES:
      s_minutes -= 1;
      if (s_minutes < 0) {
        s_minutes = 0;
      }
    break;
    case MODE_SECONDS:
      s_seconds -= 1;
      if (s_seconds < 0) {
        if (s_minutes > 0) {
          s_minutes -= 1;
          s_seconds = 59;
        }else
          s_seconds = 0;
      }
    break;
  }
  update_timer_length();
  layer_mark_dirty(s_layer);
}

static void action_bar_layer_up_handler(ClickRecognizerRef recognizer, void *context) {
  switch (s_mode) {
    case MODE_MINUTES:
      s_minutes += 1;
      if (s_minutes >= 100) {
        s_minutes = 99;
      }
    break;
    case MODE_SECONDS:
      s_seconds += 1;
      if (s_seconds >= 60) {
        s_seconds = 00;
        s_minutes += 1;
        if (s_minutes >= 100) {
          s_minutes = 99;
        }
      }
    break;
  }
  update_timer_length();
  layer_mark_dirty(s_layer);
}

static void action_bar_layer_select_handler(ClickRecognizerRef recognizer, void *context) {
  switch (s_mode) {
    case MODE_MINUTES:
      s_mode = MODE_SECONDS;
    break;
    case MODE_SECONDS:
      s_mode = MODE_MINUTES;
    break;
  }
  layer_mark_dirty(s_layer);
}

static void update_timer_length(void) {
  s_duration = s_minutes * 60 + s_seconds;
}
