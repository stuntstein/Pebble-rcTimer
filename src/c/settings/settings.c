#include <pebble.h>
#include <utils/pebble-assist.h>
#include <utils/bitmap-loader.h>
#include "../rctimer.h"
#include "../icons.h"
#include "../about.h"
#include "settings.h"
#include "win-duration.h"

#define NUM_OF_PROFILES 5

#define TXT_SETTINGS            "Settings"
#define TXT_DURATION            "Duration"
#define TXT_INTERVAL            "Interval"
#define TXT_VIBE                "Vibe"
#define TXT_VIBE_INTERVAL       TXT_VIBE" "TXT_INTERVAL
#define TXT_START_RACE_INTERVAL ("Start Race "TXT_VIBE)
#define TXT_WARNING             "Warning"
#define TXT_EOR                 "End Of Race"
#define TXT_EOR_VIBE            (TXT_EOR" "TXT_VIBE)

#define TXT_PRE_RACE            "Pre Race"
#define TXT_RACE                "Race"
#define TXT_AFTER_RACE          "After Race"
#define TXT_ABOUT               "About rcTimer"

#define TXT_PROFILE             "Profile"
#define TXT_SELECT_PROFILE      ("Select "TXT_PROFILE)

#define TXT_PRE_RACE_SETTING    (TXT_PRE_RACE" "TXT_SETTINGS)
#define TXT_RACE_SETTING        (TXT_RACE" "TXT_SETTINGS)
#define TXT_AFTER_RACE_SETTING  (TXT_AFTER_RACE" "TXT_SETTINGS)
#define TXT_ABOUT               "About rcTimer"

// Sections setup
#define NUM_MENU_SECTIONS         5
#define MENU_SECTION_PROFILE      0
#define MENU_SECTION_PRE_RACE     1
#define MENU_SECTION_RACE         2
#define MENU_SECTION_AFTER_RACE   3
#define MENU_SECTION_ABOUT        4

// Profile menu
#define NUM_SETTINGS_PROFILE          1
#define MENU_SETTINGS_PROFILE_SELECT  0

// Pre Race Settings menu
#define NUM_SETTINGS_PRE_RACE_ITEMS     3
#define MENU_SETTINGS_PRE_RACE_DURATION 0
#define MENU_SETTINGS_PRE_RACE_INTERVAL 1
#define MENU_SETTINGS_PRE_RACE_END_VIBE 2

// Race Settings menu
#define NUM_SETTINGS_RACE_ITEMS       4
#define MENU_SETTINGS_RACE_DURATION   0
#define MENU_SETTINGS_RACE_INTERVAL   1
#define MENU_SETTINGS_RACE_OVER_WARN  2
#define MENU_SETTINGS_RACE_OVER_VIBE  3

// After Race Settings menu
#define NUM_SETTINGS_AFTER_RACE_ITEMS     1
#define MENU_SETTINGS_AFTER_RACE_INTERVAL 0

// About Settings
#define NUM_SETTINGS_ABOUT_ITEMS      2
#define MENU_SETTINGS_ABOUT_VERSION   0
#define MENU_SETTINGS_ABOUT_CREDITS   1


static Window *window;
static MenuLayer *s_menu_layer;
static SettingsCallback s_callback;

static void pre_race_duration_callback(uint32_t duration);
static void pre_race_interval_callback(uint32_t duration);

static void race_duration_callback(uint32_t duration);
static void race_interval_callback(uint32_t duration);
static void race_over_warn_callback(uint32_t duration);

static void after_race_interval_callback(uint32_t duration);

// Storage
typedef struct{
    uint8_t     active;
    settings_t  settings[NUM_OF_PROFILES];
} profile_setting_t;
profile_setting_t profile;

HEAP_CHECK;

static void settings_set_default(settings_t *setting) {

  *setting = (settings_t) {
    .pre_race_duration      = 5,
    .pre_race_interval      = 0,
    .pre_race_over_vibe     = TIMER_VIBE_NONE,

    .race_duration          = 300,
    .race_interval          = 60,
    .race_over_warning      = 15,
    .race_over_vibe         = TIMER_VIBE_TRIPLE,

    .after_race_interval    = 60
  };
}

