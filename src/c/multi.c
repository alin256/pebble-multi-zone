// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyleft: Sergey (aliner) Alyaev, 2017

#include <pebble.h>
#include "map_layer.h"
#include "today_layer.h"
#include "utils.h"
#include "my_types.h"
#include "src/c/settings.h"
#include "src/c/place_layer.h"


#ifdef PBL_PLATFORM_BASALT
//Rounded edges for Basalt
#define SCREEN_RADIUS 8;
#else
#define SCREEN_RADIUS 0;
#endif

#define REDRAW_INTERVAL 15

//string constants
const char weekdays[16] = "MoTuWeThFrSaSu";
const char question[1] = "?";

//window
static Window *s_window;  

//Layers
//map things
static BitmapLayer *map_layer;
static GBitmap *three_worlds;
static GBitmap *image;
static BitmapLayer *tmp_layer;

//arrows thing
static Layer *pointer_layer;

//static bool position_known = false;
const time_t OUTDATE_TIME = 1200;

//temporary string
static char tmp_time_zone[TIMEZONE_NAME_LENGTH];

// An instance of the struct
static Settings settings;
  
static struct date_layer date_l;

static place_layer place1, place2;
static place_layer current;

//this is a reference to botom place
static Layer *bottom_place_layer;

static bool condensing = true;

static void draw_number(GContext *ctx, GPoint middle, int16_t num){
  GRect box =  GRect(middle.x - radius*3/2, middle.y -radius, 
                                radius*3, radius*2);
  graphics_fill_rect(ctx, box, radius, GCornersAll);
  graphics_context_set_text_color(ctx, settings.TextColor);
  char *text = "00";
  snprintf(text, 2, "%d", (int) num);
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), box, 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}


const int16_t offset_mid = 2;

static GRect get_offset_rect(int16_t height, int16_t width, int16_t offset){
  GRect bounds = GRect(offset, offset, width - offset*2, height - offset*2);
  return bounds;  
}

static GRect get_offset_rect_right(int16_t height, int16_t width, int16_t offset, int16_t total_width){
  GRect bounds = GRect(offset+total_width-width, offset, width - offset*2, height - offset*2);
  return bounds;  
}


// static void request_locaion(){
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "Location requested !"); 
//   send_position_request();
// }

static void draw_floating_layer_undefined(struct Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);

  //fill insides
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);
  graphics_context_set_fill_color(ctx, settings.BackgroundColor);
  graphics_fill_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.w, 0), 
                     radius, GCornersAll);
  //draw contours
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_draw_round_rect(ctx, bounds, radius);
  graphics_context_set_text_color(ctx, settings.ForegroundColor);
  graphics_draw_text(ctx, &question[0], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), get_offset_rect(bounds.size.h, bounds.size.h, -1), GTextOverflowModeWordWrap, GTextAlignmentCenter, 0);
  //graphics_draw_round_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.h, 2), radius-2);
  //graphics_draw_round_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.h, radius-1), 1);
}

