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
#include "utils.h"
#include "src/c/place_description.h"
#include "src/c/settings.h"
#include "src/c/place_layer.h"
#include "src/c/arrows_layer.h"

//#include "today_layer.h"

//window
static Window *s_window;  

//Layers
static struct MapLayer map_layer_struct;

//static bool position_known = false;
const time_t OUTDATE_TIME = 1200;

// An instance of the struct
static Settings settings;
  
//static struct date_layer date_l;

static place_layer place1, place2;
static Layer *arrow_layer1, *arrow_layer2;

//this is a reference to botom place
static Layer *bottom_place_layer;


static void handle_update_settings(){
    text_layer_set_text_color(place1.place_time_layer, settings.TextColor);
    text_layer_set_text_color(place1.place_name_layer, settings.TextColor);
    text_layer_set_text_color(place2.place_time_layer, settings.TextColor);
    text_layer_set_text_color(place2.place_name_layer, settings.TextColor);
    //     text_layer_set_text_color(current.place_time_layer, settings.TextColor);
    //     text_layer_set_text_color(current.place_name_layer, settings.TextColor);
    update_place_layer(&place1);
    update_place_layer(&place2);
    //     switch_panels_if_required();
    layer_mark_dirty(window_get_root_layer(s_window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  //GRect bounds = layer_get_bounds(window_layer);
  //TODO make ALL constants variable
  
  //load settings
  prv_load_settings(&settings);
  handle_update_settings();

  //place layers
  create_place_layer_default(&place1, &settings.place1, 0,
                             &settings, window_layer);
  create_place_layer_default(&place2, &settings.place2, 120,                                 
                             &settings, window_layer);
  bottom_place_layer = place2.place_layer;
  
  //map layer
  BitmapLayer *map_layer = map_leyer_create(GPoint(0, 48), &map_layer_struct);
  Layer *map_layer_base = bitmap_layer_get_layer(map_layer);
  layer_add_child(window_layer, map_layer_base);
  GRect map_frame = layer_get_frame(map_layer_base);
  
  //arrow layers
  arrow_layer1 = arrows_layer_create(map_frame,
                                 &place1, 
                                 &settings, 
                                 get_point_on_map,
                                 false);
  layer_add_child(map_layer_base, arrow_layer1);
  arrow_layer2 = arrows_layer_create(map_frame,
                                 &place2, 
                                 &settings, 
                                 get_point_on_map,
                                 true);
  layer_add_child(map_layer_base, arrow_layer2);
  
  
  //   //date
  //   Layer *date_root = create_date_layer(&date_l);
  //   layer_add_child(bitmap_layer_get_layer(map_layer), date_root);
    
  //   int32_t x, y;
  //   get_dark_point_map((int) time(NULL), &x, &y);
  //   layer_set_center(date_root, x, y);
    
  //floating layer 
  //create_place_layer_floating(&current, &settings.place_cur, bitmap_layer_get_layer( map_layer));
  //layer_set_hidden(current.place_layer, true);
  
  //   //event animation API 4.0
  //   UnobstructedAreaHandlers handlers = {
  //     .will_change = prv_unobstructed_will_change,
  //     .change = prv_unobstructed_change
  //   };
  //   unobstructed_area_service_subscribe(handlers, NULL);
  
  //panel switching - to be removed
  //   APP_LOG(APP_LOG_LEVEL_DEBUG, "Going to swap pannels");
  //   switch_panels_if_required();
  //   APP_LOG(APP_LOG_LEVEL_DEBUG, "Going to update the floating pannel");
  //   update_floating_place(&current);
  //   APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading complete");
}

static void window_unload(Window *window) {
  destroy_place_layer(&place1);
  destroy_place_layer(&place2);
  
  //destroy_date_layer(&date_l);
  
  layer_destroy(arrow_layer1);
  layer_destroy(arrow_layer2);
  
  map_layer_destroy(&map_layer_struct);
  //unobstructed_area_service_unsubscribe();
  
}

static void update_time(place_layer *place, time_t *time){
  struct tm *tick_time = gmtime(time);
  strftime(place->watch_str, sizeof(place->watch_str), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Time in %s is updated to %s", place->place->place_name, place->watch_str); 
  layer_mark_dirty(text_layer_get_layer(place->place_time_layer));
}


static void handle_connection_change(bool connected){
  if (connected){
    //TODO improve logic
    //request_locaion();
  }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed){
  time_t now = time(NULL);
  time_t time1 = now + place1.place->offset;
  update_time(&place1, &time1);
  
  time_t time2 = now + place2.place->offset;
  update_time(&place2, &time2);
 
  //Handling current time
  //TODO upgrade
  //   strftime(current.watch_str, sizeof(current.watch_str), clock_is_24h_style() ?
  //                                           "%H:%M" : "%I:%M", tick_time);
  //   APP_LOG(APP_LOG_LEVEL_DEBUG, "Time in current location (%s) is updated to: %s", 
  //           settings.place_cur.place_name, current.watch_str); 
  //handle_zone_change();
  map_layer_redraw_minute(&map_layer_struct);
  
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