static void settings_default_all(void)
{
  profile.active = 0;
  DEBUG("Default all Settings");

  settings_set_default(&profile.settings[0]);
  settings_set_default(&profile.settings[1]);
  settings_set_default(&profile.settings[2]);
  settings_set_default(&profile.settings[3]);
  settings_set_default(&profile.settings[4]);
}

static void settings_save(void) {
  DEBUG("Save Settings");
  if (0 > persist_write_data(SETTINGS_KEY, &profile, sizeof(profile_setting_t))) {
    LOG("Settings save failed");
  }
}

static void settings_load(void) {
  int current_version = persist_read_int(SETTINGS_VERSION_KEY);

  DEBUG("LOAD Settings: %d", current_version);

  if (SETTINGS_VERSION_CURRENT == current_version)
  {
    DEBUG("LOAD Settings");
    if (0 > persist_read_data(SETTINGS_KEY,&profile, sizeof(profile_setting_t)))
    {
      settings_default_all();
    }
  }
  else if (SETTINGS_VERSION_OLD_0 == current_version)
  {
    old_settings_t old_settings;

    // set all default and update version
    settings_default_all();
    persist_write_int(SETTINGS_VERSION_KEY, SETTINGS_VERSION_CURRENT);

    if (0 < persist_read_data(OLD_SETTINGS_KEY, &old_settings, sizeof(old_settings_t)))
    {
      // old setting exists!

      settings_t *p;

      DEBUG("Copy old Settings");
      persist_delete(OLD_SETTINGS_KEY);

      // copy from old to new format into profile1
      profile.active = 0;
      p = &profile.settings[profile.active];
      p->pre_race_duration = old_settings.pre_race_duration;
      p->pre_race_interval = old_settings.pre_race_interval;
      p->pre_race_over_vibe = old_settings.pre_race_end_vibe;
      p->race_duration = old_settings.race_duration;
      p->race_interval = old_settings.race_interval;
      p->race_over_warning = old_settings.race_eor_warning;
      p->race_over_vibe = old_settings.race_end_vibe;
      p->after_race_interval = old_settings.after_race_interval;
    }
  }
}