static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area,
void *context) {
  // Get the total available screen real-estate
  GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_window));
  if (bounds.size.h > final_unobstructed_screen_area.size.h){
    condensing = true;
  }else{
    condensing = false;
  }
  
  //GRect full_bounds = layer_get_bounds(window_get_root_layer(s_window));
  //   if (!grect_equal(&full_bounds, &final_unobstructed_screen_area)) {
  //     // Screen is about to become obstructed, hide the date
  //     layer_set_hidden(text_layer_get_layer(s_date_layer), true);
  //   }
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  // Get the total available screen real-estate
  GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_window));
  //Get frame of bubbles
  GRect bottom_bounds = layer_get_frame(bottom_place_layer);
  
  //move bottom layer
  bottom_bounds.origin.y = bounds.size.h - bottom_bounds.size.h;
  layer_set_frame(bottom_place_layer, bottom_bounds);
  
  //move map layer
  GRect map_bounds = layer_get_frame(bitmap_layer_get_layer( map_layer));
  int32_t directed_progress;
  if (condensing){
    directed_progress = progress-ANIMATION_NORMALIZED_MIN;
  }else{
    directed_progress = ANIMATION_NORMALIZED_MAX - progress;
  }
  GRect floating_bounds = layer_get_frame(current.place_layer);
  map_bounds.origin.y = bottom_bounds.size.h + 
    (- floating_bounds.origin.y + 2)
    *(directed_progress)/(ANIMATION_NORMALIZED_MAX-ANIMATION_NORMALIZED_MIN);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Animation progress: %d, directed: %d", (int) progress, (int)directed_progress);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "New y: %d", (int) map_bounds.origin.y);
  layer_set_frame(bitmap_layer_get_layer(map_layer), map_bounds);
  
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  //GRect bounds = layer_get_bounds(window_layer);
  //TODO make ALL constants variable
  
  //load settings
  prv_load_settings(&settings);

  // Load the image
  three_worlds = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_3_WORLDS);
  image = gbitmap_create_as_sub_bitmap(three_worlds, GRect(0, 0, WIDTH, HEIGHT));
  
  GRect map_bounds = GRect(0, 48, WIDTH, HEIGHT);
  map_layer = bitmap_layer_create(map_bounds);
  bitmap_layer_set_bitmap(map_layer, image);
  //(map_layer, draw_map);

  layer_add_child(window_layer, bitmap_layer_get_layer(map_layer));
  //draw_earth();
  
  tmp_layer = bitmap_layer_create(GRect(0,0, map_bounds.size.w, map_bounds.size.h));
  bitmap_layer_set_bitmap(tmp_layer, three_worlds);
  //layer_add_child(map_layer, bitmap_layer_get_layer(tmp_layer));
  
  //date
  Layer *date_root = create_date_layer(&date_l);
  layer_add_child(bitmap_layer_get_layer(map_layer), date_root);
  int32_t x, y;
  get_dark_point_map((int) time(NULL), &x, &y);
  layer_set_center(date_root, x, y);
  
  
  pointer_layer = layer_create(GRect(0,0, map_bounds.size.w, map_bounds.size.h));
  layer_set_update_proc(pointer_layer, draw_arrows);
  layer_add_child(bitmap_layer_get_layer( map_layer), pointer_layer);  
  
  create_place_layer_default(&place1, &settings.place1, 0, window_layer);
  create_place_layer_default(&place2, &settings.place2, 120, window_layer);
  bottom_place_layer = place2.place_layer;
  
  create_place_layer_floating(&current, &settings.place_cur, bitmap_layer_get_layer( map_layer));
  layer_set_hidden(current.place_layer, true);
  

  UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_will_change,
    .change = prv_unobstructed_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Going to swap pannels");
  switch_panels_if_required();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Going to update the floating pannel");
  update_floating_place(&current);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading complete");
}

static void window_unload(Window *window) {
  destroy_place_layer(&place1);
  destroy_place_layer(&place2);
  destroy_place_layer(&current);
  
  destroy_date_layer(&date_l);
  
  bitmap_layer_destroy(tmp_layer);
  
  layer_destroy(pointer_layer);
  bitmap_layer_destroy(map_layer);
  
  gbitmap_destroy(three_worlds);
  gbitmap_destroy(image);
  
  //unobstructed_area_service_unsubscribe();
  
}

static void update_time(place_descr *place, time_t *time){
  struct tm *tick_time = gmtime(time);
  strftime(place->watch_str, sizeof(place->watch_str), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Time in %s is updated to %s", place->place->place_name, place->watch_str); 
  layer_mark_dirty(text_layer_get_layer(place->place_time_layer));
}



static void handle_zone_change(){
  
  layer_set_hidden(current.place_layer, !settings.show_local_time);
  if (clock_is_timezone_set()){
    clock_get_timezone(tmp_time_zone, TIMEZONE_NAME_LENGTH);
    if (strcmp(tmp_time_zone, settings.place_cur.place_name) != 0){
      request_locaion();
      strcpy(settings.place_cur.place_name, tmp_time_zone);
    }
  }
}

static void handle_connection_change(bool connected){
  if (connected){
    //TODO improve logic
    request_locaion();
  }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed){
  time_t now = time(NULL);
  time_t time1 = now + place1.place->offset;
  update_time(&place1, &time1);
  
  time_t time2 = now + place2.place->offset;
  update_time(&place2, &time2);
 
  strftime(current.watch_str, sizeof(current.watch_str), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Time in current location (%s) is updated to: %s", 
          settings.place_cur.place_name, current.watch_str); 
  handle_zone_change();
  
  redraw_counter++;
  if (redraw_counter >= REDRAW_INTERVAL) {
    draw_earth(three_worlds, bitmap_layer_get_layer(map_layer));
    redraw_counter = 0;
  }
}

static void init(void) {
  s_window = window_create();  
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Pushing window");  
  window_stack_push(s_window, true);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Window pushed");  
  
  
  // Register AppMessage handlers
  app_message_register_inbox_received(in_received_handler); 
  app_message_register_inbox_dropped(in_dropped_handler); 
  app_message_register_outbox_failed(out_failed_handler);

  // Initialize AppMessage inbox and outbox buffers with a suitable size
  const int inbox_size = 512;
  const int outbox_size = 512;
  app_message_open(inbox_size, outbox_size);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  bluetooth_connection_service_subscribe(handle_connection_change);
}

static void deinit(void) {
  prv_save_settings(&settings);
  app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  window_destroy(s_window);
}

int main( void ) {
  init();
  app_event_loop();
  deinit();
}