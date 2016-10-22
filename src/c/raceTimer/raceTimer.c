#include <pebble.h>
#include <utils/pebble-assist.h>
#include <utils/bitmap-loader.h>
#include "../rcTimer.h"
#include "../settings/settings.h"
#include "../timer.h"
#include "../icons.h"

#include "../layers/progress_layer.h"

typedef enum
{
  ICONS_STOPPED,
  ICONS_RUNNING,
  ICONS_PAUSED,
}racetimer_icons;


#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define EVENTS(x)         \
  x(EVENT_INIT)           \
  x(EVENT_TIMER_EXPIRED)  \
  x(EVENT_CLICK_UP)       \
  x(EVENT_CLICK_DOWN)     \
  x(EVENT_SETTINGS)

typedef enum
{
  EVENTS(GENERATE_ENUM)
}racetimer_event;

static const char *EVENTS_STRING[] = {
    EVENTS(GENERATE_STRING)
};

#define STATES(x)           \
  x(STATE_STOPPED)          \
  x(STATE_PAUSED)           \
  x(STATE_PRE_RACE_RUNNING) \
  x(STATE_RACE_RUNNING)     \
  x(STATE_AFTER_RACE_RUNNING)

typedef enum
{
  STATES(GENERATE_ENUM)
}racetimer_state;

static const char *STATES_STRING[] = {
    STATES(GENERATE_STRING)
};


/*
typedef enum
{
  EVENT_INIT,
  EVENT_TIMER_EXPIRED,
  EVENT_CLICK_UP,
  EVENT_CLICK_DOWN,
  EVENT_SETTINGS,
}racetimer_event;

typedef enum
{
  STATE_STOPPED,
  STATE_PAUSED,
  STATE_PRE_RACE_RUNNING,
  STATE_RACE_RUNNING,
  STATE_AFTER_RACE_RUNNING,
}racetimer_state;
*/
#define str(x) #x

#define STATUS_BAR_HEIGHT 16


// platform colors
#if defined(PBL_PLATFORM_APLITE)
#define PROGRESS_FG_COLOR GColorWhite
#define PROGRESS_FG_COLOR_PRETIMER GColorWhite
#define ACTION_BAR_COLOR GColorBlack
#else
#define PROGRESS_FG_COLOR GColorGreen
#define PROGRESS_FG_COLOR_PRETIMER GColorRed
#define ACTION_BAR_COLOR GColorBlue
#endif


static Window *window;
static TextLayer *title_layer, *pretimer_layer, *timer_layer;
static Timer rctimer;
static char pretime_str[10], time_str[10];

ActionBarLayer *action_bar;
static GBitmap *s_icon_start, *s_icon_stop, *s_icon_pause, *s_icon_settings, *s_icon_profile;
static ProgressLayer *s_progress_layer;
static uint16_t s_progress = 0;
static uint16_t s_progress_size = 0;

static StatusBarLayer *status_bar_layer;

static racetimer_state prev_state = STATE_STOPPED;
static racetimer_state state = STATE_STOPPED;

static void racetimer_event_handler(racetimer_event event);
static void racetimer_event_handler_with_clicks(racetimer_event event, uint8_t clicks);

HEAP_CHECK


void init_statusbar_text_layer(Layer *parent) {
  status_bar_layer = status_bar_layer_create();
  status_bar_layer_set_colors(status_bar_layer,ACTION_BAR_COLOR,GColorWhite);
  status_bar_layer_set_separator_mode(status_bar_layer,StatusBarLayerSeparatorModeDotted);
  layer_add_child(parent, status_bar_layer_get_layer(status_bar_layer));
}

void deinit_statusbar(void)
{
  status_bar_layer_destroy(status_bar_layer);
}

static void pre_race_update_cb(void* context) {
  DEBUG("%s\n",__func__);
  timer_time_str_ms(timer_get_time(rctimer), false, pretime_str,sizeof(pretime_str));
  s_progress = timer_get_time(rctimer);

  text_layer_set_text(pretimer_layer, pretime_str);
  progress_layer_set_progress(s_progress_layer, (s_progress*100)/s_progress_size);

  DEBUG("pretimer:%s %d",pretime_str,s_progress);
}

static void race_update_cb(void* context) {
  DEBUG("%s\n",__func__);

  timer_time_str_ms(timer_get_time(rctimer), true, time_str,sizeof(time_str));
  text_layer_set_text(timer_layer, time_str);

  s_progress = s_progress_size - timer_get_time(rctimer);

  progress_layer_set_progress(s_progress_layer, (s_progress*100)/s_progress_size);
  DEBUG("timer:%s %d",time_str,s_progress);
}