settings_t* settings() {
  return &profile.settings[profile.active];
}

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case MENU_SECTION_PROFILE:
      return NUM_SETTINGS_PROFILE;
    case MENU_SECTION_PRE_RACE:
      return NUM_SETTINGS_PRE_RACE_ITEMS;
    case MENU_SECTION_RACE:
      return NUM_SETTINGS_RACE_ITEMS;
    case MENU_SECTION_AFTER_RACE:
      return NUM_SETTINGS_AFTER_RACE_ITEMS;
    case MENU_SECTION_ABOUT:
      return NUM_SETTINGS_ABOUT_ITEMS;
    default:
      return 0;
  }
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Determine which section we're working with
  switch (section_index) {
    case MENU_SECTION_PROFILE:
      // Draw title text in the section header
      menu_cell_basic_header_draw(ctx, cell_layer, TXT_SELECT_PROFILE);
      break;
    case MENU_SECTION_PRE_RACE:
      // Draw title text in the section header
      menu_cell_basic_header_draw(ctx, cell_layer, TXT_PRE_RACE_SETTING);
      break;
    case MENU_SECTION_RACE:
      // Draw title text in the section header
      menu_cell_basic_header_draw(ctx, cell_layer, TXT_RACE_SETTING);
      break;
    case MENU_SECTION_AFTER_RACE:
      // Draw title text in the section header
      menu_cell_basic_header_draw(ctx, cell_layer, TXT_AFTER_RACE_SETTING);
      break;
    case MENU_SECTION_ABOUT:
      // Draw title text in the section header
      menu_cell_basic_header_draw(ctx, cell_layer, TXT_ABOUT);
      break;
  }
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  char str[16] = "";

  // Determine which section we're going to draw in
  switch (cell_index->section) {
    case MENU_SECTION_PROFILE:
      // Use the row to specify which item we'll draw
      if (MENU_SETTINGS_PROFILE_SELECT == cell_index->row)
      {
          // This is a basic menu item with a title and subtitle
          snprintf(str,sizeof(str),"%s %d",TXT_PROFILE, (int)profile.active + 1);
          menu_cell_basic_draw(ctx, cell_layer, TXT_PROFILE, str, NULL);
      }
      break;
    case MENU_SECTION_PRE_RACE:
      // Use the row to specify which item we'll draw
      switch (cell_index->row) {
        case MENU_SETTINGS_PRE_RACE_DURATION:
          // This is a basic menu item with a title and subtitle
          timer_time_str(settings()->pre_race_duration, str, sizeof(str));
          menu_cell_basic_draw(ctx, cell_layer, TXT_DURATION, str, NULL);
          break;
        case MENU_SETTINGS_PRE_RACE_INTERVAL:
          // This is a basic menu item with a title and subtitle
          timer_time_str(settings()->pre_race_interval, str, sizeof(str));
          menu_cell_basic_draw(ctx, cell_layer, TXT_VIBE_INTERVAL, str, NULL);
          break;
        case MENU_SETTINGS_PRE_RACE_END_VIBE:       // This is a basic menu item with a title and subtitle
          snprintf(str,sizeof(str),"%s",timer_vibe_str(settings()->pre_race_over_vibe,true));
          menu_cell_basic_draw(ctx, cell_layer, TXT_START_RACE_INTERVAL, str, NULL);
          break;
      }
      break;
    case MENU_SECTION_RACE:
      // Use the row to specify which item we'll draw
      switch (cell_index->row) {
        case MENU_SETTINGS_RACE_DURATION:
          // This is a basic menu item with a title and subtitle
          timer_time_str(settings()->race_duration, str, sizeof(str));
          menu_cell_basic_draw(ctx, cell_layer, TXT_DURATION, str, NULL);
          break;
        case MENU_SETTINGS_RACE_INTERVAL:
          // This is a basic menu item with a title and subtitle
          timer_time_str(settings()->race_interval, str, sizeof(str));
          menu_cell_basic_draw(ctx, cell_layer, TXT_VIBE_INTERVAL, str, NULL);
          break;
        case MENU_SETTINGS_RACE_OVER_WARN:
          // This is a basic menu item with a title and subtitle
          timer_time_str(settings()->race_over_warning, str, sizeof(str));
          menu_cell_basic_draw(ctx, cell_layer, (TXT_EOR" "TXT_WARNING), str, NULL);
          break;
        case MENU_SETTINGS_RACE_OVER_VIBE:       // This is a basic menu item with a title and subtitle
          snprintf(str,sizeof(str),"%s",timer_vibe_str(settings()->race_over_vibe,true));
          menu_cell_basic_draw(ctx, cell_layer, TXT_EOR_VIBE, str, NULL);
          break;
      }
      break;
    case MENU_SECTION_AFTER_RACE:
      // Use the row to specify which item we'll draw
      switch (cell_index->row) {
        case MENU_SETTINGS_AFTER_RACE_INTERVAL:
          // This is a basic menu item with a title and subtitle
          timer_time_str(settings()->after_race_interval, str, sizeof(str));
          menu_cell_basic_draw(ctx, cell_layer, TXT_VIBE_INTERVAL, str, NULL);
          break;
      }
      break;
    case MENU_SECTION_ABOUT:
      // Use the row to specify which item we'll draw
      switch (cell_index->row) {
        case MENU_SETTINGS_ABOUT_VERSION:
          menu_cell_basic_draw(ctx, cell_layer, "Version", RCTIMER_VERSION, NULL);
          break;
        case MENU_SETTINGS_ABOUT_CREDITS:
          menu_cell_title_draw(ctx, cell_layer, "Credits");
          break;
      }
      break;
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  switch (cell_index->section) {
    case MENU_SECTION_PROFILE:
      profile.active = (profile.active + 1) % NUM_OF_PROFILES;
      // After changing the item, mark the layer to have it updated
      layer_mark_dirty(menu_layer_get_layer(menu_layer));
      break;
    case MENU_SECTION_PRE_RACE:
      switch (cell_index->row) {
        case MENU_SETTINGS_PRE_RACE_DURATION:
          win_duration_show(settings()->pre_race_duration, pre_race_duration_callback, false, (TXT_PRE_RACE" "TXT_DURATION));
          break;
        case MENU_SETTINGS_PRE_RACE_INTERVAL:
          win_duration_show(settings()->pre_race_interval, pre_race_interval_callback, false,(TXT_PRE_RACE" "TXT_VIBE_INTERVAL));
          break;
        case MENU_SETTINGS_PRE_RACE_END_VIBE:
          settings()->pre_race_over_vibe = (settings()->pre_race_over_vibe + 1) % TIMER_VIBE_MAX;
          // After changing the item, mark the layer to have it updated
          layer_mark_dirty(menu_layer_get_layer(menu_layer));
          break;
      }
      break;
    case MENU_SECTION_RACE:
      switch (cell_index->row) {
        case MENU_SETTINGS_RACE_DURATION:
          win_duration_show(settings()->race_duration, race_duration_callback, true, (TXT_RACE" "TXT_DURATION));
          break;
        case MENU_SETTINGS_RACE_INTERVAL:
          win_duration_show(settings()->race_interval, race_interval_callback, true,(TXT_RACE" "TXT_VIBE_INTERVAL));
          break;
        case MENU_SETTINGS_RACE_OVER_WARN:
          win_duration_show(settings()->race_over_warning, race_over_warn_callback, false, (TXT_EOR" "TXT_WARNING));
          break;
        case MENU_SETTINGS_RACE_OVER_VIBE:
          settings()->race_over_vibe = (settings()->race_over_vibe + 1) % TIMER_VIBE_MAX;
          // After changing the item, mark the layer to have it updated
          layer_mark_dirty(menu_layer_get_layer(menu_layer));
          break;
      }
      break;
    case MENU_SECTION_AFTER_RACE:
      switch (cell_index->row) {
        case MENU_SETTINGS_AFTER_RACE_INTERVAL:
          win_duration_show(settings()->after_race_interval, after_race_interval_callback, true, (TXT_AFTER_RACE" "TXT_VIBE_INTERVAL));
          break;
      }
      break;
    case MENU_SECTION_ABOUT:
      switch (cell_index->row) {
        case MENU_SETTINGS_ABOUT_CREDITS:
          about_window_push();
          break;
      }
      break;
  }
}


