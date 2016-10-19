
#include <pebble.h>
#include <utils/pebble-assist.h>
#include <utils/scroll-text-layer.h>

static Window *window;

// This is a scroll layer
static ScrollLayer *s_scroll_layer;

// We also use a text layer to scroll in the scroll layer
static TextLayer *s_text_layer;

static char* s_scroll_text = "Thanks to my wife for giving me all this time to race RC CARS.\n   Love U <3 \nI borrowed some ideas to parts of this app from different other apps, so thanks goes to Matthew Tole.\nPS: I fixed the heap leak in bitmap-loader.\0";


static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  GRect max_text_bounds = GRect(0, 0, bounds.size.w, 2000);

  // Initialize the scroll layer
  s_scroll_layer = scroll_layer_create(bounds);

  // This binds the scroll layer to the window so that up and down map to scrolling
  // You may use scroll_layer_set_callbacks to add or override interactivity
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);

  // Initialize the text layer
  s_text_layer = text_layer_create(max_text_bounds);
  text_layer_set_text(s_text_layer, s_scroll_text);

  // Change the font to a nice readable one
  // This is system font; you can inspect pebble_fonts.h for all system fonts
  // or you can take a look at feature_custom_font to add your own font
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(s_text_layer, GColorClear);

#if defined(PBL_PLATFORM_APLITE)
  text_layer_set_text_color(s_text_layer, GColorBlack);
#else
  text_layer_set_text_color(s_text_layer, GColorWhite);
#endif

  // Trim text layer and scroll content to fit text box
  GSize max_size = text_layer_get_content_size(s_text_layer);
  text_layer_set_size(s_text_layer, max_size);
  scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, max_size.h + 4));

  // Add the layers for display
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));

  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
 
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  scroll_layer_destroy(s_scroll_layer);
}

void about_window_push() {
#if defined(PBL_PLATFORM_APLITE)
    window_set_background_color(window, GColorWhite);
#else
    window_set_background_color(window, GColorBlueMoon);
#endif
  window_stack_push(window, true);
}

void about_init(void){
  if(!window) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
}

void about_deinit(void){
  window_destroy(window);
}