static void after_race_update_cb(void* context) {
  DEBUG("%s\n",__func__);
  timer_time_str_ms(timer_get_time(rctimer), true, time_str,sizeof(time_str));
  text_layer_set_text(timer_layer, time_str);

  DEBUG("timer:%s %d",time_str,s_progress);
}

static void timer_expired_cb(void* context) {
  DEBUG("%s\n",__func__);
  racetimer_event_handler(EVENT_TIMER_EXPIRED);
}


static void up_repeat_click_handler(ClickRecognizerRef recognizer, void *context)
{
  racetimer_event_handler_with_clicks(EVENT_CLICK_UP, (settings_get_active_profile() + 1));
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context)
{
  racetimer_event_handler_with_clicks(EVENT_CLICK_UP, (click_number_of_clicks_counted(recognizer)-1));
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context)
{
  racetimer_event_handler(EVENT_SETTINGS);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context)
{
  racetimer_event_handler(EVENT_CLICK_DOWN);
}

static void click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 500, up_repeat_click_handler);
  window_multi_click_subscribe(BUTTON_ID_UP,      1, settings_get_num_of_profiles(), 300, true, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN,   down_click_handler);
}

static void SetProfileIcon(uint8_t id)
{
  switch(id)
  {
    case 0:
      s_icon_profile = bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_1);
      break;
    case 1:
      s_icon_profile = bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_2);
      break;
    case 2:
      s_icon_profile = bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_3);
      break;
    case 3:
      s_icon_profile = bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_4);
      break;
    case 4:
      s_icon_profile = bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_5);
      break;
  }
}

static void SetActionBarIcons(racetimer_icons icons)
{
  switch(icons)
  {
    case ICONS_STOPPED:
    {
      SetProfileIcon(settings_get_active_profile());
      action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, s_icon_profile);
      action_bar_layer_set_icon(action_bar,   BUTTON_ID_SELECT, s_icon_settings);
      action_bar_layer_set_icon(action_bar,   BUTTON_ID_DOWN, s_icon_start);
    }
    break;
    case ICONS_RUNNING:
      action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, s_icon_stop);
      action_bar_layer_clear_icon(action_bar, BUTTON_ID_SELECT);
      action_bar_layer_set_icon(action_bar,   BUTTON_ID_DOWN,   s_icon_pause);
    break;
    case ICONS_PAUSED:
      action_bar_layer_set_icon(action_bar,   BUTTON_ID_UP,     s_icon_stop);
      action_bar_layer_clear_icon(action_bar, BUTTON_ID_SELECT);
      action_bar_layer_set_icon(action_bar,   BUTTON_ID_DOWN,   s_icon_start);
    break;
    default:
    break;
  }
}

static void racetimer_reset(void)
{
  DEBUG("%s\n",__func__);
  timer_reset(rctimer);

  progress_layer_set_foreground_color(s_progress_layer, PROGRESS_FG_COLOR_PRETIMER);
  progress_layer_set_progress(s_progress_layer, 100);

  timer_time_str_ms(settings()->pre_race_duration*10, false, pretime_str,sizeof(pretime_str));
  text_layer_set_text(pretimer_layer, pretime_str);
  timer_time_str_ms(settings()->race_duration*10, true, time_str,sizeof(time_str));
  text_layer_set_text(timer_layer, time_str);

  SetActionBarIcons(ICONS_STOPPED);
}

void racetimer_pause(void)
{
  DEBUG("%s\n",__func__);
  timer_pause(rctimer);
  SetActionBarIcons(ICONS_PAUSED);
}

static void racetimer_start_pre_race(void)
{
  DEBUG("%s\n",__func__);
  timer_reset(rctimer);
  timer_set_length(rctimer, settings()->pre_race_duration);
  timer_set_interval_vibration(rctimer, settings()->pre_race_interval);
  timer_set_expired_vibration(rctimer, settings()->pre_race_over_vibe);
  timer_register_update_cb(rctimer, pre_race_update_cb, NULL);
  timer_register_expired_cb(rctimer, timer_expired_cb, NULL);

  s_progress_size = settings()->pre_race_duration*10;

  timer_start(rctimer);
  SetActionBarIcons(ICONS_RUNNING);
}

