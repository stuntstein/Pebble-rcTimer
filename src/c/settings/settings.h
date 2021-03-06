#pragma once

#include "../timer.h"

#define SETTINGS_VERSION_CURRENT 2
#define SETTINGS_VERSION_OLD_0   0

#define OLD_SETTINGS_KEY 1              // This key holds the old settings
#define SETTINGS_KEY     2              // This key holds the V2 settings
#define SETTINGS_VERSION_KEY 101        // This key holds the version of the setting format


typedef enum rctimer_mode_t
{
  RACETIMER_MODE,
  LAPTIMER_MODE
}rctimer_mode_t;

typedef struct {
  rctimer_mode_t  rctimer_mode;              // if 0 start racetimer othervise run laptimer
  uint32_t        pre_race_duration; // timer duration before racetimer starts
  uint32_t        pre_race_interval;
  TimerVibration  pre_race_end_vibe;

  uint32_t        race_duration;    // race timer duration
  uint32_t        race_interval;
  uint32_t        race_eor_warning;
  TimerVibration  race_end_vibe;

  uint32_t        after_race_interval;

} old_settings_t;


typedef struct {
  uint32_t        pre_race_duration; // timer duration before racetimer starts
  uint32_t        pre_race_interval;
  TimerVibration  pre_race_over_vibe;

  uint32_t        race_duration;    // race timer duration
  uint32_t        race_interval;
  uint32_t        race_over_warning;
  TimerVibration  race_over_vibe;

  uint32_t        after_race_interval;
} settings_t;

typedef void (*SettingsCallback)(void);

settings_t* settings();

void settings_init(void);
void settings_deinit(void);
void settings_push_window(SettingsCallback callback);


uint8_t settings_get_num_of_profiles(void);
void settings_set_active_profile(uint8_t);
uint8_t settings_get_active_profile(void);


