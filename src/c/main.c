#include <pebble.h>
#include <utils/pebble-assist.h>
#include "rctimer.h"
#include "settings/settings.h"
#include "raceTimer/raceTimer.h"
#include <utils/bitmap-loader.h>
#include "about.h"

HEAP_CHECK;

static void init(void) {
  HEAP_CHECK_START();
  bitmaps_init();
  settings_init();

  racetimer_init();

  HEAP_CHECK_STOP();
}

static void deinit(void) {
  HEAP_CHECK_START();
  racetimer_deinit();
  settings_deinit();
  bitmaps_cleanup();
  HEAP_CHECK_STOP();
}


int main(void) {
  init();

  app_event_loop();

  deinit();
}