static void racetimer_start_race(void)
{
  DEBUG("%s\n",__func__);
  timer_reset(rctimer);
  timer_set_length(rctimer, settings()->race_duration);
  timer_set_interval_vibration(rctimer, settings()->race_interval);
  timer_set_expired_vibration(rctimer, settings()->race_over_vibe);
  timer_set_before_expire_warning_length(rctimer,settings()->race_over_warning);
  timer_register_update_cb(rctimer, race_update_cb,NULL);
  timer_register_expired_cb(rctimer, timer_expired_cb, NULL);

  s_progress_size = settings()->race_duration*10;
  progress_layer_set_progress(s_progress_layer, s_progress);
  progress_layer_set_foreground_color(s_progress_layer, PROGRESS_FG_COLOR);

  timer_start(rctimer);
  SetActionBarIcons(ICONS_RUNNING);
}

static void racetimer_start_after_race(void)
{
  DEBUG("%s\n",__func__);
  timer_reset(rctimer);
  timer_set_length(rctimer, 0);
  timer_set_interval_vibration(rctimer, settings()->after_race_interval);
  timer_register_update_cb(rctimer, after_race_update_cb,NULL);

  timer_start(rctimer);
  SetActionBarIcons(ICONS_RUNNING);
}

void racetimer_resume(void)
{
  DEBUG("%s\n",__func__);
  timer_start(rctimer);
  SetActionBarIcons(ICONS_RUNNING);
}

static void racetimer_setting_cb(void)
{
  racetimer_event_handler(EVENT_INIT);
}

static void racetimer_event_handler(racetimer_event event)
{
  racetimer_event_handler_with_clicks(event, 0);
}

static void racetimer_event_handler_with_clicks(racetimer_event event, uint8_t clicks)
{
  static uint8_t cnt=0;
  racetimer_state new_state = state;

  DEBUG("%2d STATE      %s", cnt, STATES_STRING[state]);
  DEBUG("%2d PREV_STATE %s", cnt, STATES_STRING[prev_state]);
  DEBUG("%2d EVENT      %s", cnt, EVENTS_STRING[event]);

  switch(state)
  {
    case STATE_STOPPED:
      switch(event)
      {
        case EVENT_INIT:
          racetimer_reset();
          break;
        case EVENT_CLICK_UP:
          settings_set_active_profile(clicks);
          racetimer_reset();
          break;

        case EVENT_CLICK_DOWN:
          if(settings()->pre_race_duration == 0) // No pretimer
          {
            new_state = STATE_RACE_RUNNING;
            racetimer_start_race();
          }
          else
          {
            new_state = STATE_PRE_RACE_RUNNING;
            racetimer_start_pre_race();
          }
          break;
        case EVENT_SETTINGS:
          settings_push_window(racetimer_setting_cb);
        default:
          break;
      }
      break;
    case STATE_PRE_RACE_RUNNING:
      switch(event)
      {
        case EVENT_CLICK_UP:
          // Stop
          new_state = STATE_STOPPED;
          racetimer_reset();
          break;
        case EVENT_CLICK_DOWN:
          // pause
          new_state = STATE_PAUSED;
          racetimer_pause();
          break;
        case EVENT_TIMER_EXPIRED:
          new_state = STATE_RACE_RUNNING;
          racetimer_start_race();
          break;
        default:
          break;
      }
      break;
    case STATE_RACE_RUNNING:
      switch(event)
      {
        case EVENT_CLICK_UP:
          // Stop
          new_state = STATE_STOPPED;
          racetimer_reset();
          break;
        case EVENT_CLICK_DOWN:
          // pause
          new_state = STATE_PAUSED;
          racetimer_pause();
          break;
        case EVENT_TIMER_EXPIRED:
          new_state = STATE_AFTER_RACE_RUNNING;
          racetimer_start_after_race();
          break;
        default:
          break;
      }
      break;
    case STATE_AFTER_RACE_RUNNING:
      switch(event)
      {
        case EVENT_CLICK_UP:
          // Stop
          new_state = STATE_STOPPED;
          racetimer_reset();
          break;
        case EVENT_CLICK_DOWN:
          // pause
          new_state = STATE_PAUSED;
          racetimer_pause();
          break;
        default:
          break;
      }
      break;
    case STATE_PAUSED:
      switch(event)
      {
        case EVENT_CLICK_UP:
          // Stop
          new_state = STATE_STOPPED;
          racetimer_reset();
          break;
        case EVENT_CLICK_DOWN:
          // resume
          new_state = prev_state;
          racetimer_resume();
          break;
        default:
          break;
      }
      break;
  }
  DEBUG("%2d NEW_STATE  %s",cnt, STATES_STRING[new_state]);
  prev_state = state;
  state = new_state;
  cnt++;
  cnt = cnt % 100;
}