//  window_stack_pop(true);
static void window_load(Window *window) {

  // Now we prepare to initialize the menu layer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  // Create the menu layer
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
#if !defined(PBL_PLATFORM_APLITE)
  menu_layer_set_highlight_colors(s_menu_layer, GColorYellow, GColorBlack);
#endif
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(s_menu_layer);
  if (s_callback)
    s_callback();
}


uint8_t settings_get_num_of_profiles(void)
{
  return NUM_OF_PROFILES;
}

void settings_set_active_profile(uint8_t id){
  profile.active = id % NUM_OF_PROFILES;
}

uint8_t settings_get_active_profile(void){
  return profile.active;
}

void settings_push_window(SettingsCallback callback){
  s_callback = callback;
  window_stack_push(window,true);
}

void settings_init()
{
  HEAP_CHECK_START();

  settings_load();
  win_duration_init();
  about_init();

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  HEAP_CHECK_STOP();
}

void settings_deinit(void) {
  HEAP_CHECK_START();
  about_deinit();
  win_duration_deinit();
  settings_save();
  window_destroy(window);
  HEAP_CHECK_STOP();
}


static void pre_race_duration_callback(uint32_t duration) {
  settings()->pre_race_duration = duration;
}

static void pre_race_interval_callback(uint32_t duration) {
  settings()->pre_race_interval = duration;
}

static void race_duration_callback(uint32_t duration) {
  settings()->race_duration = duration;
}

static void race_interval_callback(uint32_t duration) {
  settings()->race_interval = duration;
}

static void race_over_warn_callback(uint32_t duration) {
  settings()->race_over_warning = duration;
}

static void after_race_interval_callback(uint32_t duration) {
  settings()->after_race_interval = duration;
}

