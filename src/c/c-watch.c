#include <pebble.h>
#if defined(NOLOG)
#undef APP_LOG
#define APP_LOG(...)
#endif

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_sig_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static TextLayer *s_weather_layer;

// Store incoming information
static char temperature_buffer[8];
static char conditions_buffer[32];
static char weather_layer_buffer[32];

// Declare globally
static GFont s_time_font;
static GFont s_weather_font;

static uint flags = 0;


static void main_window_load(Window *window)
{
  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_22));

  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);

  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create temperature Layer
  s_weather_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(20, 10), bounds.size.w, 25));

  // Style the text
  text_layer_set_background_color(s_weather_layer, GColorClear);
  text_layer_set_text_color(s_weather_layer, GColorClear);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "! Loading !");

  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(bounds);

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, 52, bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Create the TextLayer with specific bounds
  s_sig_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(bounds.size.h - 50, bounds.size.h - 42), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_sig_layer, GColorClear);
  text_layer_set_text_color(s_sig_layer, GColorBlack);
  text_layer_set_text(s_sig_layer, "Muldy Watch");
  text_layer_set_font(s_sig_layer, s_time_font);
  text_layer_set_text_alignment(s_sig_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_sig_layer));

  // Create second custom font, apply it and add to Window
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_20));
  text_layer_set_font(s_weather_layer, s_weather_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
}
static void main_window_unload(Window *window)
{
  // Unload GFont
  fonts_unload_custom_font(s_time_font);

  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);

  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_sig_layer);

  // Destroy weather elements
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_weather_font);
}

static void deinit()
{
  // Destroy Window
  window_destroy(s_main_window);
}

static void update_time()
{
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s", temperature_buffer, conditions_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}
static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  // Get weather update every 30 minutes
  if((tick_time->tm_min % 30 == 0) || flags==0 ) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
    flags = 1;
  }
  update_time();
}

static void inbox_dropped_callback(AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}
static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC, ", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
  }
}

static void init()
{
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers){
                                                .load = main_window_load,
                                                .unload = main_window_unload});

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Make sure the time is displayed from the start
  update_time();

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}