static void window_appear(Window *window) {
//  racetimer_reset();
}

static void window_load(Window *window) {
  HEAP_CHECK_START();

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  title_layer = text_layer_create((GRect){
#if defined(PBL_ROUND)
    .origin = { 15, 20 },
    .size = { bounds.size.w - ACTION_BAR_WIDTH - 3, 20 }
#else
    .origin = { 0, 16 },
    .size = { bounds.size.w - ACTION_BAR_WIDTH - 3, 20 }
#endif
    });
  text_layer_set_text_alignment(title_layer, GTextAlignmentCenter);
  text_layer_set_font(title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(title_layer, "Race Timer");

  pretimer_layer = text_layer_create((GRect){
    .origin = { 6, 50 },
    .size = { bounds.size.w - ACTION_BAR_WIDTH - 12, 34 }
  });
  text_layer_set_text_alignment(pretimer_layer, GTextAlignmentRight);
  text_layer_set_font(pretimer_layer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
//text_layer_set_background_color(pretimer_layer,GColorGreen);

  timer_layer = text_layer_create((GRect){
    .origin = { 6, 84 },
    .size = { bounds.size.w - ACTION_BAR_WIDTH - 12, 34 }
  });
//  text_layer_set_font(timer_layer, fonts_load_resource_font(RESOURCE_ID_FONT_FUTURA_CONDENSED_24));
  text_layer_set_font(timer_layer, fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD));
  text_layer_set_text_alignment(timer_layer, GTextAlignmentRight);
//text_layer_set_background_color(timer_layer,GColorYellow);

  layer_add_child(window_layer, text_layer_get_layer(title_layer));
  layer_add_child(window_layer, text_layer_get_layer(pretimer_layer));
  layer_add_child(window_layer, text_layer_get_layer(timer_layer));


  // Initialize the action bar:
  action_bar = action_bar_layer_create();

  action_bar_layer_set_background_color(action_bar, ACTION_BAR_COLOR);
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(action_bar,
                                             click_config_provider);

  s_icon_stop = bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_STOP);
  s_icon_start = bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_PLAY);
  s_icon_pause = bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_PAUSE);
  s_icon_settings = bitmaps_get_sub_bitmap(RESOURCE_ID_ICONS, ICON_RECT_SETTINGS);

  // Status bar
  init_statusbar_text_layer(window_layer);

// Progressbar
  s_progress_layer = progress_layer_create((GRect){
#if defined(PBL_ROUND)
    .origin = { 18, 124 },
    .size = { bounds.size.w - ACTION_BAR_WIDTH - 24, 10 }
#else
    .origin = { 6, 124 },
    .size = { bounds.size.w - ACTION_BAR_WIDTH - 12, 20 }
#endif
    });
  progress_layer_set_progress(s_progress_layer, 0);
  progress_layer_set_corner_radius(s_progress_layer, 2);
  progress_layer_set_foreground_color(s_progress_layer, PROGRESS_FG_COLOR);
  progress_layer_set_background_color(s_progress_layer, GColorBlack);
  layer_add_child(window_layer, s_progress_layer);

  rctimer = timer_create();

  racetimer_event_handler(EVENT_INIT);
  HEAP_CHECK_STOP();
}

static void window_unload(Window *window) {
  HEAP_CHECK_START();
  deinit_statusbar();
  text_layer_destroy(title_layer);
  text_layer_destroy(pretimer_layer);
  text_layer_destroy(timer_layer);
  timer_destroy(rctimer);
  action_bar_layer_remove_from_window(action_bar);
  action_bar_layer_destroy(action_bar);
  layer_destroy(s_progress_layer);
  HEAP_CHECK_STOP();
}

void racetimer_init(void) {
  HEAP_CHECK_START();

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
    .appear = window_appear,
 //   .disappear = window_disappear
  });
  window_stack_push(window,true);
  HEAP_CHECK_STOP();
}

void racetimer_deinit(void) {
  HEAP_CHECK_START();
  window_destroy(window);
  HEAP_CHECK_STOP();
}
